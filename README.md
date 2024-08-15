# Integrating ImGui into Win32 OpenGL Program

Overview

This guide will walk you through the steps required to integrate ImGui into a Win32 OpenGL application. We will cover everything from setting up ImGui, modifying the rendering loop, and linking ImGui to the project.

### Step 1: Setting Up ImGui

a. Download ImGui

Clone the ImGui repository from GitHub:

```bash
git clone https://github.com/ocornut/imgui.git
```
b. Include ImGui in the Project
	
Navigate to the imgui folder and add the following files to the it:
	
	imconfig.h
 	imgui.config 
	imgui.cpp
	imgui.h
	imgui_draw.cpp	
	imgui_tables.cpp	
	imgui_widgets.cpp	
	imgui_impl_win32.cpp
	imgui_impl_win32.h
	imgui_impl_opengl3.cpp
	imgui_impl_opengl3.h
	imgui_impl_opengl3_loader.h
	imgui_internal.h
	imstb_retpack.h
	imstb_textedit.h
	imstb_truetype.h
	
	
c. Linking Libraries

Add the necessary ImGui files:

	#include "imgui.h"
	#include "imgui_impl_win32.h"
	#include "imgui_impl_opengl3.h"
	
d. Add a global Function declaration

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
	
### Step 2: Initialization of ImGui

Add imgui_initialization function which is given below in the code and call it in WinMain function before the Game loop:

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


Explanation:

IMGUI_CHECKVERSION() ensures that the version of ImGui you’re using is compatible with the implementation.
ImGui::CreateContext() creates a new ImGui context.	
ImGui::StyleColorsDark() applies a default dark theme.
ImGui_ImplWin32_Init(hWnd); initializes ImGui for Win32.
ImGui_ImplOpenGL3_Init("#version 460"); initializes ImGui for OpenGL 3.
	
	
	
### Step 3: Modifying the Rendering Loop
	
a. Start the ImGui Frame

Before we start rendering, we need to start the ImGui frame, to do so add the imgui_display function which is given below in the code
and call it inside game loop before display()

	void imgui_display(void)
	{
		// imgui window
		ImGui_ImplWin32_NewFrame();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();    
		
		ImGui::Begin("PerVertex Color Sliders");
		{
			ImGui::SliderFloat3("Top Vertex", topVertexColor, 0.0f, 1.0f);
			ImGui::SliderFloat3("Left Bottom Vertex", bottomLeftVertexColor, 0.0f, 1.0f);
			ImGui::SliderFloat3("Right Bottom Vertex", bottomRightVertexColor, 0.0f, 1.0f);
		}
		ImGui::End();
	}

Explanation:

ImGui_ImplOpenGL3_NewFrame() and ImGui_ImplWin32_NewFrame() prepare ImGui for a new frame in OpenGL and Win32 contexts.
ImGui::NewFrame() begins a new ImGui frame.
ImGui::Begin("My ImGui Window"); opens a new ImGui window.
ImGui::Text("Hello, world!"); adds text inside the ImGui window.
ImGui::End(); closes the ImGui window.
	
	
b. Render ImGui:

Add below code in display After rendering code
	
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
Explanation:
	
ImGui::Render(); finalizes the ImGui frame.
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); renders the ImGui frame using OpenGL.	
	
### Step 4: Cleanup
Shutdown ImGui on Program Exit

Before the application exits, ensure that ImGui is properly shut down:
to do so call 3 function given below in uninitialize function
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	
Explanation:

ImGui_ImplOpenGL3_Shutdown(); cleans up the OpenGL3 implementation.
ImGui_ImplWin32_Shutdown(); cleans up the Win32 implementation.
ImGui::DestroyContext(); destroys the ImGui context, freeing up resources.
	
	
### Step 5: Compile and Run

Build the Project
	
build.bat file should look like:
	
	cls

	cl.exe /c /EHsc /I "C:\\glew-2.1.0\\include" OGL.cpp ^
	imGUI/imgui.cpp ^
	imGUI/imgui_draw.cpp ^
	imGUI/imgui_impl_opengl3.cpp ^
	imGUI/imgui_impl_win32.cpp ^
	imGUI/imgui_tables.cpp ^
	imGUI/imgui_widgets.cpp

	rc.exe OGL.rc
	link.exe ^
	OGL.obj ^
	OGL.res ^
	imgui.obj ^
	imgui_draw.obj ^
	imgui_impl_opengl3.obj ^
	imgui_impl_win32.obj ^
	imgui_tables.obj ^
	imgui_widgets.obj ^
	User32.lib ^
	GDI32.lib ^
	/LIBPATH:"C:\\glew-2.1.0\\lib\\Release\\x64" /SUBSYSTEM:WINDOWS
	
Run the Application and should see the ImGui interface overlaying.
