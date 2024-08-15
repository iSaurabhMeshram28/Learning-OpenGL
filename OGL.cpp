// Windows header files
#include <windows.h>
#include <stdio.h>  // File I/O
#include <stdlib.h> // For Exit
#include <string>
#include <fstream>
#include <streambuf>

// OpenGL Header file
#include <gl/glew.h> //this must be before gl/GL.h
#include <gl/GL.h>

// imgui
#include "imGUI/imgui.h"
#include "imGUI/imgui_impl_win32.h"
#include "imGUI/imgui_impl_opengl3.h"

#include "vmath.h"
using namespace vmath;
using namespace std;
#include "OGL.h"

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// Link with OpenGL library
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")

// Global function declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// Global variable Declarations
FILE *gpFile = NULL;
BOOL gbActive = FALSE;
HWND ghwnd = NULL;
DWORD dwStyle = 0;
WINDOWPLACEMENT wpPrev = {sizeof(WINDOWPLACEMENT)};
BOOL gbFullscreen = FALSE;

// OpenGL Global Variable
HDC ghdc = NULL;
HGLRC ghrc = NULL;

GLuint shaderProgramObject = 0;
enum
{
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR
};

GLuint vao = 0;
GLuint vbo_Position = 0;
GLuint vbo_Color = 0;

GLuint mvpMatrixUniform = 0;

mat4 perspectiveProjectionMatrix; // mat4 is in vmath.h

GLfloat topVertexColor[] = {1.0f, 0.0f, 0.0f};
GLfloat bottomLeftVertexColor[] = {0.0f, 1.0f, 0.0f};
GLfloat bottomRightVertexColor[] = {0.0f, 0.0f, 1.0f};

GLfloat triangle_Color[3][3];

GLint status = 0;
GLint infoLogLength = 0;
GLchar *szInfoLog = NULL;

void ToggleFullscreen(void)
{
    // Local variable declaration
    MONITORINFO mi = {sizeof(MONITORINFO)};

    // Code
    if (gbFullscreen == FALSE)
    {
        dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
        if (dwStyle & WS_OVERLAPPEDWINDOW)
        {
            if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
            {
                SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
        ShowCursor(FALSE);
        gbFullscreen = TRUE;
    }
    else
    {
        SetWindowPlacement(ghwnd, &wpPrev);
        SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
        gbFullscreen = FALSE;
    }
}

void imgui_initialization(void)
{
    void uninitialize(void);

    // imgui implementation
    IMGUI_CHECKVERSION();

    if (ImGui::CreateContext() == NULL)
    {
        fprintf(gpFile, "ImGui::CreateContext() Failed \n");
        uninitialize();
    }

    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    if (ImGui_ImplWin32_InitForOpenGL(ghwnd) == false)
    {
        fprintf(gpFile, "ImGui_ImplWin32_InitForOpenGL(ghwnd) Failed \n");
        uninitialize();
    }

    if (ImGui_ImplOpenGL3_Init("#version 460") == false)
    {
        fprintf(gpFile, "ImGui_ImplWin32_InitForOpenGL(ghwnd) Failed \n");
        uninitialize();
    }

    ImGui::StyleColorsDark();
}

GLuint loadShader(GLenum shaderType, const char *shaderFile)
{
    void uninitialize(void);

    string shaderCode;
    ifstream shaderFileStream(shaderFile);
    if (shaderFileStream.is_open())
    {
        shaderCode.assign((istreambuf_iterator<char>(shaderFileStream)), istreambuf_iterator<char>());
        shaderFileStream.close();
    }
    else
    {
        fprintf(gpFile, "Failed to open shader file: %s\n", shaderFile);
        uninitialize();
        return 0;
    }

    GLuint shaderObject = glCreateShader(shaderType);
    const char *shaderCodePtr = shaderCode.c_str();
    glShaderSource(shaderObject, 1, &shaderCodePtr, NULL);
    glCompileShader(shaderObject);

    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            szInfoLog = (GLchar *)malloc(infoLogLength);
            if (szInfoLog != NULL)
            {
                glGetShaderInfoLog(shaderObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "Shader compilation error log: %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
                uninitialize();
                return 0;
            }
        }
    }

    return shaderObject;
}

int initialize(void)
{
    void printGLInfo(void);
    void resize(int, int);

    void uninitialize(void);
    // Function Declaration
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex = 0;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

    // Code
    // Initialization of PIXELFORMATDESCRIPTOR
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;

    // Play background music
    // PlaySound(MAKEINTRESOURCE(MYMUSIC), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);

    // Get the Device Context
    ghdc = GetDC(ghwnd);

    if (ghdc == NULL)
    {
        fprintf(gpFile, "GetDC Function Failed\n");
        return -1;
    }

    iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);

    if (iPixelFormatIndex == 0)
    {
        fprintf(gpFile, "ChoosePixelFormat Function Failed\n");
        return -2;
    }

    if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
    {
        fprintf(gpFile, "SetPixelFormat Function Failed\n");
        return -3;
    }

    // Create OpenGL context from device context
    ghrc = wglCreateContext(ghdc);

    if (ghrc == NULL)
    {
        fprintf(gpFile, "wglCreateContext Function Failed\n");
        return -4;
    }

    // Make rendering context current
    if (wglMakeCurrent(ghdc, ghrc) == FALSE)
    {
        fprintf(gpFile, "wglMakeCurrent Function Failed\n");
        return -5;
    }

    // initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        fprintf(gpFile, "glewInit Function Failed\n");
        return -6;
    }

    // print gL info
    printGLInfo();

    GLuint vertexShaderObject = loadShader(GL_VERTEX_SHADER, "Shaders/vertexShader.glsl");
    GLuint fragmentShaderObject = loadShader(GL_FRAGMENT_SHADER, "Shaders/fragmentShader.glsl");

    // Shader Program
    shaderProgramObject = glCreateProgram();
    glAttachShader(shaderProgramObject, vertexShaderObject);
    glAttachShader(shaderProgramObject, fragmentShaderObject);
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_POSITION, "aPosition");
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_COLOR, "aColor");
    glLinkProgram(shaderProgramObject);

    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;

    glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        glGetProgramiv(shaderProgramObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0)
        {
            szInfoLog = (GLchar *)malloc(infoLogLength);
            if (szInfoLog != NULL)
            {
                glGetProgramInfoLog(shaderProgramObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "Shader Program compilation error log %s/n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
                uninitialize();
            }
        }
    }

    // Get Shader Uniform Location
    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");

    const GLfloat triangle_Position[] =
        {
            0.0f, 1.0f, 0.0f,
            -1.0, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f};

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO for Position
    glGenBuffers(1, &vbo_Position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_Position), triangle_Position, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION); // caps
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VBO for color
    glGenBuffers(1, &vbo_Color);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Color);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_Color), triangle_Color, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_COLOR); // caps
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // Enabling Depth
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Set the clear color of the window to blue
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    resize(WIN_WIDTH, WIN_HEIGHT);

    return 0;
}

void printGLInfo(void)
{
    // varaible Declaration
    GLint numExtension;
    GLint i;

    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // List of Supported Extension
    // glGetIntegerv(GL_NUM_EXTENSIONS, &numExtension);

    // for (i = 0; i < numExtension; i++)
    // {
    //     fprintf(gpFile, "%s\n", glGetStringi(GL_EXTENSIONS, i));
    // }
}

void resize(int width, int height)
{
    // Code
    if (height <= 0)
        height = 1;

    // Set the viewport to match the window dimensions
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // set perspectiveProjectionMatrix
    perspectiveProjectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void imgui_display(void)
{
    // imgui window
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    // ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    ImGui::Begin("PerVertex Color Sliders");
    {
        ImGui::SliderFloat3("Top Vertex", topVertexColor, 0.0f, 1.0f);
        ImGui::SliderFloat3("Left Bottom Vertex", bottomLeftVertexColor, 0.0f, 1.0f);
        ImGui::SliderFloat3("Right Bottom Vertex", bottomRightVertexColor, 0.0f, 1.0f);
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);      // top-left corner
    ImGui::SetNextWindowSize(ImVec2(150, 100), ImGuiCond_FirstUseEver); // width and height of the window

    ImGui::Begin("Dropdown Menu");
    {
        static int selectedColorIndex = 0;
        const char *colorOptions[] = {"Red", "Green", "Blue", "Yellow", "Purple"};
        ImGui::Combo("Color", &selectedColorIndex, colorOptions, IM_ARRAYSIZE(colorOptions));
    }
    ImGui::End();
}

void display(void)
{
    // Code
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader program object
    glUseProgram(shaderProgramObject);

    // Transformations
    mat4 translationMatrix = mat4::identity();
    mat4 modelViewMatrix = mat4::identity();
    mat4 modelViewProjectionMatrix = mat4::identity();

    translationMatrix = vmath::translate(0.0f, 0.0f, -4.0f);
    modelViewMatrix = translationMatrix;
    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);

    glBindVertexArray(vao);

    triangle_Color[0][0] = topVertexColor[0];
    triangle_Color[0][1] = topVertexColor[1];
    triangle_Color[0][2] = topVertexColor[2];

    triangle_Color[1][0] = bottomLeftVertexColor[0];
    triangle_Color[1][1] = bottomLeftVertexColor[1];
    triangle_Color[1][2] = bottomLeftVertexColor[2];

    triangle_Color[2][0] = bottomRightVertexColor[0];
    triangle_Color[2][1] = bottomRightVertexColor[1];
    triangle_Color[2][2] = bottomRightVertexColor[2];

    // Bind with vertex data buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Color);
    // Create storage for buffer data for a particular target
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_Color), triangle_Color, GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);

    // Unuse shader program object
    glUseProgram(0);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ImGuiIO& io = ImGui::GetIO();
    // if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
    // {
    // 	ImGui::UpdatePlatformWindows();
    // 	ImGui::RenderPlatformWindowsDefault();
    // }

    // Swap buffer
    SwapBuffers(ghdc);
}

void update(void)
{
    // Code
}

void uninitialize(void)
{
    // Code
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (shaderProgramObject)
    {
        glUseProgram(shaderProgramObject);

        GLint numShaders = 0;

        glGetProgramiv(shaderProgramObject, GL_ATTACHED_SHADERS, &numShaders);
        if (numShaders > 0)
        {
            GLuint *pShaders = (GLuint *)malloc(numShaders * sizeof(GLuint));
            if (pShaders != NULL)
            {
                glGetAttachedShaders(shaderProgramObject, numShaders, NULL, pShaders);
                for (GLint i = 0; i < numShaders; i++)
                {
                    glDetachShader(shaderProgramObject, pShaders[i]);
                    glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
                free(pShaders);
                pShaders = NULL;
            }
        }
        glUseProgram(0);
        glDeleteProgram(shaderProgramObject);
        shaderProgramObject = 0;
    }

    // delete vbo
    if (vbo_Color)
    {
        glDeleteBuffers(1, &vbo_Color);
        vbo_Color = 0;
    }

    if (vbo_Position)
    {
        glDeleteBuffers(1, &vbo_Position);
        vbo_Position = 0;
    }

    // delete vao
    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    // Clean up and release any resources here
    if (gbFullscreen == TRUE)
    {
        ToggleFullscreen();
        gbFullscreen = FALSE;
    }

    // Release the rendering context
    if (wglGetCurrentContext() == ghrc)
    {
        wglMakeCurrent(NULL, NULL);
    }

    if (ghrc)
    {
        wglDeleteContext(ghrc);
        ghrc = NULL;
    }

    // Release the device context
    if (ghdc)
    {
        ReleaseDC(ghwnd, ghdc);
    }

    // Destroy Window
    if (ghwnd)
    {
        DestroyWindow(ghwnd);
        ghwnd = NULL;
    }

    if (gpFile)
    {
        fprintf(gpFile, "Program Ended Successfully\n");
        fclose(gpFile);
        gpFile = NULL;
    }
}

// Entry Point Function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{

    // Function declarations
    int initialize(void);
    void display(void);
    void update(void);
    void uninitialize(void);
    void imgui_initialization(void);
    void imgui_display(void);

    // Get screen width and height
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int xPos = (screenWidth - WIN_WIDTH) / 2;
    int yPos = (screenHeight - WIN_HEIGHT) / 2;

    // Local Variable Declarations
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("SSMWindow");
    int iResult = 0;
    BOOL bDone = FALSE;

    // Code
    gpFile = fopen("log.txt", "w");
    if (gpFile == NULL)
    {
        MessageBox(NULL, TEXT("Log File Cannot be Opened"),
                   TEXT("Error"), MB_OK | MB_ICONERROR);
        return 0;
    }
    fprintf(gpFile, "Program Started Successfully\n");

    // WNDCLASSEX Initialization
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = NULL;                                 // Set background to NULL for OpenGL
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON)); // Set the small icon (window icon)
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName = NULL;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON)); // Set the small icon (taskbar icon)

    // Register WNDCLASSEX
    RegisterClassEx(&wndclass);

    // Create Window with the calculated position
    hwnd = CreateWindow(szAppName,
                        TEXT("Saurabh Meshram"),
                        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                        xPos, yPos,            // Position
                        WIN_WIDTH, WIN_HEIGHT, // Size
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

    ghwnd = hwnd;

    // Initialization
    iResult = initialize();
    if (iResult != 0)
    {
        MessageBox(hwnd, TEXT("Initialization function failed"),
                   TEXT("Error"), MB_OK | MB_ICONERROR);
        uninitialize();
        exit(0);
    }

    // Show The Window
    ShowWindow(hwnd, iCmdShow);

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    imgui_initialization();

    // Game Loop
    while (bDone == FALSE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                bDone = TRUE;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {

            if (gbActive == TRUE)
            {
                imgui_display();

                // Render
                display();

                // Update
                update();
            }
        }
    }

    uninitialize();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{

    // Function Declaration
    void ToggleFullscreen(void);
    void resize(int, int);
    void unInitialize(void);

    // delete events to imgui
    if (ImGui_ImplWin32_WndProcHandler(hwnd, iMsg, wParam, lParam))
    {
        return TRUE;
    }

    // Code
    switch (iMsg)
    {

    case WM_SETFOCUS:
        gbActive = TRUE;
        break;
    case WM_KILLFOCUS:
        gbActive = FALSE;
        break;
    case WM_SIZE:
        resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_ERASEBKGND:
        return (0);
    case WM_CHAR:
        switch (LOWORD(wParam))
        {
        case 'F':
        case 'f':
            ToggleFullscreen();
            break;
        }
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            DestroyWindow(hwnd);
            break;
        }

        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, iMsg, wParam, lParam);
    }

    return 0;
}
