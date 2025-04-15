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
    SM_ATTRIBUTE_POSITION = 0,
    SM_ATTRIBUTE_TEXCOORD,
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
    HWND ghwnd;                                                                     // Handle to the window
    HDC ghdc;                                                                       // Handle to the device context
    HGLRC ghrc;                                                                     // Handle to the rendering context
    FILE *gpFile;                                                                   // Log file pointer
    GLuint shaderProgramObject;                                                     // Shader program object
    GLuint vao, vbo_Position, vbo_Texture, mvpMatrixUniform, textureSamplerUniform; // OpenGL objects
    mat4 perspectiveProjectionMatrix;                                               // Projection matrix
    BOOL gbFullscreen;                                                              // Fullscreen toggle flag
    BOOL gbActive;                                                                  // Active window flag
    DWORD dwStyle;                                                                  // Window style
    WINDOWPLACEMENT wpPrev;                                                         // Previous window placement
    GLfloat cAngle = 0.0f;                                                          // Angle for rotation
    GLuint texture_Front, texture_Back, texture_Left, texture_Right, texture_Top, texture_Bottom;

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
    void logError(const char *message);                           // Log errors to file
    void printGLInfo();                                           // Print OpenGL information
    string readShaderSource(const char *filePath);                // Read shader source from file
    void setupShaders();                                          // Setup shaders
    void setupBuffers();                                          // Setup vertex buffers
    BOOL loadGlTexture(GLuint *texture, TCHAR imageResourceId[]); // Load OpenGL texture
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
                         vao(0), vbo_Position(0), vbo_Texture(0), mvpMatrixUniform(0), textureSamplerUniform(0),
                         texture_Front(0), texture_Back(0), texture_Left(0), texture_Right(0), texture_Top(0), texture_Bottom(0),
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
    BOOL bResult = FALSE;

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

    // create texture
    if (!loadGlTexture(&texture_Front, MAKEINTRESOURCE(FRONT_TEXTURE)) ||
        !loadGlTexture(&texture_Back, MAKEINTRESOURCE(BACK_TEXTURE)) ||
        !loadGlTexture(&texture_Left, MAKEINTRESOURCE(LEFT_TEXTURE)) ||
        !loadGlTexture(&texture_Right, MAKEINTRESOURCE(RIGHT_TEXTURE)) ||
        !loadGlTexture(&texture_Top, MAKEINTRESOURCE(TOP_TEXTURE)) ||
        !loadGlTexture(&texture_Bottom, MAKEINTRESOURCE(BOTTOM_TEXTURE)))
    {
        fprintf(gpFile, "Loading cube textures failed\n");
        return -7;
    }

    glEnable(GL_TEXTURE_2D);

    resize(WIN_WIDTH, WIN_HEIGHT);
    return 0;
}

// Sets up shaders for rendering
void OpenGLApp::setupShaders()
{
    string vertexShaderSource = readShaderSource("vertex_shader.vert");
    string fragmentShaderSource = readShaderSource("fragment_shader.frag");

    if (vertexShaderSource.empty() || fragmentShaderSource.empty())
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
    glAttachShader(shaderProgramObject, fragmentShader);
    glBindAttribLocation(shaderProgramObject, SM_ATTRIBUTE_POSITION, "aPosition");
    glBindAttribLocation(shaderProgramObject, SM_ATTRIBUTE_TEXCOORD, "aTexCoord");
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

    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");
    textureSamplerUniform = glGetUniformLocation(shaderProgramObject, "uTextureSampler");
}

// Sets up vertex buffers for rendering
void OpenGLApp::setupBuffers()
{
    const GLfloat cube_Position[] =
        {
            // front
            1.0f, 1.0f, 1.0f,   // top-right of front
            -1.0f, 1.0f, 1.0f,  // top-left of front
            -1.0f, -1.0f, 1.0f, // bottom-left of front
            1.0f, -1.0f, 1.0f,  // bottom-right of front

            // right
            1.0f, 1.0f, -1.0f,  // top-right of right
            1.0f, 1.0f, 1.0f,   // top-left of right
            1.0f, -1.0f, 1.0f,  // bottom-left of right
            1.0f, -1.0f, -1.0f, // bottom-right of right

            // back
            1.0f, 1.0f, -1.0f,   // top-right of back
            -1.0f, 1.0f, -1.0f,  // top-left of back
            -1.0f, -1.0f, -1.0f, // bottom-left of back
            1.0f, -1.0f, -1.0f,  // bottom-right of back

            // left
            -1.0f, 1.0f, 1.0f,   // top-right of left
            -1.0f, 1.0f, -1.0f,  // top-left of left
            -1.0f, -1.0f, -1.0f, // bottom-left of left
            -1.0f, -1.0f, 1.0f,  // bottom-right of left

            // top
            1.0f, 1.0f, -1.0f,  // top-right of top
            -1.0f, 1.0f, -1.0f, // top-left of top
            -1.0f, 1.0f, 1.0f,  // bottom-left of top
            1.0f, 1.0f, 1.0f,   // bottom-right of top

            // bottom
            1.0f, -1.0f, 1.0f,   // top-right of bottom
            -1.0f, -1.0f, 1.0f,  // top-left of bottom
            -1.0f, -1.0f, -1.0f, // bottom-left of bottom
            1.0f, -1.0f, -1.0f,  // bottom-right of bottom
        };

    const GLfloat cube_Texcoord[] =
        {
            // front
            1.0f, 1.0f, // top-right of front
            0.0f, 1.0f, // top-left of front
            0.0f, 0.0f, // bottom-left of front
            1.0f, 0.0f, // bottom-right of front

            // right
            1.0f, 1.0f, // top-right of right
            0.0f, 1.0f, // top-left of right
            0.0f, 0.0f, // bottom-left of right
            1.0f, 0.0f, // bottom-right of right

            // back
            1.0f, 1.0f, // top-right of back
            0.0f, 1.0f, // top-left of back
            0.0f, 0.0f, // bottom-left of back
            1.0f, 0.0f, // bottom-right of back

            // left
            1.0f, 1.0f, // top-right of left
            0.0f, 1.0f, // top-left of left
            0.0f, 0.0f, // bottom-left of left
            1.0f, 0.0f, // bottom-right of left

            // top
            1.0f, 1.0f, // top-right of top
            0.0f, 1.0f, // top-left of top
            0.0f, 0.0f, // bottom-left of top
            1.0f, 0.0f, // bottom-right of top

            // bottom
            1.0f, 1.0f, // top-right of bottom
            0.0f, 1.0f, // top-left of bottom
            0.0f, 0.0f, // bottom-left of bottom
            1.0f, 0.0f, // bottom-right of bottom
        };

    // WAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO for Position
    glGenBuffers(1, &vbo_Position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_Position), cube_Position, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VBO for texture
    glGenBuffers(1, &vbo_Texture);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Texture);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_Texcoord), cube_Texcoord, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_TEXCOORD);
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

    mat4 modelViewMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    mat4 scaleMatrix = mat4::identity();
    mat4 rotationMatrix = mat4::identity();

    translationMatrix = translate(0.0f, 0.0f, -3.0f); // Translate the cube
    scaleMatrix = scale(0.5f, 0.5f, 0.5f);            // Scale the cube

    mat4 rotationMatrixX = rotate(cAngle, 1.0f, 0.0f, 0.0f);              // Rotate around X-axis
    mat4 rotationMatrixY = rotate(cAngle, 0.0f, 1.0f, 0.0f);              // Rotate around Y-axis
    mat4 rotationMatrixZ = rotate(cAngle, 0.0f, 0.0f, 1.0f);              // Rotate around Z-axis
    rotationMatrix = rotationMatrixX * rotationMatrixY * rotationMatrixZ; // Combine rotations

    modelViewMatrix = translationMatrix * scaleMatrix * rotationMatrix;             // Model-View matrix
    mat4 modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix; // MVP matrix

    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix); // Pass MVP matrix to shader

    glBindVertexArray(vao);

    // Front face
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_Front);
    glUniform1i(textureSamplerUniform, 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Back face
    glBindTexture(GL_TEXTURE_2D, texture_Back);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    // Left face
    glBindTexture(GL_TEXTURE_2D, texture_Left);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

    // Right face
    glBindTexture(GL_TEXTURE_2D, texture_Right);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

    // Top face
    glBindTexture(GL_TEXTURE_2D, texture_Top);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

    // Bottom face
    glBindTexture(GL_TEXTURE_2D, texture_Bottom);
    glDrawArrays(GL_TRIANGLE_FAN, 20, 4);
    glBindVertexArray(0);

    glUseProgram(0);

    SwapBuffers(ghdc);
}

// Updates application logic (currently empty)
void OpenGLApp::update()
{
    // Update logic
    cAngle = cAngle - 0.02f;

    if (cAngle <= 0.0f)
    {
        cAngle = cAngle + 360.0f;
    }
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

    if (vbo_Texture)
    {
        glDeleteBuffers(1, &vbo_Texture);
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

BOOL OpenGLApp::loadGlTexture(GLuint *texture, TCHAR imageResourceId[])
{
    // local variable declaration
    HBITMAP hBitmap = NULL;
    BITMAP bmp;

    // Load the Image
    hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageResourceId, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hBitmap == NULL)
    {
        fprintf(gpFile, "Load Image Failed\n");
        return FALSE;
    }

    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    // create opengltexture
    glGenTextures(1, texture);

    glBindTexture(GL_TEXTURE_2D, *texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, bmp.bmWidth, bmp.bmHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, (void *)bmp.bmBits);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    DeleteObject(hBitmap);

    hBitmap = NULL;

    return TRUE;
}
