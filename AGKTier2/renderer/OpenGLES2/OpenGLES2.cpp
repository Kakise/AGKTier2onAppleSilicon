#include "agk.h"
#include "zlib.h"

#ifdef AGK_IOS
    #include "OpenGLES/ES3/gl.h"
    #include "OpenGLES/ES3/glext.h"
#else
    #include "OpenGLES2.h"
#endif

#ifndef GL_DEPTH_COMPONENT24
	#define GL_DEPTH_COMPONENT24 0x81A6
#endif

bool g_bDepthTextureSupported = false;
bool g_bDepth24Supported = false;
bool g_bDepthNonLinear = false;
bool g_bShadowSamplers = false;
unsigned int g_iCapabilityFlags = 0;
bool g_bOpenGL3 = false;

bool IsExtensionSupported(const char *extension)
{
	const size_t extlen = strlen(extension);
	const char *supported = NULL;
 
	supported = (char*)glGetString(GL_EXTENSIONS);
 
	// If That Failed Must Be No Extensions Supported
	if (supported == NULL) return false;
 
	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (const char* p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);
		if (p == NULL) return false;                       // No Match
 
		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"
 
		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p==supported || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' ')) return true; // Match
	}
}

void CheckRendererExtensions()
{
	const char* extensions = (const char*) glGetString( GL_EXTENSIONS );
	if ( IsExtensionSupported( "GL_OES_depth_texture" ) ) g_bDepthTextureSupported = true;
	if ( IsExtensionSupported( "GL_OES_depth24" ) ) g_bDepth24Supported = true;
	if ( IsExtensionSupported( "GL_NV_depth_nonlinear" ) ) g_bDepthNonLinear = true;
	if ( IsExtensionSupported( "GL_EXT_shadow_samplers" ) ) g_bShadowSamplers = true;
    // something is wrong on iOS with OpenGL ES 2.0 and the UINT index extension, objects become corrupted
#ifndef AGK_IOS
	if ( IsExtensionSupported( "GL_OES_element_index_uint" ) ) g_iCapabilityFlags |= AGK_CAP_UINT_INDICES;
#endif
    
    if ( g_bOpenGL3 )
    {
        g_bDepthTextureSupported = true;
        g_bDepth24Supported = true;
        g_iCapabilityFlags |= AGK_CAP_UINT_INDICES;
    }
}

#ifdef AGK_ANDROID
	EGLSurface g_surface;
    EGLContext g_context;
    EGLDisplay g_display;
	EGLConfig g_eglConfig;
	static int firstcleardone = 0;

	ANativeWindow* recordWindow = 0;
	EGLSurface recordSurface = EGL_NO_SURFACE;
	int recordWidth = 0;
	int recordHeight = 0;
	cObject3D* recordQuad = 0;
	cImage* recordImage = 0;

	struct egldata 
	{
		EGLDisplay display;
		EGLSurface surface;
		EGLContext context;
		void *reserved; // ANativeActivity*
		EGLConfig config;
		void* reserved2; // ANativeWindow*
	};

	void SetRendererPointers( void *ptr )
	{
		egldata *ePtr = (egldata*)ptr;
		g_surface = ePtr->surface;
		g_context = ePtr->context;
		g_display = ePtr->display;
		g_eglConfig = ePtr->config;

		firstcleardone = 0;
	}

	int GetSurfaceWidth()
	{
		int width;
		eglQuerySurface(g_display, g_surface, EGL_WIDTH, &width);
		return width;
	}

	int GetSurfaceHeight()
	{
		int height;
		eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &height);
		return height;
	}

	// external textures are used on Android for VideoToTexture and CameraToTexture, they use GL_TEXTURE_EXTERNAL_OES instead of GL_TEXTURE_2D
	void RegenerateExternalTexture( unsigned int *tex )
	{
		if ( *tex ) glDeleteTextures( 1, tex );
		glGenTextures( 1, tex );
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, *tex ); // not shared with TEXTURE_2D so no need to clear binding
		glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	}

	void BindExternalTexture( unsigned int tex )
	{
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, tex );
	}

	void DeleteExternalTexture( unsigned int *tex )
	{
		if ( *tex ) glDeleteTextures( 1, tex );
		*tex = 0;
	}

	const char* GetRendererName()
	{
		return (const char*) glGetString( GL_RENDERER );
	}

	void agk::PlatformDestroyGL ( void )
	{
		
	}

	void agk::PlatformSwap()
	{
		if ( g_display == NULL || g_surface == NULL ) return;

		GLenum err = glGetError();
		if ( err != GL_NO_ERROR )
		{
			if ( err == GL_OUT_OF_MEMORY )
			{
				agk::Error( "Ran out of GPU memory" );
				throw 1;
			}
		}
		
		// display backbuffer
		if ( recordSurface )
		{
			// copy back buffer to texture
			recordImage->Bind( 0 );
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, GetSurfaceWidth(), GetSurfaceHeight(), 0 );
			eglSwapBuffers(g_display, g_surface);

			// draw texture to recording surface
			eglMakeCurrent(g_display, recordSurface, recordSurface, g_context);
			glViewport( 0, 0, recordWidth, recordHeight );
			m_bUsingFBO = true;
			m_iFBOWidth = recordWidth;
			m_iFBOHeight = recordHeight;
			recordQuad->Draw();
			eglSwapBuffers(g_display, recordSurface);
			
			// switch back to main surface
			eglMakeCurrent(g_display, g_surface, g_surface, g_context);
			glViewport( 0, 0, GetSurfaceWidth(), GetSurfaceHeight() );
			m_bUsingFBO = 0;
		}
		else
		{
			eglSwapBuffers(g_display, g_surface);
		}	
	}
#endif

#ifdef AGK_IOS
	EAGLContext *context = 0;
	GLuint defaultFramebuffer = 0;
    GLuint colorRenderbuffer = 0;
    GLuint depthRenderbuffer = 0;
	UINT m_iManualResolution = 0;

    // for video to OpenGLES texture
    __CVOpenGLESTextureCache *videoCache = 0;
    CVOpenGLESTextureRef videoTextureCV[2] = {0,0};
    unsigned int videoCurrTexture = 0;

    // for camera to OpenGLES texture
    __CVOpenGLESTextureCache *cameraCache = 0;
    CVOpenGLESTextureRef cameraTextureCV[2] = {0,0};
    unsigned int cameraCurrTexture = 0;

    // for AR to OpenGLES texture
    __CVOpenGLESTextureCache *ARCache = 0;
    CVOpenGLESTextureRef ARTextureCV[2] = {0,0};
    CVOpenGLESTextureRef ARTextureCV2[2] = {0,0};
    unsigned int ARCurrTexture = 0;

	int CreateRendererWindow( void *ptr, int resMode, int deviceWidth, int deviceHeight )
	{
		UIViewController* pViewController = (UIViewController*)ptr;
		CAEAGLLayer *eaglLayer = (CAEAGLLayer*)pViewController.view.layer;

        g_bOpenGL3 = 1;
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
        if ( !context )
        {
            g_bOpenGL3 = 0;
            agk::Warning( "OpenGL ES 3.0 not supported, trying OpenGL ES 2.0" );
            context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
            if ( !context )
            {
                agk::Message( "Failed to create OpenGL ES 2.0 context" );
                agk::Sleep(2000);
                NSLog(@"Failed to create OpenGL ES 2.0 context");
                return 0;
            }
        }
        
        if ( ![EAGLContext setCurrentContext:context] )
        {
            agk::Message( "Failed to make OpenGL context current" );
            agk::Sleep(2000);
            NSLog(@"Failed to make OpenGL context current");
            return 0;
        }

		CheckRendererExtensions();

		if ( !defaultFramebuffer ) glGenFramebuffers(1, &defaultFramebuffer);
		if ( !colorRenderbuffer ) glGenRenderbuffers(1, &colorRenderbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
			    
		// iPad may try to scale down our render buffer to 320x480 
		// resMode=1 overrides this to native resolution
		CGRect oldRect = [ eaglLayer bounds ];
        CGRect rect;
			    
		bool bChangedSize = false;
		if ( resMode == 1 )
		{
			rect.origin.x = 0; rect.origin.y = 0;
			rect.size.width = deviceWidth;
			rect.size.height = deviceHeight;
			[ eaglLayer setBounds:rect ];
			bChangedSize = true;
		}
		else
		{
			// if this really is a high res device the user wants a lower res
			if ( deviceHeight >= 960 || deviceWidth >= 960 )
			{
				rect.origin.x = 0; rect.origin.y = 0;
				rect.size.width = deviceWidth/2;
				rect.size.height = deviceHeight/2;
				[ eaglLayer setBounds:rect ];
				bChangedSize = true;
			}
			else
			{
				rect.origin.x = 0; rect.origin.y = 0;
				rect.size.width = deviceWidth;
				rect.size.height = deviceHeight;
				[ eaglLayer setBounds:rect ];
				bChangedSize = true;
			}
		}
        
        //NSLog(@"Old Rect Width: %f, Height: %f", oldRect.size.width, oldRect.size.height);
        //NSLog(@"New Rect Width: %f, Height: %f", rect.size.width, rect.size.height);
        //NSLog(@"Screen Width: %d, Screen Height: %d", deviceWidth, deviceHeight);
	    
		[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
		
		// reset layer size so it thinks we accepted the lower resolution
		if ( bChangedSize ) [ eaglLayer setBounds:oldRect ];

		GLint renderWidth;
		GLint renderHeight;
		
		//work out if we need any borders to prevent image stretching
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderHeight);
		
		if ( !depthRenderbuffer ) glGenRenderbuffers(1, &depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, g_bDepth24Supported ? 0x81A6 : GL_DEPTH_COMPONENT16, renderWidth, renderHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
		
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        
        return 1;
	}

	void SetRendererResolution( void* ptr, int width, int height )
	{
		UIViewController* pViewController = (UIViewController*)ptr;
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)pViewController.view.layer;
	
		if ( width < 0 ) width = 0;
		if ( height < 0 ) height = 0;
		if ( width > agk::GetMaxDeviceWidth() ) width = agk::GetMaxDeviceWidth();
		if ( height > agk::GetMaxDeviceHeight() ) height = agk::GetMaxDeviceHeight();
	    
		if ( width == 0 ) width = agk::GetMaxDeviceWidth();
		if ( height == 0 ) height = agk::GetMaxDeviceHeight();
	    
		if ( width == agk::GetMaxDeviceWidth() && height == agk::GetMaxDeviceHeight() )
			m_iManualResolution = 0;
		else
			m_iManualResolution = 1;
	    
		CGRect oldRect = [ eaglLayer bounds ];
		CGRect rect;
		rect.origin.x = 0; rect.origin.y = 0;
		rect.size.width = width;
		rect.size.height = height;
		[ eaglLayer setBounds:rect ];
				
		[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
		
		// reset layer size so it thinks we accepted the lower resolution
		[ eaglLayer setBounds:oldRect ];
	    	    
		GLint renderWidth;
		GLint renderHeight;
		
		//work out if we need any borders to prevent image stretching
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderHeight);
		
		if ( !depthRenderbuffer ) glGenRenderbuffers(1, &depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, g_bDepth24Supported ? 0x81A6 : GL_DEPTH_COMPONENT16, renderWidth, renderHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
		
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	}

	void SetRendererPointers( void *ptr, int resMode, int deviceWidth, int deviceHeight )
	{
		UIViewController* pViewController = (UIViewController*)ptr;
		CAEAGLLayer *eaglLayer = (CAEAGLLayer*)pViewController.view.layer;

		CGRect oldRect = [ eaglLayer bounds ];
		bool bChangedSize = false;
	    
		if ( m_iManualResolution )
		{
			CGRect rect;
			rect.origin.x = 0; rect.origin.y = 0;
			rect.size.width = deviceWidth;
			rect.size.height = deviceHeight;
			[ eaglLayer setBounds:rect ];
			bChangedSize = true;
	        
			[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
		}
		else
		{
	        
			if ( resMode == 1 )
			{
				CGRect rect;
				rect.origin.x = 0; rect.origin.y = 0;
				rect.size.width = deviceWidth;
				rect.size.height = deviceHeight;
				[ eaglLayer setBounds:rect ];
				bChangedSize = true;
			}
			else
			{
				// if this really is a high res device the user wants a lower res
				if ( deviceHeight >= 960 || deviceWidth >= 960 )
				{
					CGRect rect;
					rect.origin.x = 0; rect.origin.y = 0;
					rect.size.width = deviceWidth/2;
					rect.size.height = deviceHeight/2;
					[ eaglLayer setBounds:rect ];
					bChangedSize = true;
				}
				else
				{
					CGRect rect;
					rect.origin.x = 0; rect.origin.y = 0;
					rect.size.width = deviceWidth;
					rect.size.height = deviceHeight;
					[ eaglLayer setBounds:rect ];
					bChangedSize = true;
				}
			}
	        
			[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
		}
	    
		if ( bChangedSize ) [ eaglLayer setBounds:oldRect ];
	    
		GLint renderWidth;
		GLint renderHeight;
		
		//work out if we need any borders to prevent image stretching
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &renderWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &renderHeight);
		
		if ( !depthRenderbuffer ) glGenRenderbuffers(1, &depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, g_bDepth24Supported ? 0x81A6 : GL_DEPTH_COMPONENT16, renderWidth, renderHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
		
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
	}

	int GetSurfaceWidth()
	{
		int width;
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
		return width;
	}

	int GetSurfaceHeight()
	{
		int height;
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
		return height;
	}

    int CreateVideoImageData()
    {
        if ( !videoCache )
        {
            CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, context, NULL, &videoCache);
            if (err != noErr) {
                agk::Warning( "Failed to play video to texture, failed to create texture cache" );
                return 0;
            }
        }
        
        return 1;
    }

    int HasVideoImageData()
    {
        return videoCache ? 1 : 0;
    }

    void CopyVideoImageToAGKImage( CVPixelBufferRef buffer, cImage* pImage )
    {
        videoCurrTexture = 1 - videoCurrTexture;
        if ( videoTextureCV[videoCurrTexture] )
        {
            CFRelease(videoTextureCV[videoCurrTexture]);
            videoTextureCV[videoCurrTexture] = 0;
        }
        
        CVOpenGLESTextureCacheFlush(videoCache, 0);
        
        CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     videoCache,
                                                     buffer,
                                                     NULL,
                                                     GL_TEXTURE_2D,
                                                     GL_RGBA,
                                                     (int)CVPixelBufferGetWidth(buffer),
                                                     (int)CVPixelBufferGetHeight(buffer),
                                                     GL_BGRA,
                                                     GL_UNSIGNED_BYTE,
                                                     0,
                                                     &(videoTextureCV[videoCurrTexture]));
        
        pImage->OverrideTexture(CVOpenGLESTextureGetName(videoTextureCV[videoCurrTexture]), (int)CVPixelBufferGetWidth(buffer), (int)CVPixelBufferGetHeight(buffer));
    }

    void ReleaseVideoImageData()
    {
        if ( videoTextureCV[0] )
        {
            CFRelease(videoTextureCV[0]);
            videoTextureCV[0] = 0;
        }
        if ( videoTextureCV[1] )
        {
            CFRelease(videoTextureCV[1]);
            videoTextureCV[1] = 0;
        }
        
        if ( videoCache )
        {
            CVOpenGLESTextureCacheFlush(videoCache, 0);
            CFRelease(videoCache);
            videoCache = 0;
        }
    }

    int CreateCameraImageData()
    {
        if ( !cameraCache )
        {
            CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, context, NULL, &cameraCache);
            if (err != noErr) {
                agk::Warning( "Failed to set device camera to image, failed to create texture cache" );
                return 0;
            }
        }
        
        return 1;
    }

    int HasCameraImageData()
    {
        return cameraCache ? 1 : 0;
    }

    void CopyCameraImageToAGKImage( CVImageBufferRef buffer, cImage* pImage )
    {
        cameraCurrTexture = 1 - cameraCurrTexture;
        if ( cameraTextureCV[cameraCurrTexture] )
        {
            CFRelease(cameraTextureCV[cameraCurrTexture]);
            cameraTextureCV[cameraCurrTexture] = 0;
        }
        
        CVOpenGLESTextureCacheFlush(cameraCache, 0);
        
        CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     cameraCache,
                                                     buffer,
                                                     NULL,
                                                     GL_TEXTURE_2D,
                                                     GL_RGBA,
                                                     (int)CVPixelBufferGetWidth(buffer),
                                                     (int)CVPixelBufferGetHeight(buffer),
                                                     GL_BGRA,
                                                     GL_UNSIGNED_BYTE,
                                                     0,
                                                     &(cameraTextureCV[cameraCurrTexture]));
        
        pImage->OverrideTexture(CVOpenGLESTextureGetName(cameraTextureCV[cameraCurrTexture]), (int)CVPixelBufferGetWidth(buffer), (int)CVPixelBufferGetHeight(buffer));
    }

    void ReleaseCameraImageData()
    {
        if ( cameraTextureCV[0] )
        {
            CFRelease(cameraTextureCV[0]);
            cameraTextureCV[0] = 0;
        }
        if ( cameraTextureCV[1] )
        {
            CFRelease(cameraTextureCV[1]);
            cameraTextureCV[1] = 0;
        }
        
        if ( cameraCache )
        {
            CVOpenGLESTextureCacheFlush(cameraCache, 0);
            CFRelease(cameraCache);
            cameraCache = 0;
        }
    }

    int CreateARImageData()
    {
        if ( !ARCache )
        {
            CVReturn err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, context, NULL, &ARCache);
            if (err != noErr) {
                agk::Warning( "Failed to create AR texture cache" );
                return 0;
            }
        }
        
        return 1;
    }

    int HasARImageData()
    {
        return ARCache ? 1 : 0;
    }

    void CopyARImageToAGKImage( CVPixelBufferRef buffer, cImage* pImageY, cImage* pImageUV )
    {
        ARCurrTexture = 1 - ARCurrTexture;
        if ( ARTextureCV[ARCurrTexture] )
        {
            CFRelease(ARTextureCV[ARCurrTexture]);
            ARTextureCV[ARCurrTexture] = 0;
        }
        
        if ( ARTextureCV2[ARCurrTexture] )
        {
            CFRelease(ARTextureCV2[ARCurrTexture]);
            ARTextureCV2[ARCurrTexture] = 0;
        }
        
        CVOpenGLESTextureCacheFlush(ARCache, 0);
        
        int width = (int)CVPixelBufferGetWidthOfPlane(buffer,0);
        int height = (int)CVPixelBufferGetHeightOfPlane(buffer,0);
        
        // Lumninace texture
        CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     ARCache,
                                                     buffer,
                                                     NULL,
                                                     GL_TEXTURE_2D,
                                                     GL_LUMINANCE,
                                                     width,
                                                     height,
                                                     GL_LUMINANCE,
                                                     GL_UNSIGNED_BYTE,
                                                     0,
                                                     &(ARTextureCV[ARCurrTexture]));
        
        pImageY->OverrideTexture(CVOpenGLESTextureGetName(ARTextureCV[ARCurrTexture]), width, height);
        
        width = (int)CVPixelBufferGetWidthOfPlane(buffer,1);
        height = (int)CVPixelBufferGetHeightOfPlane(buffer,1);
        
        CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     ARCache,
                                                     buffer,
                                                     NULL,
                                                     GL_TEXTURE_2D,
                                                     GL_LUMINANCE_ALPHA,
                                                     width,
                                                     height,
                                                     GL_LUMINANCE_ALPHA,
                                                     GL_UNSIGNED_BYTE,
                                                     1,
                                                     &(ARTextureCV2[ARCurrTexture]));
        
        pImageUV->OverrideTexture(CVOpenGLESTextureGetName(ARTextureCV2[ARCurrTexture]), width, height);
    }

    void ReleaseARImageData()
    {
        if ( ARTextureCV[0] ) CFRelease(ARTextureCV[0]);
        if ( ARTextureCV[1] ) CFRelease(ARTextureCV[1]);
        if ( ARTextureCV2[0] ) CFRelease(ARTextureCV2[0]);
        if ( ARTextureCV2[1] ) CFRelease(ARTextureCV2[1]);
        ARTextureCV[0] = 0;
        ARTextureCV[1] = 0;
        ARTextureCV2[0] = 0;
        ARTextureCV2[1] = 0;
        
        if ( ARCache )
        {
            CVOpenGLESTextureCacheFlush(ARCache, 0);
            CFRelease(ARCache);
            ARCache = 0;
        }
    }

	void agk::PlatformDestroyGL ( void )
	{
		
	}

	void agk::PlatformSwap()
	{
		GLenum err = glGetError();
		if ( err != GL_NO_ERROR )
		{
			if ( err == GL_OUT_OF_MEMORY )
			{
				agk::Error( "Ran out of GPU memory, try using smaller or fewer images" );
				throw 1;
			}
		}
		
		// display backbuffer
		[context presentRenderbuffer:GL_RENDERBUFFER];
	}
#endif

#ifdef AGK_RASPPI
	void *g_pWindow = 0;
	void *g_pSurface = 0;
	void *g_pContext = 0;
	
	struct egldata
	{
		void *display;
		void *surface;
		void *context;
		void *reserved1;
		void *reserved2;
	};

	void SetRendererPointers( void *ptr )
	{
		egldata *ePtr = (egldata*)ptr;
		g_pWindow = ePtr->display;
		g_pSurface = ePtr->surface;
		g_pContext = ePtr->context;
	}

	int GetSurfaceWidth()
	{
		int width;
		eglQuerySurface(g_pWindow, g_pSurface, EGL_WIDTH, &width);
		return width;
	}

	int GetSurfaceHeight()
	{
		int height;
		eglQuerySurface(g_pWindow, g_pSurface, EGL_HEIGHT, &height);
		return height;
	}

	void agk::PlatformDestroyGL ( void )
	{
		
	}

	void agk::PlatformSwap()
	{
		GLenum err = glGetError();
		if ( err != GL_NO_ERROR )
		{
			if ( err == GL_OUT_OF_MEMORY )
			{
				agk::Error( "The Raspberry Pi ran out of GPU memory, please increase the amount of RAM assigned to the GPU" );
				throw 1;
				/*
				agk::CleanUp();
				XCloseDisplay( g_pXDisplay );
				eglMakeCurrent( g_pWindow, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
    			eglDestroySurface( g_pWindow, g_pSurface );
    			eglDestroyContext( g_pWindow, g_pContext );
				eglTerminate( g_pWindow );
				exit(0);
				*/
			}
		}
		
		// display backbuffer
		eglSwapBuffers(g_pWindow,g_pSurface);
	}
#endif

#ifdef AGK_HTML5
	namespace AGK
	{
		extern GLFWwindow *g_pWindow;
	}

	void agk::PlatformDestroyGL ( void )
	{
		
	}

	void agk::PlatformSwap()
	{
		glfwSwapBuffers(g_pWindow);
	}
#endif

UINT agk::IsSupportedDepthTexture()
//****
{
	return g_bDepthTextureSupported ? 1 : 0;
}

int agk::CanUseIntIndices() 
{ 
	return (g_iCapabilityFlags & AGK_CAP_UINT_INDICES) ? 1 : 0;
}

int agk::CanUseShadowSamplers() 
{ 
	return g_bShadowSamplers ? 1 : 0;
}

void agk::SetAntialiasMode( int mode )
//****
{
	// do nothing on mobile
}

void agk::PlatformRendererFinish()
{
	glFinish();
}

void agk::PlatformSetScreenRecordingParams( void* param1, void* param2 )
{
#ifdef AGK_ANDROID
	recordWindow = (ANativeWindow*) param1;

	if ( recordSurface ) eglDestroySurface( g_display, recordSurface );
	recordSurface = 0;

	if ( recordImage ) delete recordImage;
	recordImage = 0;

	if ( recordQuad ) delete recordQuad;
	recordQuad = 0;
		
	if ( recordWindow )
	{		
		recordSurface = eglCreateWindowSurface(g_display, g_eglConfig, recordWindow, NULL);
		if ( recordSurface == EGL_NO_SURFACE )
		{
			uString err; err.Format( "Failed to create record surface: %d", eglGetError() );
			agk::Error( err );
			return;
		}
		
		eglQuerySurface(g_display, recordSurface, EGL_WIDTH, &recordWidth);
		eglQuerySurface(g_display, recordSurface, EGL_HEIGHT, &recordHeight);
			
		recordImage = new cImage();
		recordImage->CreateBlankImage( GetSurfaceWidth(), GetSurfaceHeight(), 0, 0 );
		recordImage->SetWrapU( 0 );
		recordImage->SetWrapV( 0 );
		recordImage->SetMinFilter( 1 );
		recordImage->SetMagFilter( 1 );

		recordQuad = new cObject3D();
		recordQuad->CreateQuad();
		recordQuad->SetImage( recordImage );
	}
#endif
}

int agk::PlatformGetMaxVSUniforms()
{
	int maxUniformVectors;
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &maxUniformVectors);
	return maxUniformVectors;
}

int agk::PlatformGetMaxPSUniforms()
{
	int maxUniformVectors;
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &maxUniformVectors);
	return maxUniformVectors;
}

int agk::PlatformGetMaxVertexTextures()
{
	int maxVertexTextures;
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextures);
	return maxVertexTextures;
}

int agk::PlatformGetMaxVaryings()
{
	int maxVaryingVectors;
	glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryingVectors);
	return maxVaryingVectors * 4;
}

void agk::PlatformSetAlignment( int amount )
{
	glPixelStorei( GL_PACK_ALIGNMENT, amount );
	glPixelStorei( GL_UNPACK_ALIGNMENT, amount );
}

void agk::PlatformDisableScissor()
{
	glDisable( GL_SCISSOR_TEST );
}

void agk::PlatformDisableStencil()
{
	glDisable( GL_STENCIL_TEST );
}

void agk::PlatformPrepareDefaultDraw()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(1.0f);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	agk::PlatformSetDepthTest( 1 );
	agk::PlatformSetBlendMode( 1 );
	agk::PlatformSetCullMode( 0 );
	agk::PlatformSetDepthWrite( 1 );
	agk::PlatformSetDepthRange( 0, 1 );
	agk::PlatformSetDepthBias( 0 );
	glFrontFace( GL_CCW );
}

void agk::PlatformSetBlendEnabled( int mode )
{
	if ( mode == m_iCurrentBlendEnabled ) return;

	if ( mode > 0 ) glEnable( GL_BLEND );
	else glDisable( GL_BLEND );

	m_iCurrentBlendEnabled = mode > 0 ? 1 : 0;
}

void agk::PlatformSetDepthTest( int mode )
{
	if ( mode == m_iCurrentDepthTest ) return;

	if ( mode > 0 ) glEnable( GL_DEPTH_TEST );
	else glDisable( GL_DEPTH_TEST );

	m_iCurrentDepthTest = mode > 0 ? 1 : 0;
}

void agk::PlatformSetBlendFunc( int mode1, int mode2 )
{
	if ( mode1 == m_iCurrentBlendFunc1 && mode2 == m_iCurrentBlendFunc2 ) return;

	int final1 = GL_ONE;
	switch ( mode1 )
	{
		case 0: final1 = GL_ZERO; break;
		case 1: final1 = GL_ONE; break;
		case 2: final1 = GL_SRC_ALPHA; break;
		case 3: final1 = GL_ONE_MINUS_SRC_ALPHA; break;
		case 4: final1 = GL_DST_ALPHA; break;
		case 5: final1 = GL_ONE_MINUS_DST_ALPHA; break;
		//case 6: final1 = GL_SRC_COLOR; break; // illegal
		//case 7: final1 = GL_ONE_MINUS_SRC_COLOR; break; // illegal
		case 8: final1 = GL_DST_COLOR; break;
		case 9: final1 = GL_ONE_MINUS_DST_COLOR; break;
		case 10: final1 = GL_SRC_ALPHA_SATURATE; break;
		default: mode1 = AGK_BLEND_ONE;
	}

	int final2 = GL_ZERO;
	switch ( mode2 )
	{
		case 0: final2 = GL_ZERO; break;
		case 1: final2 = GL_ONE; break;
		case 2: final2 = GL_SRC_ALPHA; break;
		case 3: final2 = GL_ONE_MINUS_SRC_ALPHA; break;
		case 4: final2 = GL_DST_ALPHA; break;
		case 5: final2 = GL_ONE_MINUS_DST_ALPHA; break;
		case 6: final2 = GL_SRC_COLOR; break; 
		case 7: final2 = GL_ONE_MINUS_SRC_COLOR; break; 
		//case 8: final2 = GL_DST_COLOR; break; // illegal
		//case 9: final2 = GL_ONE_MINUS_DST_COLOR; break; // illegal
		//case 10: final2 = GL_SRC_ALPHA_SATURATE; break; // illegal
		default: mode2 = AGK_BLEND_ZERO;
	}

	glBlendFuncSeparate( final1, final2, GL_ONE_MINUS_DST_ALPHA, GL_ONE );
	
	m_iCurrentBlendFunc1 = mode1;
	m_iCurrentBlendFunc2 = mode2;
}

void agk::PlatformSetDepthWrite( int mode )
{
	if ( mode == m_iCurrentDepthWrite ) return;

	if ( mode > 0 ) glDepthMask( GL_TRUE );
	else glDepthMask( GL_FALSE );

	m_iCurrentDepthWrite = mode > 0 ? 1 : 0;
}

void agk::PlatformSetDepthBias( float bias )
{
	if ( bias == m_fCurrentDepthBias ) return;

	if ( bias == 0 ) glDisable( GL_POLYGON_OFFSET_FILL );
	else 
	{
		if ( m_fCurrentDepthBias == 0 ) glEnable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 1, bias );
	}

	m_fCurrentDepthBias = bias;
}

void agk::PlatformSetDepthRange( float zNear, float zFar )
{
	if ( zNear < 0 ) zNear = 0;
	if ( zNear > 1 ) zNear = 1;
	if ( zFar < 0 ) zFar = 0;
	if ( zFar > 1 ) zFar = 1;

	if ( zNear == m_fCurrentDepthNear && zFar == m_fCurrentDepthFar ) return;

	glDepthRangef( zNear, zFar );

	m_fCurrentDepthNear = zNear;
	m_fCurrentDepthFar = zFar;
}

void agk::PlatformSetDepthFunc( int mode )
{
	if ( mode == m_iCurrentDepthFunc ) return;

	switch ( mode )
	{
		case 0: glDepthFunc( GL_NEVER ); break;
		case 1: glDepthFunc( GL_LESS ); break;
		case 2: glDepthFunc( GL_EQUAL ); break;
		case 3: glDepthFunc( GL_LEQUAL ); break;
		case 4: glDepthFunc( GL_GREATER ); break;
		case 5: glDepthFunc( GL_NOTEQUAL ); break;
		case 6: glDepthFunc( GL_GEQUAL ); break;
		case 7: glDepthFunc( GL_ALWAYS ); break;
		default: return;
	}
	
	m_iCurrentDepthFunc = mode;
}

void agk::PlatformSetCullMode( int mode )
{
	if ( m_bUsingFBO )
	{
		if ( mode == 1 ) mode = 2;
		else if ( mode == 2 ) mode = 1;
	}

	if ( mode == m_iCurrentCullMode ) return;

	switch( mode )
	{
		case 0: if ( m_iCurrentCullMode != 0 ) glDisable( GL_CULL_FACE ); break; // both front and back
		case 1: // front faces
		{
			if ( m_iCurrentCullMode <= 0 ) glEnable( GL_CULL_FACE );
			glCullFace( GL_BACK );
			break;
		}
		case 2: // back faces
		{
			if ( m_iCurrentCullMode <= 0 ) glEnable( GL_CULL_FACE );
			glCullFace( GL_FRONT );
			break;
		}
		default: return;
	}

	m_iCurrentCullMode = mode;
}

bool agk::PlatformBindBuffer( UINT buffer )
{
	if ( m_iCurrentBoundVBO == buffer ) return false;

	m_iCurrentBoundVBO = buffer;
	glBindBuffer( GL_ARRAY_BUFFER, buffer );

	return true;
}

bool agk::PlatformBindIndexBuffer( UINT buffer )
{
	if ( m_iCurrentBoundIndexVBO == buffer ) return false;

	m_iCurrentBoundIndexVBO = buffer;
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer );

	return true;
}

void agk::PlatformSetViewport( int x, int y, int width, int height )
{
	glViewport( x, y, width, height );
}

void agk::PlatformBindDefaultFramebuffer()
{
#ifdef AGK_IOS
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer );
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
#else
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
#endif
}

void agk::PlatformFlush()
{
	glFlush();
}

void agk::PlatformClearScreenRaw( float red, float green, float blue, float alpha )
{
	glClearColor( red, green, blue, alpha );
	glClear( GL_COLOR_BUFFER_BIT );
}

void agk::PlatformClearScreen()
{
#ifdef AGK_ANDROID
	// android can't set the viewport properly until swap has been called!
	// also reset it on long frame time pauses as the app may have been switched out and back in, meaning the viewport needs resetting.
	if ( firstcleardone == 0 || GetFrameTime() > 0.5f ) PlatformSetViewport( 0, 0, m_iRenderWidth, m_iRenderHeight );
	firstcleardone = 1;
#endif
	
	// this *must* be set back to true before clearing the depth buffer.
	agk::PlatformSetDepthWrite( 1 );

	if ( m_fTargetViewportX == 0 && m_fTargetViewportY == 0 )
	{
		// no borders
		GLbitfield iClear = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		
		PlatformScissor( 0,0,0,0 );
		float red = ((m_iClearColor >> 16) & 0xff) / 255.0f;
		float green = ((m_iClearColor >> 8) & 0xff) / 255.0f;
		float blue = (m_iClearColor & 0xff) / 255.0f;
		glClearColor( red, green, blue, 0 );
		glClear( iClear );
		PlatformScissor( m_iScissorX, m_iScissorY, m_iScissorWidth, m_iScissorHeight );
	}
	else
	{
		// clear borders then clear main drawing area
		GLbitfield iClear = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

		float red = ((m_iBorderColor >> 16) & 0xff) / 255.0f;
		float green = ((m_iBorderColor >> 8) & 0xff) / 255.0f;
		float blue = (m_iBorderColor & 0xff) / 255.0f;
		PlatformScissor( 0,0,0,0 );
		glClearColor( red, green, blue, 0 );
		glClear( iClear );

		// check if we need to clear again
		if ( m_iBorderColor != m_iClearColor )
		{
			int ired = ((m_iClearColor >> 16) & 0xff);
			int igreen = ((m_iClearColor >> 8) & 0xff);
			int iblue = (m_iClearColor & 0xff);
			static cSprite* g_pClearSprite = 0;
			if ( !g_pClearSprite ) g_pClearSprite = new cSprite();
			g_pClearSprite->SetSize( (float)agk::GetVirtualWidth(), (float)agk::GetVirtualHeight() );
			g_pClearSprite->SetColor( ired, igreen, iblue, 0 );
			g_pClearSprite->SetTransparency( 0 );
			agk::PlatformSetDepthTest(0);
			agk::PlatformSetDepthWrite(0);
			g_pClearSprite->Draw();
			agk::PlatformSetDepthTest(1);
			
		}

		PlatformScissor( m_iScissorX, m_iScissorY, m_iScissorWidth, m_iScissorHeight );
	}
}

void agk::PlatformClearDepthBuffer()
{
	// this *must* be set back to true before clearing the depth buffer.
	agk::PlatformSetDepthWrite( 1 );

	PlatformScissor( 0,0,0,0 );
	glClear( GL_DEPTH_BUFFER_BIT );
	PlatformScissor( m_iScissorX, m_iScissorY, m_iScissorWidth, m_iScissorHeight );
}

void agk::PlatformScissor( int x, int y, int width, int height )
{
	if ( x == 0 && y == 0 && width == 0 && height == 0 )
	{
		if ( m_bScissorEnabled ) glDisable( GL_SCISSOR_TEST );
		m_bScissorEnabled = false;
	}
	else
	{
		if ( !m_bScissorEnabled )
		{
			glEnable( GL_SCISSOR_TEST );
		}
		glScissor( x, y, width, height );
		m_bScissorEnabled = true;
	}
}

void cImage::BindTexture( UINT iTex, UINT stage )
{
	if ( stage >= AGK_MAX_TEXTURES ) return;

	// check if we need to change texture
	if ( iTex == iCurrTexture[stage] ) return;

	glActiveTexture( GL_TEXTURE0 + stage );
	glBindTexture( GL_TEXTURE_2D, iTex );

	iCurrTexture[stage] = iTex;
}

void cImage::Bind( UINT stage )
{
	if ( stage > 7 ) return;
	
	UINT iTex = GetTextureID();

	// check if we need to change texture
	if ( iTex == iCurrTexture[stage] ) return;

	glActiveTexture( GL_TEXTURE0 + stage );
	if ( m_bIsCubeMap ) glBindTexture( GL_TEXTURE_CUBE_MAP, iTex );
	else glBindTexture( GL_TEXTURE_2D, iTex );

	iCurrTexture[stage] = iTex;
}

void cImage::UnBind()
{
	UINT iTex = GetTextureID();
    
	// check if we need to unbind texture
    for ( int i = 0; i < 8; i++ )
    {
        if ( iTex == iCurrTexture[i] )
        {
            glActiveTexture( GL_TEXTURE0 + i );
            if ( m_bIsCubeMap ) glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
            else glBindTexture( GL_TEXTURE_2D, 0 );
        
            iCurrTexture[i] = 0;
        }
    }
}

void cImage::PlatformSetSubData( int x, int y, int width, int height, unsigned char* pData )
{
	cImage::BindTexture( m_iTextureID );

	UINT glformat = GL_RGBA;
	if ( m_iImageMode == 2 ) 
	{
		glformat = GL_ALPHA;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, width,height, glformat, GL_UNSIGNED_BYTE, pData );
	if ( m_bMipmapped ) glGenerateMipmap(GL_TEXTURE_2D);
}

void cImage::PlatformGetDataFromScreen( unsigned int** pData, int x, int y, int width, int height )
{
	*pData = new UINT[ width*height ];
	glReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pData );
}

int cImage::GetRawDataFull( unsigned char** pData )
{
	if ( !pData ) return 0;
	if ( HasParent() ) return m_pParentImage->GetRawDataFull( pData );
	if ( m_iImageMode != 0 )
	{
		// can't get GL_ALPHA or depth textures back from the GPU on OpenGLES 2.0
		return 0;
	}
	
	if ( m_iTextureID == 0 ) return 0;

	UINT iTrueWidthSrc = GetTotalWidth();
	UINT iTrueHeightSrc = GetTotalHeight();

	int size = iTrueWidthSrc*iTrueHeightSrc*4;
	UINT glformat = GL_RGBA;
		
	*pData = new unsigned char[ size ];

	cImage::BindTexture( m_iTextureID );
    
	GLuint framebuf;
	glGenFramebuffers(1, &framebuf);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_iTextureID, 0 );
	glReadPixels( 0, 0, iTrueWidthSrc, iTrueHeightSrc, glformat, GL_UNSIGNED_BYTE, *pData );
	agk::BindDefaultFramebuffer();
	glDeleteFramebuffers( 1, &framebuf );

	// should only be true on Android
	if ( agk::IsSGX540GPU() )
	{
		// SGX540 doesn't like glFramebufferTexture2D it will fail after a certain number of calls
		// only way around it is to delete the texture that was used and recreate it
		cImage::BindTexture( 0 );
		glDeleteTextures( 1, &m_iTextureID );
		m_iTextureID = 0;
		if ( g_iLosingContext == 0 )
		{
			PlatformLoadFromData( iTrueWidthSrc, iTrueHeightSrc, (unsigned int*)(*pData) );
		}
	}

	return size;
}


void cImage::PlatformLoadFromData( int width, int height, UINT *bits )
{
	UINT glformat = GL_RGBA;
	UINT glbytesize = GL_UNSIGNED_BYTE;

	unsigned char *pData = (unsigned char*)bits;

	switch( m_iImageMode )
	{
		case 2: 
		{
			glformat = GL_ALPHA;
			break;
		}
	}

	if ( m_iTextureID == 0 ) glGenTextures( 1, &m_iTextureID );
	cImage::BindTexture( m_iTextureID );

	glTexImage2D(GL_TEXTURE_2D, 0, glformat, width, height, 0, glformat, glbytesize, pData );

	if ( m_bMipmapped )
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	if ( m_iWrapU == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	
	if ( m_iWrapV == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	if ( m_iMagFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if ( m_bMipmapped )
	{
		if ( m_iMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		if ( m_iMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}

void cImage::LoadSubRegionFromData( int x, int y, int width, int height, unsigned char *bits, int format )
{
	if ( m_iTextureID == 0 ) return;
	cImage::BindTexture( m_iTextureID );

	UINT glformat = GL_RGBA;
	UINT glbytesize = GL_UNSIGNED_BYTE;

	switch( m_iImageMode )
	{
		case 2: 
		{
			glformat = GL_ALPHA;
			break;
		}
	}

	glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, glformat, glbytesize, bits );
	SetCompressedPixelData( 0, 0 );
}

void cImage::PlatformCreateBlank( int width, int height, UINT format, UINT mipmap )
{
	m_iOrigWidth = width;
	m_iOrigHeight = height;
	m_iWidth = width;
	m_iHeight = height;
	m_bResized = false;

	m_fU1 = 0;
	m_fV1 = 0;
	m_fU2 = 1;
	m_fV2 = 1;

	if ( m_iTextureID == 0 ) glGenTextures( 1, &m_iTextureID );
	cImage::BindTexture( m_iTextureID );

	UINT glformat = GL_RGBA;
    UINT glformat2 = GL_RGBA;
	UINT glbytesize = GL_UNSIGNED_BYTE;
	switch( format )
	{
		case 1: 
		{
			if ( g_bDepth24Supported ) 
			{
				glformat = GL_DEPTH_COMPONENT;
                glformat2 = GL_DEPTH_COMPONENT;
				glbytesize = GL_UNSIGNED_INT;
                if ( g_bOpenGL3 ) glformat = GL_DEPTH_COMPONENT24;
			}
			else 
			{
				glformat = GL_DEPTH_COMPONENT;
                glformat2 = GL_DEPTH_COMPONENT;
				glbytesize = GL_UNSIGNED_SHORT;
                if ( g_bOpenGL3 ) glformat = GL_DEPTH_COMPONENT16;
			}
			break;
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, glformat, m_iWidth, m_iHeight, 0, glformat2, glbytesize, 0 );

	m_bMipmapped = mipmap > 0;
	if ( m_bMipmapped )
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
	if ( agk::m_iDefaultMagFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if ( m_bMipmapped )
	{
		if ( agk::m_iDefaultMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		if ( agk::m_iDefaultMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}

void cImage::GenerateMipmaps()
{
	if ( m_bMipmapped )
	{
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void cImage::PlatformSetMagFilter( UINT mode )
{
	Bind();
	if ( mode == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void cImage::PlatformSetMinFilter( UINT mode )
{
	Bind();
	if ( m_bMipmapped )
	{
		if ( mode == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		if ( mode == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}

void cImage::PlatformSetWrapU( UINT mode )
{
	Bind();
	if ( mode == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
}

void cImage::PlatformSetWrapV( UINT mode )
{
	Bind();
	if ( mode == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

int cImage::GetSupportsNPOT() 
{ 
	return 1;
}

int cImage::GetMaxTextureSize() 
{ 
	static GLint texMaxSize = 0;
	if ( texMaxSize == 0 ) glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texMaxSize);
	return texMaxSize;
}

void cImage::PlatformReloadFromData()
{
	if ( m_pParentImage ) return;

	UINT glformat = GL_RGBA;
	UINT glbytesize = GL_UNSIGNED_BYTE;
	uLong size = m_iWidth*m_iHeight*4;

	switch( m_iImageMode )
	{
		case 1: 
		{
			if ( g_bDepth24Supported ) 
			{
				glformat = GL_DEPTH_COMPONENT;
				glbytesize = GL_UNSIGNED_INT;
			}
			else 
			{
				glformat = GL_DEPTH_COMPONENT;
				glbytesize = GL_UNSIGNED_SHORT;
			}
			break;
		}
		case 2: 
		{
			glformat = GL_ALPHA;
			glbytesize = GL_UNSIGNED_BYTE;
			size = m_iWidth*m_iHeight;
			break;
		}
	}

	unsigned char* tempbuf = new unsigned char[ size ];
	if ( m_pCompressedPixelData )
	{
		int err = uncompress( tempbuf, &size, m_pCompressedPixelData, m_iCompressedLength );
		if ( err != Z_OK )
		{
			delete [] tempbuf;
			agk::Warning( "Failed to extract compressed image data" );
			return;
		}
	}
	else
	{
		// image mode 2 and 1 are expected not to have data as we can't get it from the GPU
		if ( m_iImageMode == 0 )
		{
			uString info; info.Format( "No data for image: %s", m_szFile.GetStr() );
			agk::Warning( info );
		}

		// if no data exists for the image create it as a blank image
		for( unsigned int i = 0; i < size; i++ )
		{
			tempbuf[ i ] = 0;
		}
	}

	// generate OepnGL texture
	if ( m_iTextureID == 0 ) glGenTextures( 1, &m_iTextureID );
	cImage::BindTexture( m_iTextureID );
		
	glTexImage2D(GL_TEXTURE_2D, 0, glformat, m_iWidth, m_iHeight, 0, glformat, glbytesize, tempbuf );
	
	if ( m_bMipmapped )
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	if ( m_iWrapU == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	
	if ( m_iWrapV == 0 ) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	else glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	if ( m_iMagFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if ( m_bMipmapped )
	{
		if ( m_iMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else
	{
		if ( m_iMinFilter == 0 ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		else glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	
	delete [] tempbuf;
}

void cImage::PlatformDelete()
{
	if ( m_iTextureID != 0 ) glDeleteTextures( 1, &m_iTextureID );
	m_iTextureID = 0;
}

//3D drawing
void agk::PlatformDeleteBuffer( UINT buffer )
{
	if ( m_iCurrentBoundVBO == buffer ) agk::PlatformBindBuffer( 0 );
	if ( m_iCurrentBoundIndexVBO == buffer ) agk::PlatformBindIndexBuffer( 0 );
	glDeleteBuffers( 1, &buffer );
}

void cObject3D::PlatformSetupDrawing()
{
	switch ( m_iTransparency )
	{
		case 0:	agk::PlatformSetBlendMode( 0 ); break;
		case 1:	agk::PlatformSetBlendMode( 1 ); break;
		case 2:	agk::PlatformSetBlendMode( 2 ); break;
		case 3: 
		{
			agk::PlatformSetBlendEnabled( 1 );
			agk::PlatformSetBlendFunc( m_iBlendModes & 0x0F, (m_iBlendModes >> 4) & 0x0F );
			break;
		}
		default: agk::PlatformSetBlendMode( 0 ); break;
	}

	agk::PlatformSetDepthTest( 1 );
	agk::PlatformSetDepthFunc( m_iZFunc );
	agk::PlatformSetDepthWrite( (m_iObjFlags & AGK_OBJECT_Z_WRITE) ? 1 : 0 );
	agk::PlatformSetCullMode( m_iCullMode );
	agk::PlatformSetDepthBias( m_fZBias );
	agk::PlatformSetDepthRange( m_fDepthNear, m_fDepthFar );
	
	AGKMatrix4 matWorld;
	
	matWorld.MakeWorld( rotFinal(), posFinal(), scaleFinal() );
	AGKShader::GetCurrentShader()->SetWorldMatrix( matWorld.GetFloatPtr() );

	// use inverse scale here in case the scale is non-uniform, also never invert the normal with negative scale,
	// using scale to turn the object inside out doesn't turn the polygons around, it only modifies their position
	AGKVector normScale = scaleFinal().GetInverse();
	if ( normScale.x < 0 ) normScale.x = -normScale.x;
	if ( normScale.y < 0 ) normScale.y = -normScale.y;
	if ( normScale.z < 0 ) normScale.z = -normScale.z;
	AGKMatrix3 matWorldNorm;
	matWorldNorm.MakeWorld( rotFinal(), normScale );
	AGKShader::GetCurrentShader()->SetNormalMatrix( matWorldNorm.GetFloatPtr() );
}

// mesh
void cMesh::PlatformGenBuffers()
{
	if ( m_iNumArrays == 0 ) return;
	if ( !m_iVBOVertices )
	{
		m_iVBOVertices = new UINT[ m_iNumArrays ];
	
		for ( UINT i = 0; i < m_iNumArrays; i++ )
		{
			glGenBuffers( 1, &(m_iVBOVertices[ i ]) );
			agk::PlatformBindBuffer( m_iVBOVertices[ i ] );
			glBufferData( GL_ARRAY_BUFFER, m_iNumVertices[ i ]*m_iVertexStride, m_ppVBOVertexData[ i ], GL_STATIC_DRAW );
		}
	}
	else
	{
		for ( UINT i = 0; i < m_iNumArrays; i++ )
		{
			agk::PlatformBindBuffer( m_iVBOVertices[ i ] );
			glBufferData( GL_ARRAY_BUFFER, m_iNumVertices[ i ]*m_iVertexStride, NULL, GL_STATIC_DRAW ); // discard the old contents in a GPU friendly manner
			glBufferData( GL_ARRAY_BUFFER, m_iNumVertices[ i ]*m_iVertexStride, m_ppVBOVertexData[ i ], GL_STATIC_DRAW );
		}
	}

	if ( m_ppIndices )
	{
		if ( !m_iVBOIndices )
		{
			m_iVBOIndices = new UINT[ m_iNumArrays ];

			for ( UINT i = 0; i < m_iNumArrays; i++ )
			{
				glGenBuffers( 1, &(m_iVBOIndices[ i ]) );
				agk::PlatformBindIndexBuffer( m_iVBOIndices[ i ] );
				if ( m_iFlags & AGK_MESH_UINT_INDICES ) glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(UINT), m_ppIndices[ i ], GL_STATIC_DRAW );
				else glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(unsigned short), m_ppIndices[ i ], GL_STATIC_DRAW );
			}
		}
		else
		{
			for ( UINT i = 0; i < m_iNumArrays; i++ )
			{
				agk::PlatformBindIndexBuffer( m_iVBOIndices[ i ] );
				if ( m_iFlags & AGK_MESH_UINT_INDICES ) 
				{
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(UINT), NULL, GL_STATIC_DRAW ); // discard the old contents in a GPU friendly manner
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(UINT), m_ppIndices[ i ], GL_STATIC_DRAW );
				}
				else 
				{
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(unsigned short), NULL, GL_STATIC_DRAW ); // discard the old contents in a GPU friendly manner
					glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_iNumIndices[ i ]*sizeof(unsigned short), m_ppIndices[ i ], GL_STATIC_DRAW );
				}
			}
		}
	}
}

void cMesh::PlatformDraw( int shadow, int alphamask )
{
	cMesh *pVertexOwner = this;
	if ( m_pSharedVertices ) pVertexOwner = m_pSharedVertices;

	if ( pVertexOwner->m_iNumArrays == 0 || !pVertexOwner->m_iVBOVertices )
	{
		static bool bWarned = false;
		if ( !bWarned ) agk::Warning( "Tried to draw a mesh that has no vertices" );
		bWarned = true;
		return;
	}

	AGKShader *pShader = AGKShader::GetCurrentShader();

	for ( UINT i = 0; i < pVertexOwner->m_iNumArrays; i++ )
	{
		agk::m_iVerticesProcessed += pVertexOwner->m_iNumVertices[ i ];

		agk::PlatformBindBuffer( pVertexOwner->m_iVBOVertices[ i ] );

		for ( unsigned char a = 0; a < pVertexOwner->m_iNumAttribs; a++ )
		{
			cVertexAttrib *pAttrib = pVertexOwner->m_pVertexAttribs[ a ];

			int shaderLoc = pAttrib->m_iShaderLoc;
			if ( m_pSharedVertices ) shaderLoc = pShader->GetAttribByName( pAttrib->m_sName );
			if ( shadow )
			{
				if ( strcmp(pAttrib->m_sName.GetStr(),"position") == 0 ) shaderLoc = pShader->GetPositionLoc();
				else if ( strcmp(pAttrib->m_sName.GetStr(),"boneindices") == 0 ) shaderLoc = pShader->GetBoneIndicesLoc();
				else if ( strcmp(pAttrib->m_sName.GetStr(),"boneweights") == 0 ) shaderLoc = pShader->GetBoneWeightsLoc();
				else if ( alphamask && strcmp(pAttrib->m_sName.GetStr(),"uv") == 0 ) shaderLoc = pShader->GetUVLoc();
				else continue;
			}

			if ( shaderLoc >= 0 ) 
			{
				UINT stride = 0;
				void* ptr = pAttrib->m_pData;
				if ( pAttrib->m_iOffset >= 0 ) 
				{
					stride = pVertexOwner->m_iVertexStride;
					ptr = (void*)(pAttrib->m_iOffset);
				}

				if ( pAttrib->m_iType == 0 ) pShader->SetAttribFloat( shaderLoc, pAttrib->m_iComponents, stride, (float*)ptr );
				else pShader->SetAttribUByte( shaderLoc, pAttrib->m_iComponents, stride, pAttrib->m_bNormalize, (unsigned char*)ptr );
			}
		}

		if ( !shadow && pVertexOwner->m_pDummyAttribs && i == 0 )
		{
			agk::PlatformBindBuffer( 0 );
			cDummyAttrib *pAttrib = pVertexOwner->m_pDummyAttribs;
			while( pAttrib )
			{
				int shaderLoc = pAttrib->m_iShaderLoc;
				if ( m_pSharedVertices ) shaderLoc = pShader->GetAttribByName( pAttrib->m_sName );
				if ( shaderLoc >= 0 )
				{
					// ATI doesn't seem to like glVertexAttrib4fv as a substitute for vertex arrays
					//glVertexAttrib4fv( pAttrib->m_iShaderLoc, &(pAttrib->m_pData[0]) );
					pShader->SetAttribUByte( shaderLoc, 4, 0, false, pAttrib->m_pData );
				}
				pAttrib = pAttrib->m_pNextAttrib;
			}
		}
		
		// Draw
		if ( pVertexOwner->m_iVBOIndices && pVertexOwner->m_iVBOIndices[ i ] ) 
		{
			agk::PlatformBindIndexBuffer( pVertexOwner->m_iVBOIndices[ i ] );

			if ( !shadow )
			{
				if ( m_iPrimitiveType == AGK_TRIANGLES ) agk::m_iPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] / 3;
				else if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP ) agk::m_iPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] - 2;
			}
			else
			{
				if ( m_iPrimitiveType == AGK_TRIANGLES ) agk::m_iShadowPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] / 3;
				else if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP ) agk::m_iShadowPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] - 2;
			}

			if ( m_iFlags & AGK_MESH_UINT_INDICES )
			{
				pShader->DrawIndicesInt( pVertexOwner->m_iNumIndices[ i ], 0, m_iPrimitiveType );
			}
			else
			{
				pShader->DrawIndices( pVertexOwner->m_iNumIndices[ i ], 0, m_iPrimitiveType );
			}
		}
		else 
		{
			agk::PlatformBindIndexBuffer( 0 );

			if ( !shadow ) agk::m_iPolygonsDrawn += pVertexOwner->m_iNumVertices[ i ] / 3;
			else agk::m_iShadowPolygonsDrawn += pVertexOwner->m_iNumVertices[ i ] / 3;

			pShader->DrawTriangles( 0, pVertexOwner->m_iNumVertices[ i ] );
		}
	}
}

// shaders
void AGKShader::PlatformInit() 
{ 
	m_iShaderID = glCreateProgram();
}

void AGKShader::PlatformDelete() 
{ 
	if ( m_iShaderID ) glDeleteProgram( m_iShaderID ); 
	m_iShaderID = 0;
}

void AGKShader::MakeActive()
{
	if ( g_pCurrentShader != this ) 
	{
		// record which attribs are currently on
		if ( g_pCurrentShader ) 
		{
			cShaderAttrib *pAttrib = g_pCurrentShader->m_cAttribList.GetFirst();
			while( pAttrib )
			{
				if ( pAttrib->m_iLocation >= 0 && g_iAttributeActive[ pAttrib->m_iLocation ] > 0 ) 
				{
					g_iAttributeActive[ pAttrib->m_iLocation ] = 2;
					m_bFlags |= AGK_SHADER_ATTRIBUTES_CHANGED;
				}
				pAttrib = g_pCurrentShader->m_cAttribList.GetNext();
			}
		}

		// change shader
		glUseProgram( m_iShaderID );
	}
	g_pCurrentShader = this;

	g_pCurrentShader->UpdateUniforms();

	UpdateMatrices();
	UpdateAGKUniforms();

	// set global light variables
	if ( agk::m_cDirectionalLight.m_active )
	{
		g_pCurrentShader->SetTempConstantByName( "agk_DLightDir", agk::m_cDirectionalLight.m_direction.x, agk::m_cDirectionalLight.m_direction.y, agk::m_cDirectionalLight.m_direction.z, 0 );
		g_pCurrentShader->SetTempConstantByName( "agk_DLightColor", agk::m_cDirectionalLight.m_color.x, agk::m_cDirectionalLight.m_color.y, agk::m_cDirectionalLight.m_color.z, 0 );
	}
	else
	{
		g_pCurrentShader->SetTempConstantByName( "agk_DLightColor", 0, 0, 0, 0 );
	}

	g_pCurrentShader->SetTempConstantByName( "agk_LightAmbient", agk::m_AmbientColor.x, agk::m_AmbientColor.y, agk::m_AmbientColor.z, 0 );
}

void AGKShader::NoShader()
{
	if ( g_pCurrentShader ) 
	{
		for ( int i = 0; i < AGK_MAX_ATTRIBUTES; i++ )
		{
			if ( g_iAttributeActive[ i ] > 0 ) glDisableVertexAttribArray( i );
			g_iAttributeActive[ i ] = 0;
		}
		glUseProgram( 0 );
	}
	g_pCurrentShader = 0;
}

void AGKShader::SetAttribFloat( UINT location, int size, int stride, const float* ptr )
{
	if ( g_pCurrentShader != this ) MakeActive();
	glVertexAttribPointer( location, size, GL_FLOAT, GL_FALSE, stride, (void*) ptr );
	if ( g_iAttributeActive[ location ] == 0 ) glEnableVertexAttribArray( location );
	g_iAttributeActive[ location ] = 1;
}

void AGKShader::SetAttribUByte( UINT location, int size, int stride, bool normalise, const unsigned char* ptr )
{
	if ( g_pCurrentShader != this ) MakeActive();
	glVertexAttribPointer( location, size, GL_UNSIGNED_BYTE, normalise ? GL_TRUE : GL_FALSE, stride, (void*) ptr );
	if ( g_iAttributeActive[ location ] == 0 ) glEnableVertexAttribArray( location );
	g_iAttributeActive[ location ] = 1;
}

void AGKShader::PlatformDrawPrimitives( UINT primitive, int first, UINT count )
{
	// set old attributes off if they are no longer used
	if ( (m_bFlags & AGK_SHADER_ATTRIBUTES_CHANGED) != 0 )
	{
		for ( int i = 0; i < AGK_MAX_ATTRIBUTES; i++ )
		{
			if ( g_iAttributeActive[ i ] == 2 ) 
			{
				glDisableVertexAttribArray( i );
				g_iAttributeActive[ i ] = 0;
			}
		}

		m_bFlags &= ~AGK_SHADER_ATTRIBUTES_CHANGED;
	}

	GLenum glprimitive = GL_TRIANGLES;
	switch( primitive )
	{
		case AGK_TRIANGLES: glprimitive = GL_TRIANGLES; break;
		case AGK_TRIANGLE_STRIP: glprimitive = GL_TRIANGLE_STRIP; break;
		case AGK_TRIANGLE_FAN: glprimitive = GL_TRIANGLE_FAN; break;
		case AGK_LINE_LOOP: glprimitive = GL_LINE_LOOP; break;
		case AGK_LINES: glprimitive = GL_LINES; break;
		case AGK_POINTS: glprimitive = GL_POINTS; break;
	}

	glDrawArrays( glprimitive, first, count ); 
}

void AGKShader::PlatformDrawIndices( UINT primitive, UINT count, unsigned short *pIndices )
{
	// set old attributes off if they are no longer used
	if ( (m_bFlags & AGK_SHADER_ATTRIBUTES_CHANGED) != 0 )
	{
		for ( int i = 0; i < AGK_MAX_ATTRIBUTES; i++ )
		{
			if ( g_iAttributeActive[ i ] == 2 ) 
			{
				glDisableVertexAttribArray( i );
				g_iAttributeActive[ i ] = 0;
			}
		}

		m_bFlags &= ~AGK_SHADER_ATTRIBUTES_CHANGED;
	}

	GLenum glprimitive = GL_TRIANGLES;
	switch( primitive )
	{
		case AGK_TRIANGLES: glprimitive = GL_TRIANGLES; break;
		case AGK_TRIANGLE_STRIP: glprimitive = GL_TRIANGLE_STRIP; break;
		case AGK_TRIANGLE_FAN: glprimitive = GL_TRIANGLE_FAN; break;
		case AGK_LINE_LOOP: glprimitive = GL_LINE_LOOP; break;
		case AGK_LINES: glprimitive = GL_LINES; break;
		case AGK_POINTS: glprimitive = GL_POINTS; break;
	}

	glDrawElements( glprimitive, count, GL_UNSIGNED_SHORT, pIndices );
}

void AGKShader::PlatformDrawIndicesInt( UINT primitive, UINT count, unsigned int *pIndices )
{
	// set old attributes off if they are no longer used
	if ( (m_bFlags & AGK_SHADER_ATTRIBUTES_CHANGED) != 0 )
	{
		for ( int i = 0; i < AGK_MAX_ATTRIBUTES; i++ )
		{
			if ( g_iAttributeActive[ i ] == 2 ) 
			{
				glDisableVertexAttribArray( i );
				g_iAttributeActive[ i ] = 0;
			}
		}

		m_bFlags &= ~AGK_SHADER_ATTRIBUTES_CHANGED;
	}

	GLenum glprimitive = GL_TRIANGLES;
	switch( primitive )
	{
		case AGK_TRIANGLES: glprimitive = GL_TRIANGLES; break;
		case AGK_TRIANGLE_STRIP: glprimitive = GL_TRIANGLE_STRIP; break;
		case AGK_TRIANGLE_FAN: glprimitive = GL_TRIANGLE_FAN; break;
		case AGK_LINE_LOOP: glprimitive = GL_LINE_LOOP; break;
		case AGK_LINES: glprimitive = GL_LINES; break;
		case AGK_POINTS: glprimitive = GL_POINTS; break;
	}

	glDrawElements( glprimitive, count, GL_UNSIGNED_INT, pIndices );
}

void AGKShader::SetTempConstantByName( const char* szName, float f1, float f2, float f3, float f4 )
{
	if ( !szName ) return;
	if ( g_pCurrentShader != this ) MakeActive(); // can only set temp constants whilst the shader is active

	cShaderUniform *pUniform = m_cUniformList.GetItem( szName );
	if ( !pUniform ) return;
	if ( pUniform->m_iType != 0 )
	{
		agk::Error( "Failed to set shader constant - tried to set vector values on a matrix" );
		return;
	}

	switch( pUniform->m_iComponents )
	{
		case 1: glUniform1f( pUniform->m_iLocation, f1 ); break;
		case 2: glUniform2f( pUniform->m_iLocation, f1, f2 ); break;
		case 3: glUniform3f( pUniform->m_iLocation, f1, f2, f3 ); break;
		case 4: glUniform4f( pUniform->m_iLocation, f1, f2, f3, f4 ); break;
	}

	// set it as changed so the default value gets put back later
	if ( !pUniform->m_bChanged )
	{
		pUniform->m_bChanged = true;
		pUniform->m_pNextUniform = m_pChangedUniforms;
		m_pChangedUniforms = pUniform;
	}
}

void AGKShader::SetTempConstantArrayByName( const char* szName, UINT index, float f1, float f2, float f3, float f4 )
{
	if ( !szName ) return;
	if ( g_pCurrentShader != this ) MakeActive(); // can only set temp constants whilst the shader is active

	cShaderUniform *pUniform = m_cUniformList.GetItem( szName );
	if ( !pUniform ) return;
	if ( pUniform->m_iType != 0 )
	{
		agk::Error( "Failed to set shader constant - tried to set vector values on a matrix" );
		return;
	}

	if ( index >= pUniform->m_iArrayMembers ) return;

	switch( pUniform->m_iComponents )
	{
		case 1: glUniform1f( pUniform->m_iLocation + index, f1 ); break;
		case 2: glUniform2f( pUniform->m_iLocation + index, f1, f2 ); break;
		case 3: glUniform3f( pUniform->m_iLocation + index, f1, f2, f3 ); break;
		case 4: glUniform4f( pUniform->m_iLocation + index, f1, f2, f3, f4 ); break;
	}

	// set it as changed so the default value gets put back later
	if ( !pUniform->m_bChanged )
	{
		pUniform->m_bChanged = true;
		pUniform->m_pNextUniform = m_pChangedUniforms;
		m_pChangedUniforms = pUniform;
	}
}

void AGKShader::SetTempConstantArrayByName( const char* szName, const float* values )
{
	if ( !szName ) return;
	if ( g_pCurrentShader != this ) MakeActive(); // can only set temp variables whilst the shader is active

	cShaderUniform *pUniform = m_cUniformList.GetItem( szName );
	if ( !pUniform ) return;
	if ( pUniform->m_iType != 0 )
	{
		agk::Error( "Failed to set shader variable - tried to set vector values on a matrix" );
		return;
	}

	switch( pUniform->m_iComponents )
	{
		case 1: glUniform1fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, values ); break;
		case 2: glUniform2fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, values ); break;
		case 3: glUniform3fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, values ); break;
		case 4: glUniform4fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, values ); break;
	}

	// set it as changed so the default value gets put back later
	if ( !pUniform->m_bChanged )
	{
		pUniform->m_bChanged = true;
		pUniform->m_pNextUniform = m_pChangedUniforms;
		m_pChangedUniforms = pUniform;
	}
}

void AGKShader::SetTempConstantMatrixByName( const char* szName, const float* values )
{
	if ( !szName ) return;
	if ( g_pCurrentShader != this ) MakeActive(); // can only set temp constants whilst the shader is active

	cShaderUniform *pUniform = m_cUniformList.GetItem( szName );
	if ( !pUniform ) return;
	if ( pUniform->m_iType != 1 )
	{
		agk::Error( "Failed to set shader constant - tried to set matrix values on a vector" );
		return;
	}

	switch( pUniform->m_iComponents )
	{
		case 2: glUniformMatrix2fv( pUniform->m_iLocation, 1, GL_FALSE, values ); break;
		case 3: glUniformMatrix3fv( pUniform->m_iLocation, 1, GL_FALSE, values ); break;
		case 4: glUniformMatrix4fv( pUniform->m_iLocation, 1, GL_FALSE, values ); break;
	}

	// set it as changed so the default value gets put back later
	if ( !pUniform->m_bChanged )
	{
		pUniform->m_bChanged = true;
		pUniform->m_pNextUniform = m_pChangedUniforms;
		m_pChangedUniforms = pUniform;
	}
}

void AGKShader::SetTempConstantMatrixArrayByName( const char* szName, UINT index, const float* values )
{
	if ( !szName ) return;
	if ( g_pCurrentShader != this ) MakeActive(); // can only set temp constants whilst the shader is active

	cShaderUniform *pUniform = m_cUniformList.GetItem( szName );
	if ( !pUniform ) return;
	if ( pUniform->m_iType != 1 )
	{
		agk::Error( "Failed to set shader constant - tried to set matrix values on a vector" );
		return;
	}

	if ( index >= pUniform->m_iArrayMembers ) return;

	switch( pUniform->m_iComponents )
	{
		case 2: glUniformMatrix2fv( pUniform->m_iLocation + index, 1, GL_FALSE, values ); break;
		case 3: glUniformMatrix3fv( pUniform->m_iLocation + index, 1, GL_FALSE, values ); break;
		case 4: glUniformMatrix4fv( pUniform->m_iLocation + index, 1, GL_FALSE, values ); break;
	}

	// set it as changed so the default value gets put back later
	if ( !pUniform->m_bChanged )
	{
		pUniform->m_bChanged = true;
		pUniform->m_pNextUniform = m_pChangedUniforms;
		m_pChangedUniforms = pUniform;
	}
}

void AGKShader::UpdateUniforms()
{
	cShaderUniform *pLast = 0;
	cShaderUniform *pUniform = m_pChangedUniforms;
	while ( pUniform )
	{
		if ( pUniform->m_iType == 0 )
		{
			// scalar/vector
			switch( pUniform->m_iComponents )
			{
				case 1: glUniform1fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, pUniform->m_pValues ); break;
				case 2: glUniform2fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, pUniform->m_pValues ); break;
				case 3: glUniform3fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, pUniform->m_pValues ); break;
				case 4: glUniform4fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, pUniform->m_pValues ); break;
			}
		}
		else
		{
			// matrix
			switch( pUniform->m_iComponents )
			{
				case 2: glUniformMatrix2fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, GL_FALSE, pUniform->m_pValues ); break;
				case 3: glUniformMatrix3fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, GL_FALSE, pUniform->m_pValues ); break;
				case 4: glUniformMatrix4fv( pUniform->m_iLocation, pUniform->m_iArrayMembers, GL_FALSE, pUniform->m_pValues ); break;
			}
		}
		
		pLast = pUniform;
		pUniform = pUniform->m_pNextUniform;
		
		// remove it from the changed list
		pLast->m_pNextUniform = 0;
		pLast->m_bChanged = false;
	}

	// clear changed list
	m_pChangedUniforms = 0;
}

void AGKShader::UpdateAGKUniforms()
{
	if ( m_iAGKTime >= 0 ) glUniform1f( m_iAGKTime, agk::Timer() );
	if ( m_iAGKSinTime >= 0 ) glUniform1f( m_iAGKSinTime, agk::SinRad(agk::Timer()) );
	if ( m_iAGKResolution >= 0 ) 
	{
		if ( agk::m_bUsingFBO ) glUniform2f( m_iAGKResolution, (float) agk::m_iFBOWidth, (float) agk::m_iFBOHeight );
		else glUniform2f( m_iAGKResolution, (float) agk::GetDeviceWidth(), (float) agk::GetDeviceHeight() );
	}
	if ( m_iAGKInvert >= 0 ) 
	{
		if ( agk::m_bUsingFBO ) glUniform1f( m_iAGKInvert, -1 );
		else glUniform1f( m_iAGKInvert, 1 );
	}
	if ( m_iAGKCameraPos >= 0 && agk::m_pCurrentCamera ) glUniform3f( m_iAGKCameraPos, agk::m_pCurrentCamera->GetX(), agk::m_pCurrentCamera->GetY(), agk::m_pCurrentCamera->GetZ() );
	if ( m_iAGKShadowParams >= 0 ) 
	{
		if ( agk::m_fShadowRange > 0 ) glUniform4f( m_iAGKShadowParams, agk::m_fShadowRange, agk::m_fShadowBias, 1.0f/agk::m_iShadowMapWidth, 1.0f/agk::m_iShadowMapHeight );
		else glUniform4f( m_iAGKShadowParams, agk::m_pCurrentCamera->GetFarRange(), agk::m_fShadowBias, 1.0f/agk::m_iShadowMapWidth, 1.0f/agk::m_iShadowMapHeight );
	}
	if ( m_iAGKShadowParams2 >= 0 ) glUniform4f( m_iAGKShadowParams2, g_fShadowParams2[0], g_fShadowParams2[1], g_fShadowParams2[2], g_fShadowParams2[3] );
}

void AGKShader::UpdateMatrices()
{
	if ( agk::m_pCurrentCamera && (agk::m_pCurrentCamera->HasUpdated() || agk::m_pCurrentCamera != m_pCurrentCamera) )
	{
		AGKMatrix4 view;
		agk::m_pCurrentCamera->GetViewMatrix( view );
		SetViewMatrix( view.GetFloatPtr() );
	}

	if ( agk::m_pCurrentCamera && (agk::m_pCurrentCamera->HasProjUpdated() || agk::m_pCurrentCamera != m_pCurrentCamera) )
	{
		const AGKMatrix4 *proj = agk::m_pCurrentCamera->GetProjMatrix();
		SetProjMatrix( &(proj->mat[0][0]) );

		m_pCurrentCamera = agk::m_pCurrentCamera;
	}

	// check if anything has changed
	bool bWorldChanged = (m_bFlags & AGK_SHADER_WORLD_CHANGED) != 0;
	bool bViewChanged = (m_bFlags & AGK_SHADER_VIEW_CHANGED) != 0;
	bool bProjChanged = (m_bFlags & AGK_SHADER_PROJ_CHANGED) != 0;
	bool bOrthoChanged = (m_bFlags & AGK_SHADER_ORTHO_CHANGED) != 0;
	bool bNormChanged = (m_bFlags & AGK_SHADER_WORLDNORMAL_CHANGED) != 0;
	bool bShadowChanged = (m_bFlags & AGK_SHADER_SHADOW_PROJ_CHANGED) != 0;

	// check if we should update everything anyway
	if ( (m_bFlags & AGK_SHADER_RELOAD_UNIFORMS) != 0 )
	{
		bWorldChanged = true;
		bViewChanged = true;
		bProjChanged = true;
		bOrthoChanged = true;
		bNormChanged = true;
		bShadowChanged = true;

		// texture samplers only need to be updated once, do it here for simplicity
		for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
		{
			glUniform1i( m_iTexture2D[ i ], i );
			glUniform1i( m_iTextureCube[ i ], i );
		}

		if ( m_iShadowMapTex >= 0 ) glUniform1i( m_iShadowMapTex, 7 );
		if ( m_iShadowMap2Tex >= 0 ) glUniform1i( m_iShadowMap2Tex, 6 );
		if ( m_iShadowMap3Tex >= 0 ) glUniform1i( m_iShadowMap3Tex, 5 );
		if ( m_iShadowMap4Tex >= 0 ) glUniform1i( m_iShadowMap4Tex, 4 );

		m_bTextureBoundsChanged = 0xffffffff;
		m_bUVBoundsChanged = 0xffffffff;

		m_bFlags &= ~AGK_SHADER_RELOAD_UNIFORMS;
	}

	// check if texture UV offsets changed
	for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
	{
		UINT index = 1 << i;
		if ( (m_bTextureBoundsChanged & index) && m_iTextureBounds[ i ] >= 0 )
		{
			glUniform4f( m_iTextureBounds[ i ], m_fTextureU2[ i ]-m_fTextureU1[ i ], m_fTextureV2[ i ]-m_fTextureV1[ i ], m_fTextureU1[ i ], m_fTextureV1[ i ] );
		}
		m_bTextureBoundsChanged &= ~index;

		if ( (m_bUVBoundsChanged & index) && m_iUVBounds[ i ] >= 0 )
		{
			glUniform4f( m_iUVBounds[ i ], m_fU2[ i ]-m_fU1[ i ], m_fV2[ i ]-m_fV1[ i ], m_fU1[ i ], m_fV1[ i ] );
		}
		m_bUVBoundsChanged &= ~index;
	}

	if ( m_iUniformWorldMat >= 0 && bWorldChanged ) glUniformMatrix4fv( m_iUniformWorldMat, 1, GL_FALSE, &m_matWorld.mat[0][0] );
	if ( m_iUniformViewMat >= 0 && bViewChanged ) glUniformMatrix4fv( m_iUniformViewMat, 1, GL_FALSE, &m_matView.mat[0][0] );
	if ( bProjChanged && m_iUniformProjMat >= 0 ) glUniformMatrix4fv( m_iUniformProjMat, 1, GL_FALSE, &m_matProj.mat[0][0] );
	if ( bOrthoChanged && m_iUniformOrthoMat >= 0 ) glUniformMatrix4fv( m_iUniformOrthoMat, 1, GL_FALSE, &g_matOrtho.mat[0][0] );
	if ( m_iUniformNormalMat >= 0 && bNormChanged ) glUniformMatrix3fv( m_iUniformNormalMat, 1, GL_FALSE, &m_matWorldNormal.mat[0][0] );
	if ( m_iUniformShadowProjMat >= 0 && bShadowChanged ) glUniformMatrix4fv( m_iUniformShadowProjMat, 1, GL_FALSE, &g_matShadowProj.mat[0][0] );
	if ( m_iUniformShadowProj2Mat >= 0 && bShadowChanged ) glUniformMatrix4fv( m_iUniformShadowProj2Mat, 1, GL_FALSE, &g_matShadow2Proj.mat[0][0] );
	if ( m_iUniformShadowProj3Mat >= 0 && bShadowChanged ) glUniformMatrix4fv( m_iUniformShadowProj3Mat, 1, GL_FALSE, &g_matShadow3Proj.mat[0][0] );
	if ( m_iUniformShadowProj4Mat >= 0 && bShadowChanged ) glUniformMatrix4fv( m_iUniformShadowProj4Mat, 1, GL_FALSE, &g_matShadow4Proj.mat[0][0] );

	if ( m_iUniformVPMat >= 0 && (bViewChanged || bProjChanged) ) 
	{
		AGKMatrix4 vp = m_matView;
		vp.Mult( m_matProj );
		glUniformMatrix4fv( m_iUniformVPMat, 1, GL_FALSE, vp.GetFloatPtr() );
	}

	if ( m_iUniformWVPMat >= 0 && (bWorldChanged || bViewChanged || bProjChanged) ) 
	{
		AGKMatrix4 wvp = m_matWorld;
		wvp.Mult( m_matView );
		wvp.Mult( m_matProj );
		glUniformMatrix4fv( m_iUniformWVPMat, 1, GL_FALSE, &wvp.mat[0][0] );
	}

	if ( m_iUniformWVOMat >= 0 && (bWorldChanged || bOrthoChanged) ) 
	{
		AGKMatrix4 wvo = m_matWorld;
		wvo.Mult( g_matOrtho );
		glUniformMatrix4fv( m_iUniformWVOMat, 1, GL_FALSE, &wvo.mat[0][0] );
	}

	UINT mask = AGK_SHADER_WORLD_CHANGED | AGK_SHADER_VIEW_CHANGED | AGK_SHADER_PROJ_CHANGED 
		      | AGK_SHADER_WORLDNORMAL_CHANGED | AGK_SHADER_ORTHO_CHANGED | AGK_SHADER_SHADOW_PROJ_CHANGED;
	m_bFlags &= ~mask;
}

int agk::PlatformSupportsPSHighp()
{
	int range[2];
	int precision;
	glGetShaderPrecisionFormat( GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision );
	if ( range[0] > 0 ) return 1;
	return 0;
}

void AGKShader::SetShaderSource( const char* vertex, const char* fragment )
{
	GLint logLength=0, logLength2=0, status=0;
	GLuint vertShader=0, fragShader=0;

	m_sVSLog.SetStr( "" );
	m_sPSLog.SetStr( "" );

	m_sVSSource.SetStr( vertex );
	m_sPSSource.SetStr( fragment );

	if ( !vertex || !fragment || !*vertex || !*fragment )
	{
		m_bValid = false;
		return;
	}

	// check device capabilities
	int range[2];
	int precision;
	glGetShaderPrecisionFormat( GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision );
	if ( range[0] > 0 )
	{
#ifdef AGK_ANDROID
		// Nexus 5 reports high precision and fails if we try to use mediump (uses lowp instead) so we must use highp
		// This may cause performance problems, but we must use highp because at least 2 devices have the mediump bug!
		m_sPSSource.ReplaceStr( "mediump ", "highp " );

		// also change any shared uniforms in the vertex shader as this may cause the linker to fail
		m_sVSSource.ReplaceStr( "mediump float agk_", "highp float agk_" );
		m_sVSSource.ReplaceStr( "mediump vec2 agk_", "highp vec2 agk_" );
		m_sVSSource.ReplaceStr( "mediump vec3 agk_", "highp vec3 agk_" );
		m_sVSSource.ReplaceStr( "mediump vec4 agk_", "highp vec4 agk_" );
#endif
	}
	else
	{
		static int warned = 0;
		if ( !warned )
		{
			warned = 1;
			agk::Warning( "Device does not support high precision pixel shader values" );
		}
		m_sPSSource.ReplaceStr( "highp ", "mediump " );

		// also change any shared uniforms in the vertex shader as this may cause the linker to fail
		m_sVSSource.ReplaceStr( "highp float agk_", "mediump float agk_" );
		m_sVSSource.ReplaceStr( "highp vec2 agk_", "mediump vec2 agk_" );
		m_sVSSource.ReplaceStr( "highp vec3 agk_", "mediump vec3 agk_" );
		m_sVSSource.ReplaceStr( "highp vec4 agk_", "mediump vec4 agk_" );
	}		

	// add precision if not already present
	int iVSCopied = 0;
	char *vertex2 = (char*)m_sVSSource.GetStr();
	if ( m_sVSSource.FindStr("#if") <= 0 )
	{
		iVSCopied = 1;
		vertex2 = new char[ m_sVSSource.GetLength() + 100 ];
		strcpy( vertex2, "precision highp float;\n" );
		strcat( vertex2, m_sVSSource.GetStr() );
	}

	int iPSCopied = 0;
	char *fragment2 = (char*)m_sPSSource.GetStr();
	if ( m_sPSSource.FindStr("#if") <= 0 )
	{
		iPSCopied = 1;
		fragment2 = new char[ m_sPSSource.GetLength() + 100 ];
#ifdef AGK_ANDROID
		if ( range[0] > 0 ) strcpy( fragment2, "precision highp float;\n" );
		else strcpy( fragment2, "precision mediump float;\n" );
#else
		strcpy( fragment2, "precision mediump float;\n" );
#endif
		strcat( fragment2, m_sPSSource.GetStr() );
	}

	// vertex shader
	vertShader = glCreateShader ( GL_VERTEX_SHADER );
	glShaderSource ( vertShader, 1, (const char**)&vertex2, NULL );
	glCompileShader ( vertShader );

	if ( iVSCopied ) delete [] vertex2;

	glGetShaderiv ( vertShader, GL_COMPILE_STATUS, &status );
	if ( status == 0 )
	{
		glGetShaderiv ( vertShader, GL_INFO_LOG_LENGTH, &logLength );
		if ( logLength > 1 )
		{
			char *log = (char*)malloc(logLength+1);
			glGetShaderInfoLog(vertShader, logLength, &logLength2, log);
			m_sVSLog.SetStr( log );
			free(log);
		}

		glDeleteShader ( vertShader );
		uString info;
		info.Format( "Vertex shader %s failed to compile: %s", m_sVSFilename.GetStr(), m_sVSLog.GetStr() );
		agk::Error( info );
		agk::Message( info );
		m_bValid = false;
		return;
	}

	// fragment shader
	fragShader = glCreateShader ( GL_FRAGMENT_SHADER );
	glShaderSource ( fragShader, 1, (const char**)&fragment2, NULL );
	glCompileShader ( fragShader );

	if ( iPSCopied ) delete [] fragment2;

	glGetShaderiv ( fragShader, GL_COMPILE_STATUS, &status );
	if ( status == 0 )
	{
		glGetShaderiv ( fragShader, GL_INFO_LOG_LENGTH, &logLength );
		if ( logLength > 1 )
		{
			char *log = (char*)malloc(logLength+1);
			glGetShaderInfoLog(fragShader, logLength, &logLength2, log);
			m_sPSLog.SetStr( log );
			free(log);
		}

		glDeleteShader ( vertShader );
		glDeleteShader ( fragShader );
		uString info;
		info.Format( "Pixel shader %s failed to compile: %s", m_sPSFilename.GetStr(), m_sPSLog.GetStr() );
		agk::Error( info );
		agk::Message( info );
		m_bValid = false;
		return;
	}

	// attach shaders to program
	glAttachShader ( m_iShaderID, vertShader );
	glAttachShader ( m_iShaderID, fragShader );

	// link program
	glLinkProgram ( m_iShaderID );
	
	glGetProgramiv ( m_iShaderID, GL_LINK_STATUS, &status);
	if ( status == 0 )
	{
        if ( vertShader ) { glDeleteShader ( vertShader ); vertShader=0; }
		if ( fragShader ) { glDeleteShader ( fragShader ); fragShader=0; }

		glGetProgramiv ( m_iShaderID, GL_INFO_LOG_LENGTH, &logLength);
		if ( logLength > 0 )
		{
			char *log = (char*)malloc(logLength);
			glGetProgramInfoLog ( m_iShaderID, logLength, &logLength, log);
			m_sLinkLog.SetStr( log );
			free(log);
		}
		uString info;
		info.Format( "Vertex shader %s and pixel shader %s failed to link: %s", m_sVSFilename.GetStr(), m_sPSFilename.GetStr(), m_sLinkLog.GetStr() );
		agk::Error( info );
		agk::Message( info );
		m_bValid = false;
		return;
	}

	if ( m_bReloading )
	{
		if ( vertShader ) glDeleteShader ( vertShader );
		if ( fragShader ) glDeleteShader ( fragShader );
		m_bValid = true;
		return;
	}

	m_pChangedUniforms = 0;
	cShaderUniform* pUniform = m_cUniformList.GetFirst();
	while ( pUniform )
	{
		delete pUniform;
		pUniform = m_cUniformList.GetNext();
	}
	m_cUniformList.ClearAll();

	cShaderAttrib* pAttrib = m_cAttribList.GetFirst();
	while ( pAttrib )
	{
		delete pAttrib;
		pAttrib = m_cAttribList.GetNext();
	}
	m_cAttribList.ClearAll();

	// get uniform locations
	m_iUniformWorldMat = glGetUniformLocation ( m_iShaderID, "agk_World" );
	m_iUniformNormalMat = glGetUniformLocation ( m_iShaderID, "agk_WorldNormal" );
	m_iUniformViewMat = glGetUniformLocation ( m_iShaderID, "agk_View" );
	m_iUniformProjMat = glGetUniformLocation ( m_iShaderID, "agk_Proj" );
	m_iUniformOrthoMat = glGetUniformLocation ( m_iShaderID, "agk_Ortho" );
	m_iUniformVPMat = glGetUniformLocation ( m_iShaderID, "agk_ViewProj" );
	m_iUniformWVPMat = glGetUniformLocation ( m_iShaderID, "agk_WorldViewProj" );
	m_iUniformWVOMat = glGetUniformLocation ( m_iShaderID, "agk_WorldOrtho" );
	m_iUniformShadowProjMat = glGetUniformLocation ( m_iShaderID, "agk_ShadowProj" );
	m_iUniformShadowProj2Mat = glGetUniformLocation ( m_iShaderID, "agk_Shadow2Proj" );
	m_iUniformShadowProj3Mat = glGetUniformLocation ( m_iShaderID, "agk_Shadow3Proj" );
	m_iUniformShadowProj4Mat = glGetUniformLocation ( m_iShaderID, "agk_Shadow4Proj" );

	int numUniforms = 0;
	int maxLength = 0;
	int length = 0;
	int size = 0;
	UINT type = 0;
	glGetProgramiv( m_iShaderID, GL_ACTIVE_UNIFORMS, &numUniforms );
	glGetProgramiv( m_iShaderID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength );
	char* szUniformName = new char[ maxLength+1 ];
	
	m_iNumUniforms = 0;
	for ( int i = 0; i < numUniforms; i++ )
	{
		glGetActiveUniform( m_iShaderID, i, maxLength, &length, &size, &type, szUniformName );
		int location = glGetUniformLocation( m_iShaderID, szUniformName );

		int newSize = 0;
		int newType = 0;
		switch( type )
		{
			case GL_FLOAT: newSize = 1; break;
			case GL_FLOAT_VEC2: newSize = 2; break;
			case GL_FLOAT_VEC3: newSize = 3; break;
			case GL_FLOAT_VEC4: newSize = 4; break;
			case GL_FLOAT_MAT2: newSize = 2; newType = 1; break;
			case GL_FLOAT_MAT3: newSize = 3; newType = 1; break;
			case GL_FLOAT_MAT4: newSize = 4; newType = 1; break;
			default:
			{
				/*
				uString info;
				info.Format( "Shader constant \"%s\" is not supported. Only float, vec2, vec3, vec4 are currently allowed.", szUniformName );
				agk::Message( info );
				*/
				continue;
				break;
			}
		}

		cShaderUniform *pNewUniform = new cShaderUniform();
		pNewUniform->m_iComponents = newSize;
		pNewUniform->m_iLocation = location;
		pNewUniform->m_iType = newType;
		pNewUniform->m_iArrayMembers = size;
		pNewUniform->m_sName.SetStr( szUniformName );
		pNewUniform->m_sName.Trunc( '[' );
		if ( newType == 0 ) 
		{
			pNewUniform->m_pValues = new float[ size*newSize ];
			for ( int i = 0; i < size*newSize; i++ ) pNewUniform->m_pValues[ i ] = 0;
		}
		else 
		{
			pNewUniform->m_pValues = new float[ size*newSize*newSize ];
			for ( int i = 0; i < size*newSize*newSize; i++ ) pNewUniform->m_pValues[ i ] = 0;
		}
		
		m_cUniformList.AddItem( pNewUniform, pNewUniform->m_sName );
		m_iNumUniforms++;
	}

	delete [] szUniformName;

	m_iAGKTime = glGetUniformLocation( m_iShaderID, "agk_time" );
	m_iAGKSinTime = glGetUniformLocation( m_iShaderID, "agk_sintime" );
	m_iAGKResolution = glGetUniformLocation( m_iShaderID, "agk_resolution" );
	m_iAGKInvert = glGetUniformLocation( m_iShaderID, "agk_invert" );
	m_iAGKCameraPos = glGetUniformLocation( m_iShaderID, "agk_CameraPos" );
	m_iAGKShadowParams = glGetUniformLocation( m_iShaderID, "agk_ShadowParams" );
	m_iAGKShadowParams2 = glGetUniformLocation( m_iShaderID, "agk_ShadowParams2" );

	if ( glGetUniformLocation( m_iShaderID, "agk_spritepos" ) >= 0 ) m_bFlags |= AGK_SHADER_PER_SPRITE_UNIFORM;
	if ( glGetUniformLocation( m_iShaderID, "agk_spritesize" ) >= 0 ) m_bFlags |= AGK_SHADER_PER_SPRITE_UNIFORM;
		
	uString name;
	for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
	{
		name.Format( "texture%d", i );
		m_iTexture2D[ i ] = glGetUniformLocation( m_iShaderID, name.GetStr() );
		name.Format( "CubeMap%d", i );
		m_iTextureCube[ i ] = glGetUniformLocation( m_iShaderID, name.GetStr() );
		name.Format( "uvBounds%d", i );
		m_iUVBounds[ i ] = glGetUniformLocation( m_iShaderID, name.GetStr() );
		name.Format( "textureBounds%d", i );
		m_iTextureBounds[ i ] = glGetUniformLocation( m_iShaderID, name.GetStr() );
	}

	m_iShadowMapTex = glGetUniformLocation( m_iShaderID, "shadowMap" );
	m_iShadowMap2Tex = glGetUniformLocation( m_iShaderID, "shadowMap2" );
	m_iShadowMap3Tex = glGetUniformLocation( m_iShaderID, "shadowMap3" );
	m_iShadowMap4Tex = glGetUniformLocation( m_iShaderID, "shadowMap4" );

	int numAttribs = 0;
	glGetProgramiv( m_iShaderID, GL_ACTIVE_ATTRIBUTES, &numAttribs );
	glGetProgramiv( m_iShaderID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength );
	char* szAttribName = new char[ maxLength+1 ];
	for ( int i = 0; i < numAttribs; i++ )
	{
		glGetActiveAttrib( m_iShaderID, i, maxLength, &length, &size, &type, szAttribName );
		int location = glGetAttribLocation( m_iShaderID, szAttribName );

		//if ( strcmp( szAttribName, "position" ) == 0 ) m_iPositionAttrib = location;
		//if ( strcmp( szAttribName, "normal" ) == 0 ) m_iNormalAttrib = location;
		//if ( strcmp( szAttribName, "color" ) == 0 ) m_iColorAttrib = location;
		//if ( strcmp( szAttribName, "texcoord" ) == 0 ) m_iTexCoordAttrib = location;

		int newSize = 0;
		int newType = 0;
		switch( type )
		{
			case GL_FLOAT: newSize = 1; break;
			case GL_FLOAT_VEC2: newSize = 2; break;
			case GL_FLOAT_VEC3: newSize = 3; break;
			case GL_FLOAT_VEC4: newSize = 4; break;
			case GL_FLOAT_MAT2: newSize = 2; newType = 1; break;
			case GL_FLOAT_MAT3: newSize = 3; newType = 1; break;
			case GL_FLOAT_MAT4: newSize = 4; newType = 1; break;
			default:
			{
				uString info;
				info.Format( "Shader attribute \"%s\" is not supported. Non-square matrices are a likely cause", szAttribName );
				agk::Message( info );
				continue;
				break;
			}
		}

		m_iNumAttribs++;

		if ( strcmp(szAttribName, "position") == 0 ) m_iPositionLoc = location;
		else if ( strcmp(szAttribName, "normal") == 0 ) m_iNormalLoc = location;
		else if ( strcmp(szAttribName, "tangent") == 0 ) m_iTangentLoc = location;
		else if ( strcmp(szAttribName, "binormal") == 0 ) m_iBiNormalLoc = location;
		else if ( strcmp(szAttribName, "color") == 0 ) m_iColorLoc = location;
		else if ( strcmp(szAttribName, "uv") == 0 ) m_iTexCoordLoc = location;
		else if ( strcmp(szAttribName, "boneweights") == 0 ) m_iBoneWeightsLoc = location;
		else if ( strcmp(szAttribName, "boneindices") == 0 ) m_iBoneIndicesLoc = location;

		cShaderAttrib *pNewAttrib = new cShaderAttrib();
		pNewAttrib->m_iComponents = newSize;
		pNewAttrib->m_iLocation = location;
		pNewAttrib->m_iType = newType;
		pNewAttrib->m_sName.SetStr( szAttribName );
		m_cAttribList.AddItem( pNewAttrib, szAttribName );
		
		/*
		uString info;
		info.Format( "location: %d, name: %s, type: %d, size: %d", location, szAttribName, type, size );
		agk::Message( info );
		*/
	}

	delete [] szAttribName;

	// release shaders now program created okay
	if ( vertShader ) glDeleteShader ( vertShader );
	if ( fragShader ) glDeleteShader ( fragShader );

	m_bValid = true;
}

// Frame Buffer Objects

void FrameBuffer::PlatformCreateFrameBuffer( cImage *pColor, cImage *pDepth, bool forceDepth )
{
	if ( m_iFBO != 0 ) PlatformDeleteFrameBuffer();

	glGenFramebuffers( 1, &m_iFBO );
	Bind();
	
	if ( pColor )
	{
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pColor->GetTextureID(), 0 );
	}

	if ( pDepth )
	{
		if ( g_bDepthTextureSupported ) 
		{
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pDepth->GetTextureID(), 0 );
		}
		else
		{
			agk::Warning( "Depth texture not support on this device, using an internal depth buffer instead" );
			glGenRenderbuffers( 1, &m_iRBODepth );
			glBindRenderbuffer( GL_RENDERBUFFER, m_iRBODepth );
			glRenderbufferStorage( GL_RENDERBUFFER, g_bDepth24Supported ? 0x81A6 : GL_DEPTH_COMPONENT16, pColor ? pColor->GetTotalWidth() : 32, pColor ? pColor->GetTotalHeight() : 32 );
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_iRBODepth );
		}
	}
	else
	{
		if ( forceDepth )
		{
			glGenRenderbuffers( 1, &m_iRBODepth );
			glBindRenderbuffer( GL_RENDERBUFFER, m_iRBODepth );
			glRenderbufferStorage( GL_RENDERBUFFER, g_bDepth24Supported ? 0x81A6 : GL_DEPTH_COMPONENT16, pColor ? pColor->GetTotalWidth() : 32, pColor ? pColor->GetTotalHeight() : 32 );
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_iRBODepth );
		}
	}

	GLenum result = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if ( result != GL_FRAMEBUFFER_COMPLETE )
	{
		switch( result )
		{
			case GL_FRAMEBUFFER_UNSUPPORTED: agk::Message( "GL_FRAMEBUFFER_UNSUPPORTED" ); break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: agk::Message( "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" ); break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: agk::Message( "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" ); break;
			//case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: agk::Message( "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" ); break;
			//case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: agk::Message( "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" ); break;
			default: 
			{
				uString err = "Framebuffer error: ";
				err += result;
				agk::Message( err );
			}
		}
	}

	agk::BindDefaultFramebuffer();
}

void FrameBuffer::PlatformDeleteFrameBuffer()
{
	if ( m_iFBO ) 
	{
		glDeleteFramebuffers( 1, &m_iFBO );
		m_iFBO = 0;
	}

	if ( m_iRBODepth )
	{
		glDeleteRenderbuffers( 1, &m_iRBODepth );
		m_iRBODepth = 0;
	}
}

void FrameBuffer::PlatformBind()
{
	glBindFramebuffer( GL_FRAMEBUFFER, m_iFBO );
}
