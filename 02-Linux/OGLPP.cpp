//headers
#include <iostream>
#include <stdio.h> //for printf()
#include <stdlib.h> //for exit()
#include <memory.h> //for memset()

//headers for XServer
#include <X11/Xlib.h> //analogous to windows.h
#include <X11/Xutil.h> //for visuals
#include <X11/XKBlib.h> //XkbKeycodeToKeysym()
#include <X11/keysym.h> //for 'Keysym'

#include <GL/glew.h> // for GLSL extensions IMPORTANT : This Line Should Be Before #include<gl\gl.h>

#include <GL/gl.h>
#include <GL/glx.h> //for 'glx' functions

#include "pmath.h"

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

enum
{
	VDG_ATTRIBUTE_VERTEX = 0,
	VDG_ATTRIBUTE_COLOR,
	VDG_ATTRIBUTE_NORMAL,
	VDG_ATTRIBUTE_TEXTURE0,
};

//global variable declarations
FILE *gpFile = NULL;

Display *gpDisplay=NULL;
XVisualInfo *gpXVisualInfo=NULL;
Colormap gColormap;
Window gWindow;
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB=NULL;
GLXFBConfig gGLXFBConfig;
GLXContext gGLXContext; //parallel to HGLRC

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

GLfloat gAnglePyramid = 0.0f; //angle of rotation of Trinagle
GLfloat gAngleCube = 0.0f; //angle of rotation for Square

//entry-point function
int main(int argc, char *argv[])
{
	//function prototype
	void CreateWindow(void);
	void ToggleFullscreen(void);
	void initialize(void);
	void resize(int,int);
	void display(void);
	void spin(void);
	void uninitialize(void);
	
	//code
	// create log file
	gpFile=fopen("Log.txt", "w");
	if (gpFile==NULL)
	{
		printf("Log File Can Not Be Created. EXitting Now ...\n");
		exit(0);
	}
	else
	{
		fprintf(gpFile, "Log File Is Successfully Opened.\n");
	}
	
	// create the window
	CreateWindow();
	
	//initialize()
	initialize();
	
	//Message Loop

	//variable declarations
	XEvent event; //parallel to 'MSG' structure
	KeySym keySym;
	int winWidth;
	int winHeight;
	bool bDone=false;
	
	while(bDone==false)
	{
		while(XPending(gpDisplay))
		{
			XNextEvent(gpDisplay,&event); //parallel to GetMessage()
			switch(event.type) //parallel to 'iMsg'
			{
				case MapNotify: //parallel to WM_CREATE
					break;
				case KeyPress: //parallel to WM_KEYDOWN
					keySym=XkbKeycodeToKeysym(gpDisplay,event.xkey.keycode,0,0);
					switch(keySym)
					{
						case XK_Escape:
							bDone=true;
							break;
						case XK_F:
						case XK_f:
							if(gbFullscreen==false)
							{
								ToggleFullscreen();
								gbFullscreen=true;
							}
							else
							{
								ToggleFullscreen();
								gbFullscreen=false;
							}
							break;
						default:
							break;
					}
					break;
				case ButtonPress:
					switch(event.xbutton.button)
					{
						case 1: //Left Button
							break;
						case 2: //Middle Button
							break;
						case 3: //Right Button
							break;
						default: 
							break;
					}
					break;
				case MotionNotify: //parallel to WM_MOUSEMOVE
					break;
				case ConfigureNotify: //parallel to WM_SIZE
					winWidth=event.xconfigure.width;
					winHeight=event.xconfigure.height;
					resize(winWidth,winHeight);
					break;
				case Expose: //parallel to WM_PAINT
					break;
				case DestroyNotify:
					break;
				case 33: //close button, system menu -> close
					bDone=true;
					break;
				default:
					break;
			}
		}
		
		display();
		spin();
	}
	
	uninitialize();
	return(0);
}

void CreateWindow(void)
{
	//function prototype
	void uninitialize(void);
	
	//variable declarations
	XSetWindowAttributes winAttribs;
	GLXFBConfig *pGLXFBConfigs=NULL;
	GLXFBConfig bestGLXFBConfig;
	XVisualInfo *pTempXVisualInfo=NULL;
	int iNumFBConfigs=0;
	int styleMask;
	int i;
	
	static int frameBufferAttributes[]={
		GLX_X_RENDERABLE,True,
		GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
		GLX_RENDER_TYPE,GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
		GLX_RED_SIZE,8,
		GLX_GREEN_SIZE,8,
		GLX_BLUE_SIZE,8,
		GLX_ALPHA_SIZE,8,
		GLX_DEPTH_SIZE,24,
		GLX_STENCIL_SIZE,8,
		GLX_DOUBLEBUFFER,True,
		//GLX_SAMPLE_BUFFERS,1,
		//GLX_SAMPLES,4,
		None}; // array must be terminated by 0
	
	//code
	gpDisplay=XOpenDisplay(NULL);
	if(gpDisplay==NULL)
	{
		printf("ERROR : Unable To Obtain X Display.\n");
		uninitialize();
		exit(1);
	}
	
	// get a new framebuffer config that meets our attrib requirements
	pGLXFBConfigs=glXChooseFBConfig(gpDisplay,DefaultScreen(gpDisplay),frameBufferAttributes,&iNumFBConfigs);
	if(pGLXFBConfigs==NULL)
	{
		printf( "Failed To Get Valid Framebuffer Config. Exitting Now ...\n");
		uninitialize();
		exit(1);
	}
	printf("%d Matching FB Configs Found.\n",iNumFBConfigs);
	
	// pick that FB config/visual with the most samples per pixel
	int bestFramebufferconfig=-1,worstFramebufferConfig=-1,bestNumberOfSamples=-1,worstNumberOfSamples=999;
	for(i=0;i<iNumFBConfigs;i++)
	{
		pTempXVisualInfo=glXGetVisualFromFBConfig(gpDisplay,pGLXFBConfigs[i]);
		if(pTempXVisualInfo)
		{
			int sampleBuffer,samples;
			glXGetFBConfigAttrib(gpDisplay,pGLXFBConfigs[i],GLX_SAMPLE_BUFFERS,&sampleBuffer);
			glXGetFBConfigAttrib(gpDisplay,pGLXFBConfigs[i],GLX_SAMPLES,&samples);
			printf("Matching Framebuffer Config=%d : Visual ID=0x%lu : SAMPLE_BUFFERS=%d : SAMPLES=%d\n",i,pTempXVisualInfo->visualid,sampleBuffer,samples);
			if(bestFramebufferconfig < 0 || sampleBuffer && samples > bestNumberOfSamples)
			{
				bestFramebufferconfig=i;
				bestNumberOfSamples=samples;
			}
			if( worstFramebufferConfig < 0 || !sampleBuffer || samples < worstNumberOfSamples)
			{
				worstFramebufferConfig=i;
			    worstNumberOfSamples=samples;
			}
		}
		XFree(pTempXVisualInfo);
	}
	bestGLXFBConfig = pGLXFBConfigs[bestFramebufferconfig];
	// set global GLXFBConfig
	gGLXFBConfig=bestGLXFBConfig;
	
	// be sure to free FBConfig list allocated by glXChooseFBConfig()
	XFree(pGLXFBConfigs);
	
	gpXVisualInfo=glXGetVisualFromFBConfig(gpDisplay,bestGLXFBConfig);
	printf("Chosen Visual ID=0x%lu\n",gpXVisualInfo->visualid );
	
	//setting window's attributes
	winAttribs.border_pixel=0;
	winAttribs.background_pixmap=0;
	winAttribs.colormap=XCreateColormap(gpDisplay,
										RootWindow(gpDisplay,gpXVisualInfo->screen), //you can give defaultScreen as well
										gpXVisualInfo->visual,
										AllocNone); //for 'movable' memory allocation
										
	winAttribs.event_mask=StructureNotifyMask | KeyPressMask | ButtonPressMask |
						  ExposureMask | VisibilityChangeMask | PointerMotionMask;
	
	styleMask=CWBorderPixel | CWEventMask | CWColormap;
	gColormap=winAttribs.colormap;										           
	
	gWindow=XCreateWindow(gpDisplay,
						  RootWindow(gpDisplay,gpXVisualInfo->screen),
						  0,
						  0,
						  WIN_WIDTH,
						  WIN_HEIGHT,
						  0, //border width
						  gpXVisualInfo->depth, //depth of visual (depth for Colormap)          
						  InputOutput, //class(type) of your window
						  gpXVisualInfo->visual,
						  styleMask,
						  &winAttribs);
	if(!gWindow)
	{
		printf("Failure In Window Creation.\n");
		uninitialize();
		exit(1);
	}
	
	XStoreName(gpDisplay,gWindow,"OpenGL Programmable Pipeline Window");
	
	Atom windowManagerDelete=XInternAtom(gpDisplay,"WM_WINDOW_DELETE",True);
	XSetWMProtocols(gpDisplay,gWindow,&windowManagerDelete,1);
	
	XMapWindow(gpDisplay,gWindow);
}

void ToggleFullscreen(void)
{
	//code
	Atom wm_state=XInternAtom(gpDisplay,"_NET_WM_STATE",False); //normal window state
	
	XEvent event;
	memset(&event,0,sizeof(XEvent));
	
	event.type=ClientMessage;
	event.xclient.window=gWindow;
	event.xclient.message_type=wm_state;
	event.xclient.format=32; //32-bit
	event.xclient.data.l[0]=gbFullscreen ? 0 : 1;

	Atom fullscreen=XInternAtom(gpDisplay,"_NET_WM_STATE_FULLSCREEN",False);	
	event.xclient.data.l[1]=fullscreen;
	
	//parallel to SendMessage()
	XSendEvent(gpDisplay,
			   RootWindow(gpDisplay,gpXVisualInfo->screen),
			   False, //do not send this message to Sibling windows
			   StructureNotifyMask, //resizing mask (event_mask)
			   &event);	
}

void initialize(void)
{
	// function declarations
	void uninitialize(void);
	void resize(int,int);
	
	//code
	// create a new GL context 4.5 for rendering
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((GLubyte *)"glXCreateContextAttribsARB");
	
	GLint attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB,4,
		GLX_CONTEXT_MINOR_VERSION_ARB,5,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0 }; // array must be terminated by 0
		
	gGLXContext = glXCreateContextAttribsARB(gpDisplay,gGLXFBConfig,0,True,attribs);

	if(!gGLXContext) // fallback to safe old style 2.x context
	{
		// When a context version below 3.0 is requested, implementations will return 
		// the newest context version compatible with OpenGL versions less than version 3.0.
		GLint attribs[] = {
			GLX_CONTEXT_MAJOR_VERSION_ARB,1,
			GLX_CONTEXT_MINOR_VERSION_ARB,0,
			0 }; // array must be terminated by 0
		printf("Failed To Create GLX 4.5 context. Hence Using Old-Style GLX Context\n");
		gGLXContext = glXCreateContextAttribsARB(gpDisplay,gGLXFBConfig,0,True,attribs);
	}
	else // successfully created 4.1 context
	{
		printf("OpenGL Context 4.5 Is Created.\n");
	}
	
	// verifying that context is a direct context
	if(!glXIsDirect(gpDisplay,gGLXContext))
	{
		printf("Indirect GLX Rendering Context Obtained\n");
	}
	else
	{
		printf("Direct GLX Rendering Context Obtained\n" );
	}
	
	glXMakeCurrent(gpDisplay,gWindow,gGLXContext);
	
	//code
	// GLEW Initialization Code For GLSL ( IMPORTANT : It Must Be Here. Means After Creating OpenGL Context But Before Using Any OpenGL Function )
	GLenum glew_error = glewInit();
	if (glew_error != GLEW_OK)
	{
		printf("Failure To Initialize GLEW. Exitting Now ...\n");
		uninitialize();
		exit(1);
	}

	// *** VERTEX SHADER ***
	// create shader
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	// provide source code to shader
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

	// compile shader
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

	// *** FRAGMENT SHADER ***
	// create shader
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	// provide source code to shader
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

	// compile shader
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

	// *** SHADER PROGRAM ***
	// create
	gShaderProgramObject = glCreateProgram();

	// attach vertex shader to shader program
	glAttachShader(gShaderProgramObject, gVertexShaderObject);

	// attach fragment shader to shader program
	glAttachShader(gShaderProgramObject, gFragmentShaderObject);

	// pre-link binding of shader program object with vertex shader position attribute
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_VERTEX, "vPosition");
	// pre-link binding of shader program object with vertex shader color attribute
	glBindAttribLocation(gShaderProgramObject, VDG_ATTRIBUTE_COLOR, "vColor");
	
	// link shader
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

	// get MVP uniform location
	gMVPUniform = glGetUniformLocation(gShaderProgramObject, "u_mvp_matrix");

	// *** vertices, colors, shader attribs, vbo, vao initializations ***
	const GLfloat pyramidVertices[] =
	{
		0, 1, 0,    // front-top
		-1, -1, 1,  // front-left
		1, -1, 1,   // front-right

		0, 1, 0,    // right-top
		1, -1, 1,   // right-left
		1, -1, -1,  // right-right

		0, 1, 0,    // back-top
		1, -1, -1,  // back-left
		-1, -1, -1, // back-right

		0, 1, 0,    // left-top
		-1, -1, -1, // left-left
		-1, -1, 1   // left-right
	};

	const GLfloat pyramidColors[] =
	{
		1, 0, 0,    // red
		0, 1, 0,    // green
		0, 0, 1,    // blue

		1, 0, 0,    // red
		0, 0, 1,    // blue
		0, 1, 0,    // green

		1, 0, 0,    // red
		0, 1, 0,    // green
		0, 0, 1,    // blue

		1, 0, 0,    // red
		0, 0, 1,    // blue
		0, 1, 0,    // green
	};

	// separated two arrays of cube according to above mixed array
	GLfloat cubeVertices[] =
	{
		// top surface
		1.0f, 1.0f, -1.0f,  // top-right of top
		-1.0f, 1.0f, -1.0f, // top-left of top
		-1.0f, 1.0f, 1.0f, // bottom-left of top
		1.0f, 1.0f, 1.0f,  // bottom-right of top

		// bottom surface
		1.0f, -1.0f, 1.0f,  // top-right of bottom
		-1.0f, -1.0f, 1.0f, // top-left of bottom
		-1.0f, -1.0f, -1.0f, // bottom-left of bottom
		1.0f, -1.0f, -1.0f,  // bottom-right of bottom

		// front surface
		1.0f, 1.0f, 1.0f,  // top-right of front
		-1.0f, 1.0f, 1.0f, // top-left of front
		-1.0f, -1.0f, 1.0f, // bottom-left of front
		1.0f, -1.0f, 1.0f,  // bottom-right of front

		// back surface
		1.0f, -1.0f, -1.0f,  // top-right of back
		-1.0f, -1.0f, -1.0f, // top-left of back
		-1.0f, 1.0f, -1.0f, // bottom-left of back
		1.0f, 1.0f, -1.0f,  // bottom-right of back

		// left surface
		-1.0f, 1.0f, 1.0f, // top-right of left
		-1.0f, 1.0f, -1.0f, // top-left of left
		-1.0f, -1.0f, -1.0f, // bottom-left of left
		-1.0f, -1.0f, 1.0f, // bottom-right of left

		// right surface
		1.0f, 1.0f, -1.0f,  // top-right of right
		1.0f, 1.0f, 1.0f,  // top-left of right
		1.0f, -1.0f, 1.0f,  // bottom-left of right
		1.0f, -1.0f, -1.0f,  // bottom-right of right
	};

	// If above -1.0f Or +1.0f Values Make Cube Much Larger Than Pyramid,
	// then follow the code in following loop which will convertt all 1s And -1s to -0.75 or +0.75
	for (int i = 0; i<72; i++)
	{
		if (cubeVertices[i]<0.0f)
			cubeVertices[i] = cubeVertices[i] + 0.25f;
		else if (cubeVertices[i]>0.0f)
			cubeVertices[i] = cubeVertices[i] - 0.25f;
		else
			cubeVertices[i] = cubeVertices[i]; // no change
	}

	const GLfloat cubeColors[] =
	{
		// top surface
		0.0f, 1.0f, 0.0f,    // green color
		0.0f, 1.0f, 0.0f,    // green color
		0.0f, 1.0f, 0.0f,    // green color
		0.0f, 1.0f, 0.0f,    // green color

		// bottom surface
		1.0f, 0.5f, 0.0f,    // orange color
		1.0f, 0.5f, 0.0f,    // orange color
		1.0f, 0.5f, 0.0f,    // orange color
		1.0f, 0.5f, 0.0f,    // orange color

		// front surface
		1.0f, 0.0f, 0.0f,    // red color
		1.0f, 0.0f, 0.0f,    // red color
		1.0f, 0.0f, 0.0f,    // red color
		1.0f, 0.0f, 0.0f,    // red color

		// back surface
		1.0f, 1.0f, 0.0f,    // yellow color
		1.0f, 1.0f, 0.0f,    // yellow color
		1.0f, 1.0f, 0.0f,    // yellow color
		1.0f, 1.0f, 0.0f,    // yellow color

		// left surface
		0.0f, 0.0f, 1.0f,    // blue color
		0.0f, 0.0f, 1.0f,    // blue color
		0.0f, 0.0f, 1.0f,    // blue color
		0.0f, 0.0f, 1.0f,    // blue color

		// right surface
		1.0f, 0.0f, 1.0f,    // violet color
		1.0f, 0.0f, 1.0f,    // violet color
		1.0f, 0.0f, 1.0f,    // violet color
		1.0f, 0.0f, 1.0f,    // violet color
	};

	// PYRAMID CODE
	glGenVertexArrays(1, &gVao_pyramid);
	glBindVertexArray(gVao_pyramid);

	// vbo for position
	glGenBuffers(1, &gVbo_pyramid_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_pyramid_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVertices), pyramidVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for x,y,z in triangleVertices array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// vbo for color
	glGenBuffers(1, &gVbo_pyramid_color);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_pyramid_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidColors), pyramidColors, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for r,g,b in triangleColors array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_COLOR);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	// ==================

	// CUBE CODE
	glGenVertexArrays(1, &gVao_cube);
	glBindVertexArray(gVao_cube);

	// vbo for position
	glGenBuffers(1, &gVbo_cube_position);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_position);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for x,y,z in cubeVertices array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_VERTEX);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// vbo for color
	glGenBuffers(1, &gVbo_cube_color);
	glBindBuffer(GL_ARRAY_BUFFER, gVbo_cube_color);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);

	glVertexAttribPointer(VDG_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL); // 3 is for r,g,b in cubeColors array

	glEnableVertexAttribArray(VDG_ATTRIBUTE_COLOR);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
	// ==================
	
	glShadeModel(GL_SMOOTH);
	// set-up depth buffer
	glClearDepth(1.0f);
	// enable depth testing
	glEnable(GL_DEPTH_TEST);
	// depth test to do
	glDepthFunc(GL_LEQUAL);
	// set really nice percpective calculations ?
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	// We will always cull back faces for better performance
	glEnable(GL_CULL_FACE);

	// set background clearing color
	glClearColor(0.0f,0.0f,0.0f,0.0f); // black
	
	// set perspective matrix to identitu matrix
	myIdentity(gPerspectiveProjectionMatrix);
	
	// resize
	resize(WIN_WIDTH, WIN_HEIGHT);
}

void resize(int width,int height)
{
    //code
	if(height==0)
		height=1;
		
	glViewport(0,0,(GLsizei)width,(GLsizei)height);

	myPerspective(gPerspectiveProjectionMatrix, 45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void display(void)
{
	//code
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// start using OpenGL program object
	glUseProgram(gShaderProgramObject);

	float modelViewMatrix[16];
	float rotationMatrix[16];
	float modelViewProjectionMatrix[16];

	// OpenGL Drawing
	// PYRAMID CODE
	// set modelview matrix to identity matrix
	myIdentity(modelViewMatrix);

	// apply z axis translation to go deep into the screen by -6.0,
	// so that triangle with same fullscreen co-ordinates, but due to above translation will look small
	myTranslate(modelViewMatrix, -1.5f, 0.0f, -6.0f);

	myRotate(rotationMatrix, gAnglePyramid, 0.0f, 1.0f, 0.0f); // Y axis rotation by gAnglePyramid angle

	multiplyMatrices1(rotationMatrix, modelViewMatrix); // ORDER IS IMPORTANT

	// multiply the modelview and projection matrix to get modelviewprojection matrix
	multiplyMatrices2(modelViewProjectionMatrix, modelViewMatrix, gPerspectiveProjectionMatrix); // ORDER IS IMPORTANT

	// pass above modelviewprojection matrix to the vertex shader in 'u_mvp_matrix' shader variable
	// whose position value we already calculated in initWithFrame() by using glGetUniformLocation()
	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelViewProjectionMatrix);

	// *** bind vao ***
	glBindVertexArray(gVao_pyramid);

	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	glDrawArrays(GL_TRIANGLES, 0, 12); // // 3 (each with its x,y,z ) vertices for 4 faces of pyramid

	// *** unbind vao ***
	glBindVertexArray(0);
	// ==================

	// CUBE CODE
	// set modelview matrix to identity matrix
	myIdentity(modelViewMatrix);

	// apply z axis translation to go deep into the screen by -5.0,
	// so that triangle with same fullscreen co-ordinates, but due to above translation will look small
	myTranslate(modelViewMatrix, 1.5f, 0.0f, -6.0f);

	// *** This is a special polymorphic version of rotate() for TRIAXIAL rotation ***
	myRotateTriaxial(rotationMatrix, gAngleCube, gAngleCube, gAngleCube);// All axes rotation by gAngleCube angle

	multiplyMatrices1(rotationMatrix, modelViewMatrix); // ORDER IS IMPORTANT

	// multiply the modelview and projection matrix to get modelviewprojection matrix
	multiplyMatrices2(modelViewProjectionMatrix, modelViewMatrix, gPerspectiveProjectionMatrix); // ORDER IS IMPORTANT

	// pass above modelviewprojection matrix to the vertex shader in 'u_mvp_matrix' shader variable
	// whose position value we already calculated in initWithFrame() by using glGetUniformLocation()
	glUniformMatrix4fv(gMVPUniform, 1, GL_FALSE, modelViewProjectionMatrix);

	// *** bind vao ***
	glBindVertexArray(gVao_cube);

	// *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
	// actually 2 triangles make 1 square, so there should be 6 vertices,
	// but as 2 tringles while making square meet each other at diagonal,
	// 2 of 6 vertices are common to both triangles, and hence 6-2=4
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	// *** unbind vao ***
	glBindVertexArray(0);

	// stop using OpenGL program object
	glUseProgram(0);
	
	//to process buffered OpenGL Routines
	glXSwapBuffers(gpDisplay,gWindow);
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
	// PYRAMID
	// destroy vao
	if (gVao_pyramid)
	{
		glDeleteVertexArrays(1, &gVao_pyramid);
		gVao_pyramid = 0;
	}

	// destroy vbo position
	if (gVbo_pyramid_position)
	{
		glDeleteBuffers(1, &gVbo_pyramid_position);
		gVbo_pyramid_position = 0;
	}

	// destroy vbo color
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

	// destroy vbo position
	if (gVbo_cube_position)
	{
		glDeleteBuffers(1, &gVbo_cube_position);
		gVbo_cube_position = 0;
	}

	// destroy vbo color
	if (gVbo_cube_color)
	{
		glDeleteBuffers(1, &gVbo_cube_color);
		gVbo_cube_color = 0;
	}
	
	// detach vertex shader from shader program object
	glDetachShader(gShaderProgramObject, gVertexShaderObject);
	// detach fragment  shader from shader program object
	glDetachShader(gShaderProgramObject, gFragmentShaderObject);

	// delete vertex shader object
	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject = 0;
	// delete fragment shader object
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject = 0;

	// delete shader program object
	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject = 0;

	// unlink shader program
	glUseProgram(0);

	// Releasing OpenGL related and XWindow related objects 	
	GLXContext currentContext=glXGetCurrentContext();
	if(currentContext!=NULL && currentContext==gGLXContext)
	{
		glXMakeCurrent(gpDisplay,0,0);
	}
	
	if(gGLXContext)
	{
		glXDestroyContext(gpDisplay,gGLXContext);
	}
	
	if(gWindow)
	{
		XDestroyWindow(gpDisplay,gWindow);
	}
	
	if(gColormap)
	{
		XFreeColormap(gpDisplay,gColormap);
	}
	
	if(gpXVisualInfo)
	{
		free(gpXVisualInfo);
		gpXVisualInfo=NULL;
	}
	
	if(gpDisplay)
	{
		XCloseDisplay(gpDisplay);
		gpDisplay=NULL;
	}

	if (gpFile)
	{
		fprintf(gpFile, "Log File Is Successfully Closed.\n");
		fclose(gpFile);
		gpFile = NULL;
	}
}
