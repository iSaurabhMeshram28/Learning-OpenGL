#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <gl/glew.h>
#include <gl/GL.h>
#include "OGL.h"

// Macros
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "OpenGL32.lib")

class OpenGLApp
{
private:
    HWND ghwnd;
    HDC ghdc;
    HGLRC ghrc;
    FILE *gpFile;
    GLuint shaderProgramObject;
    BOOL gbFullscreen;
    BOOL gbActive;
    DWORD dwStyle;
    WINDOWPLACEMENT wpPrev;

public:
    OpenGLApp::OpenGLApp() : ghwnd(NULL), ghdc(NULL), ghrc(NULL), gpFile(NULL), shaderProgramObject(0),
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

    ~OpenGLApp()
    {
        uninitialize();
    }

    void setActive(BOOL active)
    {
        gbActive = active;
    }

    BOOL isActive() const
    {
        return gbActive;
    }

    void ToggleFullscreen()
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

    int initialize(HWND hwnd)
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
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        return 0;
    }

    void resize(int width, int height)
    {
        if (height <= 0)
            height = 1;
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    }

    void display()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(ghdc);
    }

    void update()
    {
        // Update logic here
    }

    void uninitialize()
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

private:
    void logError(const char *message)
    {
        if (gpFile)
        {
            fprintf(gpFile, "%s\n", message);
        }
    }

    void printGLInfo()
    {
        fprintf(gpFile, "OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
        fprintf(gpFile, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
        fprintf(gpFile, "OpenGL Version: %s\n", glGetString(GL_VERSION));
    }

    void setupShaders()
    {
        const GLchar *vertexShaderSource =
            "#version 460 core\n"
            "void main() { }";

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
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

        const GLchar *fragmentShaderSource =
            "#version 460 core\n"
            "void main() { }";

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
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
    }
};

// Global instance
OpenGLApp app;

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_SETFOCUS:
        app.setActive(TRUE);
        break;
    case WM_KILLFOCUS:
        app.setActive(FALSE);
        break;
    case WM_SIZE:
        app.resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        else if (wParam == 'F' || wParam == 'f')
        {
            app.ToggleFullscreen();
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

// Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    WNDCLASSEX wndclass = {sizeof(WNDCLASSEX)};
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = NULL;                                 // Set background to NULL for OpenGL
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON)); // Set the small icon (window icon)
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = TEXT("OpenGLApp");
    wndclass.lpszMenuName = NULL;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON)); // Set the small icon (taskbar icon)

    RegisterClassEx(&wndclass);

    HWND hwnd = CreateWindow(TEXT("OpenGLApp"), TEXT("OpenGL Application"),
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             WIN_WIDTH, WIN_HEIGHT, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    if (app.initialize(hwnd) != 0)
    {
        MessageBox(hwnd, TEXT("Initialization failed"), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 0;
    }

    MSG msg;
    BOOL bDone = FALSE;
    while (!bDone)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bDone = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if (app.isActive())
            { // Use isActive() instead of directly accessing gbActive
                app.display();
                app.update();
            }
        }
    }

    return (int)msg.wParam;
}
