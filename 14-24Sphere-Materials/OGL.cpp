#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <gl/glew.h>
#include <gl/GL.h>
#include "OGL.h"
#include "vmath.h"
#include "Sphere.h"

// Enum for attribute locations
// This enum is used to define attribute locations for shaders
enum
{
    SM_ATTRIBUTE_POSITION = 0,
    SM_ATTRIBUTE_COLOR,
    SM_ATTRIBUTE_NORMAL,
    SM_ATTRIBUTE_TEXCOORD
};

// Macros for window dimensions
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// Link necessary libraries
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "Sphere.lib")

using namespace std;
using namespace vmath;

// OpenGLApp Class Definition
// This class encapsulates the OpenGL application logic
class OpenGLApp
{
private:
    HWND ghwnd;                                                                                               // Handle to the window
    HDC ghdc;                                                                                                 // Handle to the device context
    HGLRC ghrc;                                                                                               // Handle to the rendering context
    FILE *gpFile;                                                                                             // Log file pointer
    GLuint shaderProgramObject;                                                                               // Shader program object
    GLuint vao, vbo_Position, vbo_Color, vbo_Texcoord, vbo_Normal, vbo_Element;                               // Vertex Array and Buffer Objects
    GLuint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform;                                    // Uniform locations
    GLuint lightDiffuseUniform, lightAmbientUniform, lightSpecularUniform, lightPositionUniform;              // Light uniform locations
    GLuint materialAmbientUniform, materialDiffuseUniform, materialSpecularUniform, materialShininessUniform; // Material uniform locations
    GLuint keyPressedUniform;                                                                                 // Key pressed uniform location
    mat4 perspectiveProjectionMatrix;                                                                         // Projection matrix
    BOOL gbFullscreen;                                                                                        // Fullscreen toggle flag
    BOOL gbActive;                                                                                            // Active window flag
    DWORD dwStyle;                                                                                            // Window style
    WINDOWPLACEMENT wpPrev;                                                                                   // Previous window placement

    GLuint NumSphereElements; // Number of elements in the sphere
    GLfloat lightAmbient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat lightDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightSpecular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightPosition[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    struct Material
    {
        vec4 ambient;
        vec4 diffuse;
        vec4 specular;
        vec1 shininess;
    };

    struct Material material[24];

public:
    BOOL bLightingEnabled; // Lighting enabled flag
    int keyPressed = 0;    // Key pressed flag
    GLfloat angleForXRotation = 0.0f;
    GLfloat angleForYRotation = 0.0f;
    GLfloat angleForZRotation = 0.0f;

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
        else if (wParam == 'L' || wParam == 'l') // Toggle lighting on 'L' key press
        {
            app.bLightingEnabled = !app.bLightingEnabled;
        }
        else if (wParam == 'X' || wParam == 'x') // Rotate around X-axis
        {
            app.keyPressed = 1;
            app.angleForXRotation = 0.0f; // Reset
        }
        else if (wParam == 'Y' || wParam == 'y') // Rotate around Y-axis
        {
            app.keyPressed = 2;
            app.angleForYRotation = 0.0f; // Reset
        }
        else if (wParam == 'Z' || wParam == 'z') // Rotate around Z-axis
        {
            app.keyPressed = 3;
            app.angleForZRotation = 0.0f; // Reset
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
                         vao(0), vbo_Position(0), vbo_Color(0), vbo_Texcoord(0), vbo_Normal(0),
                         modelMatrixUniform(0), viewMatrixUniform(0), projectionMatrixUniform(0),
                         lightDiffuseUniform(0), lightAmbientUniform(0), lightSpecularUniform(0), lightPositionUniform(0),
                         materialAmbientUniform(0), materialDiffuseUniform(0), materialSpecularUniform(0), materialShininessUniform(0),
                         keyPressedUniform(0), gbFullscreen(FALSE), gbActive(FALSE), dwStyle(0), wpPrev({sizeof(WINDOWPLACEMENT)})
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

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    material[0].ambient = vec4(0.0215f, 0.1745f, 0.0215f, 1.0f);
    material[0].diffuse = vec4(0.07568f, 0.61424f, 0.07568f, 1.0f);
    material[0].specular = vec4(0.633f, 0.727811f, 0.633f, 1.0f);
    material[0].shininess = vec1(0.6 * 128);

    material[1].ambient = vec4(0.135f, 0.2225f, 0.1575f, 1.0f);
    material[1].diffuse = vec4(0.54f, 0.89f, 0.63f, 1.0f);
    material[1].specular = vec4(0.316228f, 0.316228f, 0.316228f, 1.0f);
    material[1].shininess = vec1(0.1 * 128);

    material[2].ambient = vec4(0.05375f, 0.05f, 0.06625f, 1.0f);
    material[2].diffuse = vec4(0.18275f, 0.17f, 0.22525f, 1.0f);
    material[2].specular = vec4(0.332741f, 0.328634f, 0.346435f, 1.0f);
    material[2].shininess = vec1(0.3 * 128);

    material[3].ambient = vec4(0.25f, 0.20725f, 0.20725f, 1.0f);
    material[3].diffuse = vec4(1.0f, 0.829f, 0.829f, 1.0f);
    material[3].specular = vec4(0.296648f, 0.296648f, 0.296648f, 1.0f);
    material[3].shininess = vec1(0.088 * 128);

    material[4].ambient = vec4(0.1745f, 0.01175f, 0.01175f, 1.0f);
    material[4].diffuse = vec4(0.61424f, 0.04136f, 0.04136f, 1.0f);
    material[4].specular = vec4(0.727811f, 0.626959f, 0.626959f, 1.0f);
    material[4].shininess = vec1(0.6 * 128);

    material[5].ambient = vec4(0.1f, 0.18725f, 0.1745f, 1.0f);
    material[5].diffuse = vec4(0.396f, 0.74151f, 0.69102f, 1.0f);
    material[5].specular = vec4(0.297254f, 0.30829f, 0.306678f, 1.0f);
    material[5].shininess = vec1(0.1 * 128);

    material[6].ambient = vec4(0.329412f, 0.223529f, 0.027451f, 1.0f);
    material[6].diffuse = vec4(0.780392f, 0.568627f, 0.113725f, 1.0f);
    material[6].specular = vec4(0.992157f, 0.941176f, 0.807843f, 1.0f);
    material[6].shininess = vec1(0.21794872 * 128);

    material[7].ambient = vec4(0.2125f, 0.1275f, 0.054f, 1.0f);
    material[7].diffuse = vec4(0.714f, 0.4284f, 0.18144f, 1.0f);
    material[7].specular = vec4(0.393548f, 0.271906f, 0.166721f, 1.0f);
    material[7].shininess = vec1(0.2 * 128);

    material[8].ambient = vec4(0.25f, 0.25f, 0.25f, 1.0f);
    material[8].diffuse = vec4(0.4f, 0.4f, 0.4f, 1.0f);
    material[8].specular = vec4(0.774597f, 0.774597f, 0.774597f, 1.0f);
    material[8].shininess = vec1(0.6 * 128);

    material[9].ambient = vec4(0.19125f, 0.0735f, 0.0225f, 1.0f);
    material[9].diffuse = vec4(0.7038f, 0.27048f, 0.0828f, 1.0f);
    material[9].specular = vec4(0.256777f, 0.137622f, 0.086014f, 1.0f);
    material[9].shininess = vec1(0.1 * 128);

    material[10].ambient = vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
    material[10].diffuse = vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
    material[10].specular = vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
    material[10].shininess = vec1(0.4 * 128);

    material[11].ambient = vec4(0.19225f, 0.19225f, 0.19225f, 1.0f);
    material[11].diffuse = vec4(0.50754f, 0.50754f, 0.50754f, 1.0f);
    material[11].specular = vec4(0.508273f, 0.508273f, 0.508273f, 1.0f);
    material[11].shininess = vec1(0.4 * 128);

    material[12].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[12].diffuse = vec4(0.01f, 0.01f, 0.01f, 1.0f);
    material[12].specular = vec4(0.50f, 0.50f, 0.50f, 1.0f);
    material[12].shininess = vec1(0.25 * 128);

    material[13].ambient = vec4(0.0f, 0.1f, 0.06f, 1.0f);
    material[13].diffuse = vec4(0.0f, 0.50980392f, 0.50980392f, 1.0f);
    material[13].specular = vec4(0.50196078f, 0.50196078f, 0.50196078f, 1.0f);
    material[13].shininess = vec1(0.25 * 128);

    material[14].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[14].diffuse = vec4(0.1f, 0.35f, 0.1f, 1.0f);
    material[14].specular = vec4(0.45f, 0.55f, 0.45f, 1.0f);
    material[14].shininess = vec1(0.25 * 128);

    material[15].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[15].diffuse = vec4(0.5f, 0.0f, 0.0f, 1.0f);
    material[15].specular = vec4(0.7f, 0.6f, 0.6f, 1.0f);
    material[15].shininess = vec1(0.25 * 128);

    material[16].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[16].diffuse = vec4(0.55f, 0.55f, 0.55f, 1.0f);
    material[16].specular = vec4(0.7f, 0.7f, 0.7f, 1.0f);
    material[16].shininess = vec1(0.25 * 128);

    material[17].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[17].diffuse = vec4(0.5f, 0.5f, 0.0f, 1.0f);
    material[17].specular = vec4(0.6f, 0.6f, 0.5f, 1.0f);
    material[17].shininess = vec1(0.25 * 128);

    material[18].ambient = vec4(0.02f, 0.02f, 0.02f, 1.0f);
    material[18].diffuse = vec4(0.01f, 0.01f, 0.01f, 1.0f);
    material[18].specular = vec4(0.4f, 0.4f, 0.4f, 1.0f);
    material[18].shininess = vec1(0.078125 * 128);

    material[19].ambient = vec4(0.0f, 0.05f, 0.05f, 1.0f);
    material[19].diffuse = vec4(0.4f, 0.5f, 0.5f, 1.0f);
    material[19].specular = vec4(0.04f, 0.7f, 0.7f, 1.0f);
    material[19].shininess = vec1(0.078125 * 128);

    material[20].ambient = vec4(0.0f, 0.05f, 0.0f, 1.0f);
    material[20].diffuse = vec4(0.4f, 0.5f, 0.4f, 1.0f);
    material[20].specular = vec4(0.04f, 0.7f, 0.04f, 1.0f);
    material[20].shininess = vec1(0.078125 * 128);

    material[21].ambient = vec4(0.05f, 0.0f, 0.0f, 1.0f);
    material[21].diffuse = vec4(0.5f, 0.4f, 0.4f, 1.0f);
    material[21].specular = vec4(0.7f, 0.04f, 0.04f, 1.0f);
    material[21].shininess = vec1(0.078125 * 128);

    material[22].ambient = vec4(0.05f, 0.05f, 0.05f, 1.0f);
    material[22].diffuse = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    material[22].specular = vec4(0.7f, 0.7f, 0.7f, 1.0f);
    material[22].shininess = vec1(0.078125 * 128);

    material[23].ambient = vec4(0.05f, 0.05f, 0.0f, 1.0f);
    material[23].diffuse = vec4(0.5f, 0.5f, 0.4f, 1.0f);
    material[23].specular = vec4(0.7f, 0.7f, 0.04f, 1.0f);
    material[23].shininess = vec1(0.078125 * 128);

    lightPosition[0] = angleForXRotation;
    lightPosition[1] = angleForYRotation;
    lightPosition[2] = angleForZRotation;

    resize(WIN_WIDTH, WIN_HEIGHT);
    return 0;
}

// Sets up shaders for rendering
void OpenGLApp::setupShaders()
{
    string vertexShaderSource = readShaderSource("vertexShader.vert");
    string fragmentShaderSource = readShaderSource("fragmentShader.frag");

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
    glBindAttribLocation(shaderProgramObject, SM_ATTRIBUTE_NORMAL, "aNormal");
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

    modelMatrixUniform = glGetUniformLocation(shaderProgramObject, "uModelMatrix");
    viewMatrixUniform = glGetUniformLocation(shaderProgramObject, "uViewMatrix");
    projectionMatrixUniform = glGetUniformLocation(shaderProgramObject, "uProjectionViewMatrix");
    lightAmbientUniform = glGetUniformLocation(shaderProgramObject, "lightAmbientUniform");
    lightDiffuseUniform = glGetUniformLocation(shaderProgramObject, "uLightDiffuse");
    lightSpecularUniform = glGetUniformLocation(shaderProgramObject, "uLightSpecular");
    lightPositionUniform = glGetUniformLocation(shaderProgramObject, "uLightPosition");
    materialAmbientUniform = glGetUniformLocation(shaderProgramObject, "uMaterialAmbient");
    materialDiffuseUniform = glGetUniformLocation(shaderProgramObject, "uMaterialDiffuse");
    materialSpecularUniform = glGetUniformLocation(shaderProgramObject, "uMaterialSpecular");
    materialShininessUniform = glGetUniformLocation(shaderProgramObject, "uMaterialShininess");

    keyPressedUniform = glGetUniformLocation(shaderProgramObject, "ukeypressed");
}

// Sets up vertex buffers for rendering
void OpenGLApp::setupBuffers()
{
    float sphere_Position[1146];
    float sphere_normals[1146];
    float sphere_texcoord[764];
    unsigned short sphere_elements[2280];

    getSphereVertexData(sphere_Position, sphere_normals, sphere_texcoord, sphere_elements);
    NumSphereElements = getNumberOfSphereElements();

    // Sphere
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO for Position
    glGenBuffers(1, &vbo_Position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_Position), sphere_Position, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_POSITION); // caps
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // VBO for Normal
    glGenBuffers(1, &vbo_Normal);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Normal);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_normals), sphere_normals, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_NORMAL); // caps
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vbo_Texcoord);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_Texcoord);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_texcoord), sphere_texcoord, GL_STATIC_DRAW);
    glVertexAttribPointer(SM_ATTRIBUTE_TEXCOORD, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(SM_ATTRIBUTE_TEXCOORD); // caps
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // element vbo
    glGenBuffers(1, &vbo_Element);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_Element);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphere_elements), sphere_elements, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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

    // Define grid parameters
    const int numColumns = 4;
    const int numRows = 6;
    const float spacing = 2.0f;     // Adjust spacing between spheres
    const float zPosition = -15.0f; // Common z position for all spheres

    // Loop through rows and columns
    for (int row = 0; row < numRows; ++row)
    {
        for (int col = 0; col < numColumns; ++col)
        {
            // Compute position for this sphere
            float x = col * spacing - (numColumns - 1) * spacing / 2.0f;
            float y = row * spacing - (numRows - 1) * spacing / 2.0f;

            // Setup transformation matrices
            mat4 modelMatrix = mat4::identity();
            mat4 viewMatrix = mat4::identity();
            mat4 translationMatrix = vmath::translate(x, y, zPosition);
            modelMatrix = translationMatrix;

            // Push the MVP matrices to the vertex shader
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, modelMatrix);
            glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, viewMatrix);
            glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, perspectiveProjectionMatrix);

            if (bLightingEnabled == TRUE)
            {
                glUniform1i(keyPressedUniform, 1);

                glUniform3fv(lightAmbientUniform, 1, lightAmbient);
                glUniform3fv(lightDiffuseUniform, 1, lightDiffuse);
                glUniform3fv(lightSpecularUniform, 1, lightSpecular);
                glUniform4fv(lightPositionUniform, 1, lightPosition);

                // Set the material properties for the current sphere
                int materialIndex = row * numColumns + col;
                if (materialIndex < 24) // Ensure we don't go out of bounds
                {
                    glUniform3fv(materialAmbientUniform, 1, material[materialIndex].ambient);
                    glUniform3fv(materialDiffuseUniform, 1, material[materialIndex].diffuse);
                    glUniform3fv(materialSpecularUniform, 1, material[materialIndex].specular);
                    glUniform1fv(materialShininessUniform, 1, material[materialIndex].shininess);
                }
            }
            else
            {
                glUniform1i(keyPressedUniform, 0);
            }

            glBindVertexArray(vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_Element);
            glDrawElements(GL_TRIANGLES, NumSphereElements, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
        }
    }

    glUseProgram(0);

    SwapBuffers(ghdc);
}

// Updates application logic (currently empty)
void OpenGLApp::update()
{
    // Code
    if (keyPressed == 1) // Rotate around X-axis
    {
        lightPosition[0] = 0.0f;
        lightPosition[1] = 5.0f * cos(angleForXRotation); // Y-axis component
        lightPosition[2] = 5.0f * sin(angleForXRotation); // Z-axis component
        angleForXRotation += 0.002f;
        if (angleForXRotation > 2 * M_PI)
            angleForXRotation -= 2 * M_PI;
    }

    if (keyPressed == 2) // Rotate around Y-axis
    {
        lightPosition[0] = 5.0f * cos(angleForYRotation); // X-axis component
        lightPosition[1] = 0.0f;
        lightPosition[2] = 5.0f * sin(angleForYRotation); // Z-axis component
        angleForYRotation += 0.002f;
        if (angleForYRotation > 2 * M_PI)
            angleForYRotation -= 2 * M_PI;
    }

    if (keyPressed == 3) // Rotate around Z-axis
    {
        lightPosition[0] = 5.0f * cos(angleForZRotation); // X-axis component
        lightPosition[1] = 5.0f * sin(angleForZRotation); // Y-axis component
        lightPosition[2] = 0.0f;
        angleForZRotation += 0.002f;
        if (angleForZRotation > 2 * M_PI)
            angleForZRotation -= 2 * M_PI;
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

    if (vbo_Element)
    {
        glDeleteBuffers(1, &vbo_Element);
    }

    if (vbo_Texcoord)
    {
        glDeleteBuffers(1, &vbo_Texcoord);
    }

    if (vbo_Normal)
    {
        glDeleteBuffers(1, &vbo_Normal);
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
