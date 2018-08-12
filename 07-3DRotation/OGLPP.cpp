#include <windows.h>
#include <stdio.h>

#include <gl\glew.h>

#include <gl/GL.h>

#include "pmath.h"

#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

enum
{
	VDG_ATTRIBUTE_VERTEX = 0,
	VDG_ATTRIBUTE_COLOR,
	VDG_ATTRIBUTE_NORMAL,
	VDG_ATTRIBUTE_TEXTURE0,
};

//Prototype Of WndProc() declared Globally
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Global variable declarations
FILE *gpFile = NULL;

HWND ghwnd = NULL;
HDC ghdc = NULL;
HGLRC ghrc = NULL;

DWORD dwStyle;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

bool gbActiveWindow = false;
bool gbEscapeKeyIsPressed = false;
bool gbFullscreen = false;

GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gVao_pyramid;
GLuint gVbo_pyramid_position;
GLuint gVbo_pyramid_color;

GLuint gVao_cube;
GLuint gVbo_cube_position;
GLuint gVbo_cube_color;

GLuint gMVPUniform;

float gPerspectiveProjectionMatrix[16];

GLfloat gAnglePyramid = 0.0f; 
GLfloat gAngleCube = 0.0f; 

//main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	//function prototype
	void initialize(void);
	void uninitialize(void);
	void display(void);
	void spin(void);

	//variable declaration
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[] = TEXT("OpenGLPP");
	bool bDone = false;

	//code
	if (fopen_s(&gpFile, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("Log File Can Not Be Created\nExitting ..."), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
		exit(0);
	}
	else
	{
		fprintf(gpFile, "Log File Is Successfully Opened.\n");
	}

	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.lpfnWndProc = WndProc;
	wndclass.lpszClassName = szClassName;
	wndclass.lpszMenuName = NULL;

	RegisterClassEx(&wndclass);

	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szClassName,
		TEXT("OpenGL Programmable Pipeline : 3D Rotation"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		100,
		100,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	initialize();

	while (bDone == false) 
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				bDone = true;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			display();
			spin();

			if (gbActiveWindow == true)
			{
				if (gbEscapeKeyIsPressed == true) 
					bDone = true;
			}
		}
	}

	uninitialize();

	return((int)msg.wParam);
}

//WndProc()
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	//function prototype
	void resize(int, int);
	void ToggleFullscreen(void);
	void uninitialize(void);

	//code
	switch (iMsg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0) 
			gbActiveWindow = true;
		else 
			gbActiveWindow = false;
		break;
	case WM_ERASEBKGND:
		return(0);
	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam)); 
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE: 
			gbEscapeKeyIsPressed = true; 
			break;
		case 0x46: 
			if (gbFullscreen == false)
			{
				ToggleFullscreen();
				gbFullscreen = true;
			}
			else
			{
				ToggleFullscreen();
				gbFullscreen = false;
			}
			break;
		default:
			break;
		}
		break;
	case WM_LBUTTONDOWN: 
		break;
	case WM_CLOSE: 
		uninitialize();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}

void ToggleFullscreen(void)
{
	//variable declarations
	MONITORINFO mi;

	//code
	if (gbFullscreen == false)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			mi = { sizeof(MONITORINFO) };
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
	}

	else
	{
		//code
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);
	}
}

void initialize(void)
{
	//function prototypes
	void uninitialize(void);
	void resize(int, int);

	//variable declarations
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	//code
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
	pfd.cDepthBits = 32;

	ghdc = GetDC(ghwnd);

	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
	if (iPixelFormatIndex == 0)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == false)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	ghrc = wglCreateContext(ghdc);
	if (ghrc == NULL)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	if (wglMakeCurrent(ghdc, ghrc) == false)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	//VERTEX SHADER
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	const GLchar *vertexShaderSourceCode =
		"#version 450 core" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec4 vColor;" \
		"out vec4 out_color;" \
		"uniform mat4 u_mvp_matrix;" \
		"void main(void)" \
		"{" \
		"gl_Position = u_mvp_matrix * vPosition;" \
		"out_color=vColor;"
		"}";

	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

	glCompileShader(gVertexShaderObject);
	GLint iInfoLogLength = 0;
	GLint iShaderCompiledStatus = 0;
	char *szInfoLog = NULL;
	glGetShaderiv(gVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Vertex Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	//FRAGMENT SHADER
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar *fragmentShaderSourceCode =
		"#version 450 core" \
		"\n" \
		"in vec4 out_color;" \
		"out vec4 FragColor;" \
		"void main(void)" \
		"{" \
		"FragColor = out_color;" \
		"}";

	glShaderSource(gFragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	glCompileShader(gFragmentShaderObject);
	glGetShaderiv(gFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompiledStatus);
	if (iShaderCompiledStatus == GL_FALSE)
	{
		glGetShaderiv(gFragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gFragmentShaderObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Fragment Shader Compilation Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	//SHADER PROGRAM
	gShaderProgramObject = glCreateProgram();

	glAttachShader(gShaderProgramObject, gVertexShaderObject);

	glAttachShader(gShaderProgramObject, gFragmentShaderObject);

	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_COLOR, "vColor");

	glLinkProgram(gShaderProgramObject);
	GLint iShaderProgramLinkStatus = 0;
	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iShaderProgramLinkStatus);
	if (iShaderProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength>0)
		{
			szInfoLog = (char *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpFile, "Shader Program Link Log : %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	gMVPUniform = glGetUniformLocation(gShaderProgramObject, "u_mvp_matrix");

	const GLfloat pyramidVertices[] =
	{
		0, 1, 0,  
		-1, -1, 1, 
		1, -1, 1,  

		0, 1, 0,   
		1, -1, 1,  
		1, -1, -1, 

		0, 1, 0,   
		1, -1, -1, 
		-1, -1, -1,

		0, 1, 0,    
		-1, -1, -1, 
		-1, -1, 1   
	};

	const GLfloat pyramidColors[] =
	{
		1, 0, 0,    
		0, 1, 0,    
		0, 0, 1,    

		1, 0, 0,    
		0, 0, 1,    
		0, 1, 0,    

		1, 0, 0,    
		0, 1, 0,    
		0, 0, 1,    

		1, 0, 0,    
		0, 0, 1,    
		0, 1, 0,    
	};

	GLfloat cubeVertices[] =
	{
		1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,  

		1.0f, -1.0f, 1.0f, 
		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, 
		1.0f, -1.0f, -1.0f,  

		1.0f, 1.0f, 1.0f,  
		-1.0f, 1.0f, 1.0f, 
		-1.0f, -1.0f, 1.0f, 
		1.0f, -1.0f, 1.0f,  

		1.0f, -1.0f, -1.0f,  
		-1.0f, -1.0f, -1.0f, 
		-1.0f, 1.0f, -1.0f, 
		1.0f, 1.0f, -1.0f,  

		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 
		-1.0f, -1.0f, -1.0f, 
		-1.0f, -1.0f, 1.0f, 

		1.0f, 1.0f, -1.0f,  
		1.0f, 1.0f, 1.0f, 
		1.0f, -1.0f, 1.0f,  
		1.0f, -1.0f, -1.0f, 
	};

	const GLfloat cubeColors[] =
	{
		0.0f, 1.0f, 0.0f,   
		0.0f, 1.0f, 0.0f,    
		0.0f, 1.0f, 0.0f,    
		0.0f, 1.0f, 0.0f,   

		1.0f, 0.5f, 0.0f,    
		1.0f, 0.5f, 0.0f,   
		1.0f, 0.5f, 0.0f,    
		1.0f, 0.5f, 0.0f,    

		1.0f, 0.0f, 0.0f,   
		1.0f, 0.0f, 0.0f,   
		1.0f, 0.0f, 0.0f,    
		1.0f, 0.0f, 0.0f,    

		1.0f, 1.0f, 0.0f,    
		1.0f, 1.0f, 0.0f,   
		1.0f, 1.0f, 0.0f,    
		1.0f, 1.0f, 0.0f,    

		0.0f, 0.0f, 1.0f,    
		0.0f, 0.0f, 1.0f,    
		0.0f, 0.0f, 1.0f,    
		0.0f, 0.0f, 1.0f,   

		1.0f, 0.0f, 1.0f,   
		1.0f, 0.0f, 1.0f,    
		1.0f, 0.0f, 1.0f,    
		1.0f, 0.0f, 1.0f,   
	};

	for (int i = 0; i < 72; i++)
	{
		if (cubeVertices[i] > 0.0f)
			cubeVertices[i] = cubeVertices[i] - 0.25f;
		else if (cubeVertices[i] < 0.0f)
			cubeVertices[i] = cubeVertices[i] + 0.25f;
		else
			cubeVertices[i] = cubeVertices[i];
	}

	// PYRAMID CODE
	glGenVertexArrays(1, &gVao_pyramid);
	glBindVertexArray(gVao_pyramid);

	glGenBuffers(1, &gVbo_pyramid_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_pyramid_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &gVbo_pyramid_color);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_pyramid_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidColors), pyramidColors, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

	glEnableVertexAttribArray(VDG_ATTRIBUTE_COLOR);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	// CUBE CODE
	glGenVertexArrays(1, &gVao_cube);
	glBindVertexArray(gVao_cube);

	glGenBuffers(1, &gVbo_cube_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &gVbo_cube_color);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

	glEnableVertexAttribArray(VDG_ATTRIBUTE_COLOR);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 

	myIdentity(gPerspectiveProjectionMatrix);

	resize(WIN_WIDTH, WIN_HEIGHT);
}

void display(void)
{
	//code
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(gShaderProgramObject);

	float modelViewMatrix[16];
	float rotationMatrix[16];
	float modelViewProjectionMatrix[16];

	// OpenGL Drawing

	// PYRAMID CODE
	myIdentity(modelViewMatrix);
	
	myIdentity(rotationMatrix);

	myTranslate(modelViewMatrix , -1.5f, 0.0f, -6.0f);

	myRotate(rotationMatrix, gAnglePyramid, 0.0f, 1.0f, 0.0f); 

	multiplyMatrices1(rotationMatrix, modelViewMatrix); 

	multiplyMatrices2(modelViewProjectionMatrix, modelViewMatrix, gPerspectiveProjectionMatrix); 

	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelViewProjectionMatrix);

	glBindVertexArray(gVao_pyramid);

	glDrawArrays(GL_TRIANGLES, 0, 12); 

	glBindVertexArray(0);

	// CUBE CODE
	myIdentity(modelViewMatrix);

	myIdentity(rotationMatrix);

	myTranslate(modelViewMatrix, 1.5f, 0.0f, -6.0f);

	myRotateTriaxial(rotationMatrix, gAngleCube, gAngleCube, gAngleCube); 

	multiplyMatrices1(rotationMatrix, modelViewMatrix); 

	multiplyMatrices2(modelViewProjectionMatrix, modelViewMatrix, gPerspectiveProjectionMatrix); 

	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelViewProjectionMatrix);

	glBindVertexArray(gVao_cube);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	glBindVertexArray(0);

	glUseProgram(0);

	SwapBuffers(ghdc);
}

void resize(int width, int height)
{
	//code
	if (height == 0)
		height = 1;
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	myPerspective(gPerspectiveProjectionMatrix, 45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void spin(void)
{
	// code
	gAnglePyramid = gAnglePyramid + 1.0f;
	if (gAnglePyramid >= 360.0f)
		gAnglePyramid = gAnglePyramid - 360.0f;

	gAngleCube = gAngleCube + 1.0f;
	if (gAngleCube >= 360.0f)
		gAngleCube = gAngleCube - 360.0f;
}

void uninitialize(void)
{
	//code
	if (gbFullscreen == true)
	{
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd, &wpPrev);
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowCursor(TRUE);

	}

	// PYRAMID
	if (gVao_pyramid)
	{
		glDeleteVertexArrays(1, &gVao_pyramid);
		gVao_pyramid = 0;
	}

	if (gVbo_pyramid_position)
	{
		glDeleteBuffers(1, &gVbo_pyramid_position);
		gVbo_pyramid_position = 0;
	}

	if (gVbo_pyramid_color)
	{
		glDeleteBuffers(1, &gVbo_pyramid_color);
		gVbo_pyramid_color = 0;
	}

	// CUBE
	if (gVao_cube)
	{
		glDeleteVertexArrays(1, &gVao_cube);
		gVao_cube = 0;
	}

	if (gVbo_cube_position)
	{
		glDeleteBuffers(1, &gVbo_cube_position);
		gVbo_cube_position = 0;
	}

	if (gVbo_cube_color)
	{
		glDeleteBuffers(1, &gVbo_cube_color);
		gVbo_cube_color = 0;
	}

	glDetachShader(gShaderProgramObject, gVertexShaderObject);
	glDetachShader(gShaderProgramObject, gFragmentShaderObject);

	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject = 0;
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject = 0;

	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject = 0;

	glUseProgram(0);

	wglMakeCurrent(NULL, NULL);

	wglDeleteContext(ghrc);
	ghrc = NULL;

	ReleaseDC(ghwnd, ghdc);
	ghdc = NULL;

	if (gpFile)
	{
		fprintf(gpFile, "Log File Is Successfully Closed.\n");
		fclose(gpFile);
		gpFile = NULL;
	}
}
