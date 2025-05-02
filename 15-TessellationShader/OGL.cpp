#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <gl/glew.h>
#include <gl/GL.h>
#include "OGL.h"
#include "vmath.h"

// Enum for attribute locations
// This enum is used to define attribute locations for shaders
enum
{
    SM_ATTRIBUTE_POSITION = 0
};

// Macros for window dimensions
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// Link necessary libraries
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")

using namespace std;
using namespace vmath;

// OpenGLApp Class Definition
// This class encapsulates the OpenGL application logic
class OpenGLApp
{
private:
    HWND ghwnd;                                                               // Handle to the window
    HDC ghdc;                                                                 // Handle to the device context
    HGLRC ghrc;                                                               // Handle to the rendering context
    FILE *gpFile;                                                             // Log file pointer
    GLuint shaderProgramObject;                                               // Shader program object
    GLuint vao, vbo_Position, vbo_Color, mvpMatrixUniform;                    // OpenGL objects
    GLuint numberOfSegmentsUniform, numberOfStripsUnforms, lineColorUniforms; // Uniforms for shaders
    mat4 perspectiveProjectionMatrix;                                         // Projection matrix
    BOOL gbFullscreen;                                                        // Fullscreen toggle flag
    BOOL gbActive;                                                            // Active window flag
    DWORD dwStyle;                                                            // Window style
    WINDOWPLACEMENT wpPrev;                                                   // Previous window placement

public:
    unsigned int uiNumberofLineSegments; // Number of line segments

public:
    OpenGLApp();                        // Constructor
    ~OpenGLApp();                       // Destructor
    void setActive(BOOL active);        // Set active state
    BOOL isActive() const;              // Check if active
    void ToggleFullscreen();            // Toggle fullscreen mode
    int initialize(HWND hwnd);          // Initialize OpenGL
    void resize(int width, int height); // Handle window resizing
    void display();                     // Render the scene
    void update();                      // Update logic
    void uninitialize();                // Cleanup resources

private:
    void logError(const char *message);            // Log errors to file
    void printGLInfo();                            // Print OpenGL information
    string readShaderSource(const char *filePath); // Read shader source from file
    void setupShaders();                           // Setup shaders
    void setupBuffers();                           // Setup vertex buffers
};

// Global instance of OpenGLApp
OpenGLApp app;

// Window Procedure
// Handles window messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_SETFOCUS:
        app.setActive(TRUE); // Set app as active when window gains focus
        break;
    case WM_KILLFOCUS:
        app.setActive(FALSE); // Set app as inactive when window loses focus
        break;
    case WM_SIZE:
        app.resize(LOWORD(lParam), HIWORD(lParam)); // Handle window resizing
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) // Exit on ESC key press
        {
            DestroyWindow(hwnd);
        }
        else if (wParam == 'F' || wParam == 'f') // Toggle fullscreen on 'F' key press
        {
            app.ToggleFullscreen();
        }
        else if (wParam == VK_UP) // Increase number of line segments
        {
            app.uiNumberofLineSegments++;
            if (app.uiNumberofLineSegments >= 30)
                app.uiNumberofLineSegments = 30;
        }
        else if (wParam == VK_DOWN) // Decrease number of line segments
        {
            app.uiNumberofLineSegments--;
            if (app.uiNumberofLineSegments <= 1)
                app.uiNumberofLineSegments = 1;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd); // Destroy window on close
        break;
    case WM_DESTROY:
        PostQuitMessage(0); // Post quit message
        break;
    default:
        return DefWindowProc(hwnd, iMsg, wParam, lParam); // Default message handling
    }
    return 0;
}

// Entry Point
// Main function for the application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // Define and register the window class
    WNDCLASSEX wndclass = {sizeof(WNDCLASSEX)};
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = NULL;
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = TEXT("OpenGLApp");
    wndclass.lpszMenuName = NULL;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));

    RegisterClassEx(&wndclass);

    // Create the application window
    HWND hwnd = CreateWindow(TEXT("OpenGLApp"), TEXT("OpenGL Application"),
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             WIN_WIDTH, WIN_HEIGHT, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    // Initialize the OpenGL application
    if (app.initialize(hwnd) != 0)
    {
        MessageBox(hwnd, TEXT("Initialization failed"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 0;
    }

    // Main message loop
    MSG msg;
    BOOL bDone = FALSE;
    while (!bDone)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bDone = TRUE; // Exit loop on quit message
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if (app.isActive()) // Render and update only if the app is active
            {
                app.display();
                app.update();
            }
        }
    }

    return (int)msg.wParam;
}

// OpenGLApp Class Implementation

// Constructor
// Initializes member variables and creates a log file
OpenGLApp::OpenGLApp() : ghwnd(NULL), ghdc(NULL), ghrc(NULL), gpFile(NULL), shaderProgramObject(0),
                         vao(0), vbo_Position(0), mvpMatrixUniform(0), numberOfSegmentsUniform(0),
                         numberOfStripsUnforms(0), lineColorUniforms(0), uiNumberofLineSegments(1),
                         gbFullscreen(FALSE), gbActive(FALSE), dwStyle(0), wpPrev({sizeof(WINDOWPLACEMENT)})
{
    fopen_s(&gpFile, "Log.txt", "w");
    if (!gpFile)
    {
        MessageBox(NULL, TEXT("Log file could not be created"), TEXT("Error"), MB_OK | MB_ICONERROR);
    }
    else
    {
        fprintf(gpFile, "Log file created successfully\n");
    }
}

// Destructor
// Cleans up resources
OpenGLApp::~OpenGLApp()
{
    uninitialize();
}

// Sets the active state of the application
void OpenGLApp::setActive(BOOL active)
{
    gbActive = active;
}

// Returns whether the application is active
BOOL OpenGLApp::isActive() const
{
    return gbActive;
}

// Toggles fullscreen mode
void OpenGLApp::ToggleFullscreen()
{
    MONITORINFO mi = {sizeof(MONITORINFO)};
    if (!gbFullscreen)
    {
        dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
        if (dwStyle & WS_OVERLAPPEDWINDOW)
        {
            if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
            {
                SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                             mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                             SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
        ShowCursor(FALSE);
        gbFullscreen = TRUE;
    }
    else
    {
        SetWindowPlacement(ghwnd, &wpPrev);
        SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
        gbFullscreen = FALSE;
    }
}

// Initializes OpenGL and sets up the rendering context
int OpenGLApp::initialize(HWND hwnd)
{
    ghwnd = hwnd;
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex = 0;

    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;

    ghdc = GetDC(ghwnd);
    if (!ghdc)
    {
        logError("GetDC failed");
        return -1;
    }

    iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
    if (iPixelFormatIndex == 0)
    {
        logError("ChoosePixelFormat failed");
        return -2;
    }

    if (!SetPixelFormat(ghdc, iPixelFormatIndex, &pfd))
    {
        logError("SetPixelFormat failed");
        return -3;
    }

    ghrc = wglCreateContext(ghdc);
    if (!ghrc)
    {
        logError("wglCreateContext failed");
        return -4;
    }

    if (!wglMakeCurrent(ghdc, ghrc))
    {
        logError("wglMakeCurrent failed");
        return -5;
    }

    if (glewInit() != GLEW_OK)
    {
        logError("glewInit failed");
        return -6;
    }

    printGLInfo();
    setupShaders();
    setupBuffers();

    // Enabling Depth
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    resize(WIN_WIDTH, WIN_HEIGHT);

    return 0;
}

// Sets up shaders for rendering
void OpenGLApp::setupShaders()
{
    string vertexShaderSource = readShaderSource("vertexShader.vert");
    string tessellationControlShaderSource = readShaderSource("tessellationControlShader.tesc");
    string tessellationEvaluationShaderSource = readShaderSource("tessellationEvaluationShader.tese");
    string fragmentShaderSource = readShaderSource("fragmentShader.frag");

    if (vertexShaderSource.empty() && fragmentShaderSource.empty() && tessellationControlShaderSource.empty() && tessellationEvaluationShaderSource.empty())
    {
        logError("Shader source is empty");
        return;
    }

    const GLchar *vertexShaderCode = vertexShaderSource.c_str();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        logError("Vertex Shader Compilation Failed");
        logError(infoLog);
    }

    // Tessellation Control Shader
    const GLchar *tessControlShaderCode = tessellationControlShaderSource.c_str();
    GLuint tessControlShader = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tessControlShader, 1, &tessControlShaderCode, NULL);
    glCompileShader(tessControlShader);

    glGetShaderiv(tessControlShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(tessControlShader, 512, NULL, infoLog);
        logError("tessControl Shader Compilation Failed");
        logError(infoLog);
    }

    // Tessellation Evaluation Shader
    const GLchar *tessEvalShaderCode = tessellationEvaluationShaderSource.c_str();
    GLuint tessEvalShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tessEvalShader, 1, &tessEvalShaderCode, NULL);
    glCompileShader(tessEvalShader);

    glGetShaderiv(tessEvalShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(tessEvalShader, 512, NULL, infoLog);
        logError("tessEval Shader Compilation Failed");
        logError(infoLog);
    }

    const GLchar *fragmentShaderCode = fragmentShaderSource.c_str();
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        logError("Fragment Shader Compilation Failed");
        logError(infoLog);
    }

    shaderProgramObject = glCreateProgram();
    glAttachShader(shaderProgramObject, vertexShader);
    glAttachShader(shaderProgramObject, tessControlShader);
    glAttachShader(shaderProgramObject, tessEvalShader);
    glAttachShader(shaderProgramObject, fragmentShader);
    glBindAttribLocation(shaderProgramObject, SM_ATTRIBUTE_POSITION, "aPosition");
    glLinkProgram(shaderProgramObject);

    glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(shaderProgramObject, 512, NULL, infoLog);
        logError("Shader Program Linking Failed");
        logError(infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get Shader Uniform Location
    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");
    numberOfSegmentsUniform = glGetUniformLocation(shaderProgramObject, "uNumberOfSegments");
    numberOfStripsUnforms = glGetUniformLocation(shaderProgramObject, "uNumberOfStrips");
    lineColorUniforms = glGetUniformLocation(shaderProgramObject, "uLineColor");
}

// Sets up vertex buffers for rendering
void OpenGLApp::setupBuffers()
{
    const GLfloat point_Position[] =
        {
            -1.0, -1.0,
            -0.5, 1.0,
            0.5, -1.0,
            1.0, 1.0};

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO for Position
    glGenBuffers(1, &vbo_Position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point_Position), point_Position, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

// Handles window resizing and updates the projection matrix
void OpenGLApp::resize(int width, int height)
{
    if (height <= 0)
        height = 1;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    perspectiveProjectionMatrix = perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

// Renders the scene
void OpenGLApp::display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramObject);

    // transformation
    mat4 modelViewMatrix = vmath::translate(0.0f, 0.0f, -3.0f);
    mat4 modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix; // order of multiplication is important

    // push above mvp into vertex shader's mvp uniform
    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);
    glUniform1i(numberOfSegmentsUniform, uiNumberofLineSegments);
    glUniform1i(numberOfStripsUnforms, 1);
    glUniform4fv(lineColorUniforms, 1, vmath::vec4(1.0, 1.0, 0.0, 1.0));

    // display chamging number of segments in windows title bar
    // TCHAR str

    // tell opengl how many vertices required by patch
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    glBindVertexArray(vao);

    glDrawArrays(GL_PATCHES, 0, 4);

    glBindVertexArray(0);

    glUseProgram(0);

    SwapBuffers(ghdc);
}

// Updates application logic (currently empty)
void OpenGLApp::update()
{
    // Update logic here
}

// Cleans up resources and uninitializes OpenGL
void OpenGLApp::uninitialize()
{
    if (shaderProgramObject)
    {
        glUseProgram(shaderProgramObject);
        GLint numShaders = 0;
        glGetProgramiv(shaderProgramObject, GL_ATTACHED_SHADERS, &numShaders);
        if (numShaders > 0)
        {
            GLuint *shaders = (GLuint *)malloc(numShaders * sizeof(GLuint));
            if (shaders)
            {
                glGetAttachedShaders(shaderProgramObject, numShaders, NULL, shaders);
                for (GLint i = 0; i < numShaders; i++)
                {
                    glDetachShader(shaderProgramObject, shaders[i]);
                    glDeleteShader(shaders[i]);
                }
                free(shaders);
            }
        }
        glUseProgram(0);
        glDeleteProgram(shaderProgramObject);
    }

    if (vbo_Color)
    {
        glDeleteBuffers(1, &vbo_Color);
    }

    if (vbo_Position)
    {
        glDeleteBuffers(1, &vbo_Position);
    }

    if (vao)
    {
        glDeleteVertexArrays(1, &vao);
    }

    if (gbFullscreen)
    {
        ToggleFullscreen();
    }

    if (wglGetCurrentContext() == ghrc)
    {
        wglMakeCurrent(NULL, NULL);
    }

    if (ghrc)
    {
        wglDeleteContext(ghrc);
        ghrc = NULL;
    }

    if (ghdc)
    {
        ReleaseDC(ghwnd, ghdc);
        ghdc = NULL;
    }

    if (gpFile)
    {
        fprintf(gpFile, "Program Ended Successfully\n");
        fclose(gpFile);
        gpFile = NULL;
    }
}

// Logs errors to the log file
void OpenGLApp::logError(const char *message)
{
    if (gpFile)
    {
        fprintf(gpFile, "%s\n", message);
    }
}

// Prints OpenGL information to the log file
void OpenGLApp::printGLInfo()
{
    fprintf(gpFile, "OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version: %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

// Reads shader source code from a file
string OpenGLApp::readShaderSource(const char *filePath)
{
    ifstream file(filePath);
    if (!file.is_open())
    {
        logError("Failed to open shader file");
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
