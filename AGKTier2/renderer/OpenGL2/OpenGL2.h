#ifndef _H_AGK_OPENGL2_
#define _H_AGK_OPENGL2_

#include "PlatformDefines.h"

#ifdef AGK_WINDOWS
	#include <gl\gl.h>
	#include "glext.h"
	#include "wglext.h"

	typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );
	extern PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT;

	typedef HGLRC (APIENTRY *PFNWGLCreateContextARB)( HDC, HGLRC, const int* );
	extern PFNWGLCreateContextARB wglCreateContextAttribsARB;

	int CreateRendererWindow( HWND hWnd );
	bool LoadRendererExtensions();
#endif

#ifdef AGK_MACOS
    #include <OpenGL/gl.h>
	#include "../../platform/mac/GLFW/glfw3.h"
#endif

#ifdef AGK_LINUX
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GLFW/glfw3.h>
	#include <GL/glx.h>

	bool LoadRendererExtensions();
#endif

#endif // _H_AGK_OPENGL2_
