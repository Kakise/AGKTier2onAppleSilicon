#ifndef _H_AGK_OPENGLES2_
#define _H_AGK_OPENGLES2_

#include "PlatformDefines.h"

#ifdef AGK_ANDROID
	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include "GLES2/gl2ext.h"

	void SetRendererPointers( void *ptr );
	int GetSurfaceWidth();
	int GetSurfaceHeight();
	const char* GetRendererName();

	void RegenerateExternalTexture( unsigned int *tex );
	void BindExternalTexture( unsigned int tex );
	void DeleteExternalTexture( unsigned int *tex );
#endif

#ifdef AGK_IOS
	#import <OpenGLES/EAGL.h>
	#import <OpenGLES/ES2/gl.h>
	#import <OpenGLES/ES2/glext.h>
	#import <OpenGLES/EAGLDrawable.h>

	int CreateRendererWindow( void *ptr, int resMode, int deviceWidth, int deviceHeight );
	void SetRendererResolution( void *ptr, int width, int height );
	void SetRendererPointers( void *ptr, int resMode, int deviceWidth, int deviceHeight );
	int GetSurfaceWidth();
	int GetSurfaceHeight();

    int CreateVideoImageData();
    int HasVideoImageData();
    void CopyVideoImageToAGKImage( CVPixelBufferRef buffer, cImage* pImage );
    void ReleaseVideoImageData();

    int CreateCameraImageData();
    int HasCameraImageData();
    void CopyCameraImageToAGKImage( CVImageBufferRef buffer, cImage* pImage );
    void ReleaseCameraImageData();

    int CreateARImageData();
    int HasARImageData();
    void CopyARImageToAGKImage( CVPixelBufferRef buffer, cImage* pImageY, cImage* pImageUV );
    void ReleaseARImageData();
#endif

#ifdef AGK_RASPPI
	#include "bcm_host.h"
	#include "GLES2/gl2.h"
	#include "GLES2/gl2ext.h"
	#include "EGL/egl.h"
	#include "EGL/eglext.h"

	void SetRendererPointers( void *ptr );
	int GetSurfaceWidth();
	int GetSurfaceHeight();
#endif

#ifdef AGK_HTML5
	#include <GLES2/gl2.h>
	#define GLFW_INCLUDE_ES2
	#include <GLFW/glfw3.h>
#endif

void CheckRendererExtensions();

#endif // _H_AGK_OPENGLES2_
