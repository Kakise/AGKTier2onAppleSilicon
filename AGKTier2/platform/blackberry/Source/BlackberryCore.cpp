#include "agk.h"
#include <pthread.h>
#ifdef USE_BOX2D
	#include "DebugDraw.h"
	#include "Box2D/Box2D.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bps/locale.h>
#include <bps/navigator.h>
#include <bps/dialog.h>
#include <bps/netstatus.h>
#include <bps/geolocation.h>
#include <bps/deviceinfo.h>
#include <bps/accelerometer.h>
#include <bps/sensor.h>

#include "png.h"

#include "jpeglib.h"
#include "jpegint.h"

extern unsigned char libImageAscii[];
extern unsigned char libImageAsciiExt[];

int agk::m_iKeyboardMode = 2; // virtual keyboard

namespace AGK
{
 	INT64 uFixTime = 0;
	float fFixTime2 = 0;
	bool g_bOpenGL2 = false;

	EGLDisplay g_pDisplay = 0;
	EGLSurface g_pSurface = 0;
	screen_window_t screen_win = 0;

	int g_iUpdateSurface = 0;
	int g_iNewSurfaceWidth = 0;
	int g_iNewSurfaceHeight = 0;

	GLuint defaultFramebuffer;

	struct egldata
	{
		EGLDisplay display;
		EGLSurface surface;
		screen_window_t screen_win;
	};

	// message dialog
	char g_szWindowGroup[ 128 ] = "";

	// music
	mmr_connection_t *g_mediaConnection = 0;
	mmr_context_t *g_mediaContext = 0;
	int g_audioID = 0;

	// sounds
	ALCcontext	*audioContext = NULL;
	ALCdevice	*audioDevice = NULL;
	
	// video
	mmr_connection_t *g_videoConnection = 0;
	mmr_context_t *g_videoContext = 0;
	int g_videoOutputID = -1;
	int g_audioOutputID = -1;
	int g_videoPlaying = 0;
	int g_videoPosition = 0;
	screen_window_t g_videoWindow = 0;
	uString g_sVideoFile;
	cLock g_videoLock;
	float g_videoPositionTime = 0;

	struct videoData
	{
		int type;
		void* window;
		int position;
	};

	class cSoundInst
	{
		public:
			UINT m_iID;
			UINT m_uLastUsed;
			int m_iParent;
			int m_iVolume;
			int m_iLoop;
			float m_fRate;
			float m_fBalance;
			int m_iLoopCount;
			cSoundInst *m_pPrevInst;
			cSoundInst *m_pNextInst;

			ALuint bufferID;
			ALuint sourceID;
		
			cSoundInst() 
			{ 
				m_iID = 0;
				m_uLastUsed = 0;
				m_iParent = 0;
				m_iVolume = 100;
				m_iLoop = 0;
				m_fRate = 1.0f;
				m_fBalance = 0;
				m_iLoopCount = 0;
				m_pPrevInst = UNDEF;
				m_pNextInst = UNDEF;
			}
			
			~cSoundInst()
			{

			}
		
			void Reset()
			{
				m_iParent = 0;
				m_iLoop = 0;
				m_fRate = 1.0f;
				m_fBalance = 0;
				m_iLoopCount = 0;
				if( sourceID )
				{
					alSourceStop(sourceID);
					alDeleteSources(1, &sourceID);
				}
				if ( bufferID )
				{
					alDeleteBuffers(1, &bufferID);
				}
				sourceID = 0;
				bufferID = 0;
			}
	};
	
	cHashedList<cSoundInst> m_cSoundInstances(256);

	char szAppFolderName[ MAX_PATH ] = "";
	char szBaseDir[ MAX_PATH ] = "";
	char szRootDir[ MAX_PATH ] = "";
	char szWriteDir[ MAX_PATH ] = "";
	char szOriginalWriteDir[ MAX_PATH ] = "";
	
	/*
	NSBundle *g_pBundle = NULL;
	NSFileManager *g_pFileManager = NULL;
	*/
	
	// text input variables
	int g_iTextCursorPos = 0;
	cSprite *g_pTextInputCursor = 0;
	cSprite *g_pTextInputArea = 0;
	cSprite *g_pTextInputBack = 0;
	cText *g_pTextInputText = 0;
	float g_fCursorBlinkTime = 0;
	int g_iShowCursor = 1;
	float g_fTextStartX = 0;
	float g_fTextStartY = 0;
	bool g_bPasswordMode = false;
	
	cSprite *pTextBackground = 0;
	bool g_bEditBoxHack = false;

	// advert variables
	bool g_bAdvertActive = false;
	int g_iAdvertHorz = 0;
	int g_iAdvertVert = 0;
	float g_fAdvertOffsetX = 0;
	float g_fAdvertOffsetY = 0;

	// facebook variables
	uString m_sFBAppID;
	uString m_sAccessToken;
	cHTTPConnection *m_pFacebookConnection = 0;
	int m_iFacebookGettingFriends = 0;
	int m_iFBGettingPicture = 0;
	uString m_sFBLocalFile;
	uString m_sFBUserID;
	uString m_sFBName;
	int m_iFBUserIDStarted = 0;

	struct FacebookUser
	{
		uString userID;
		uString name;
	};

	FacebookUser *m_pFBFriends = 0;
	int m_iFBFriendCount = 0;
}

using namespace AGK;

void cFileEntry::TraverseDirectory( const char* dir )
{
	// todo
	/*
	DIR *pDir = opendir( dir );
	if ( pDir )
	{
		dirent* item = readdir( pDir );
		while( item )
		{
			//uString sDir;
			//sDir.Format( "%s - %d", item->d_name, item->d_type );
			//agk::Warning( sDir );

			if ( item->d_type == DT_DIR )
			{
				// directory
				if ( strcmp( item->d_name, "." ) != 0 && strcmp( item->d_name, ".." ) != 0 )
				{
					char str[ 1024 ];
					strcpy( str, dir );
					strcat( str, item->d_name );
					AddNewFile( str );

					strcat( str, "/" );
					TraverseDirectory( str );
				}
			}
			else if ( item->d_type == DT_REG )
			{
				// file
				char str[ 1024 ];
				strcpy( str, dir );
				strcat( str, item->d_name );
				AddNewFile( str );
			}

			item = readdir( pDir );
		}

		closedir( pDir );
	}
	*/
}

void cFileEntry::InitFileList()
{
	//TraverseDirectory( szRootDir ); //can't get asset folder directories so root is case sensitive only
	TraverseDirectory( szWriteDir );
}

void agk::SetWindowPosition( int x, int y )
{
	// do nothing on blackberry
}

void agk::SetWindowSize( int width, int height, int fullscreen )
{
	// do nothing on blackberry
}

void agk::SetWindowAllowResize( int mode )
{
	// do nothing on blackberry
}

void agk::MinimizeApp()
{
	// how to do this on Blackberry?
}

void agk::RestoreApp()
//****
{
	// do nothing
}

void agk::SetImmersiveMode( int mode )
{
	
}

void agk::SetScreenResolution( int width, int height )
//****
{
	if ( width < 0 ) width = 0;
	if ( height < 0 ) height = 0;
	if ( width > agk::GetMaxDeviceWidth() ) width = agk::GetMaxDeviceWidth();
	if ( height > agk::GetMaxDeviceHeight() ) height = agk::GetMaxDeviceHeight();

	g_iNewSurfaceWidth = width;
	g_iNewSurfaceHeight = height;
	g_iUpdateSurface = 1;
}

void agk::GetDeviceName( uString &outString )
//****
{
	outString.SetStr( "blackberry" );
}

char* agk::GetDeviceName( )
//****
{
	char *str = new char[ 15 ];
	strcpy( str, "blackberry" );
	return str;
}

char* agk::GetDeviceBaseName( )
//****
{
	char *str = new char[ 15 ];
	strcpy( str, "blackberry" );
	return str;
}

char* agk::GetDeviceType( )
//****
{
	// todo
	deviceinfo_data_t data;
	deviceinfo_get_data(&data);
	char *str = new char[ 128 ];
	strcpy( str, data.device_os );
	deviceinfo_free_data(&data);
	return str;
}

void agk::GetAppName( uString &outString )
//****
{
	outString.SetStr( m_sAppName );
}

char* agk::GetAppName( )
//****
{
	char *str = new char[ m_sAppName.GetLength()+1 ];
	strcpy( str, m_sAppName.GetStr() );
	return str;
}

bool agk::GetDeviceHasSplash()
{
	//if ( agk::GetFileExists( "Default.jpg" ) ) return true;
	return false;
}

char* agk::GetDeviceLanguage( )
//****
{
	char* language = 0;
	char* country = 0;
	locale_get( &language, &country );

	char *str = new char[ strlen(language)+1 ];
	strcpy( str, language );

	bps_free( language );
	bps_free( country );

	return str;
}

void agk::SetSleepMode( int mode )
//****
{
	int idle_mode = SCREEN_IDLE_MODE_KEEP_AWAKE;
	if ( mode == 1 ) idle_mode = SCREEN_IDLE_MODE_NORMAL;
	screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_IDLE_MODE, &idle_mode);
}

void agk::SetAntialiasMode( int mode )
//****
{
	//if ( mode == 1 ) glEnable( GL_MULTISAMPLE );
	//else glDisable( GL_MULTISAMPLE );
}

void agk::SetExpansionFileKey( const char *key )
//****
{
	// do nothing
}

void agk::SetExpansionFileVersion(int version)
{
	// do nothing on blackberry
}

int agk::GetExpansionFileState()
{
	return 0;
}

void agk::DownloadExpansionFile()
{
	// do nothing on blackberry
}

float agk::GetExpansionFileProgress()
{
	return 0;
}

void agk::SetWindowTitle( const char *szTitle )
{

}

bool agk::GetDeviceCanRotate()
{
	return true;
}

bool agk::GetAGKShouldRotate()
{
	return false;
}

void agk::PlatformSetOrientationAllowed( int portrait, int portrait2, int landscape, int landscape2 )
{
	screen_display_t screen_disp;
	int rc = screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_DISPLAY, (void **)&screen_disp);

	int screen_resolution[2];
	rc = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, screen_resolution);

	int angle = atoi(getenv("ORIENTATION"));

	bool isTablet = true;
	if ( screen_resolution[0] < screen_resolution[1] && (angle == 0 || angle == 180) ) isTablet = false;
	if ( screen_resolution[0] > screen_resolution[1] && (angle == 90 || angle == 270) ) isTablet = false;

	if ( isTablet )
	{
		//agk::Warning( "Rotating Tablet" );

		int change = 0;
		switch( angle )
		{
			case 0: if ( !landscape ) change = 1; break;
			case 90: if ( !portrait ) change = 1; break;
			case 180: if ( !landscape2 ) change = 1; break;
			case 270: if ( !portrait2 ) change = 1; break;
		}

		if ( change )
		{
			if ( portrait ) navigator_set_orientation( 90, 0 );
			else if ( portrait2 ) navigator_set_orientation( 270, 0 );
			else if ( landscape ) navigator_set_orientation( 0, 0 );
			else if ( landscape2 ) navigator_set_orientation( 180, 0 );
		}
	}
	else
	{

		//agk::Warning( "Rotating Phone" );
		int change = 0;
		switch( angle )
		{
			case 0: if ( !portrait ) change = 1; break;
			case 90: if ( !landscape ) change = 1; break;
			case 180: if ( !portrait2 ) change = 1; break;
			case 270: if ( !landscape2 ) change = 1; break;
		}

		if ( change )
		{
			if ( portrait ) navigator_set_orientation( 0, 0 );
			else if ( portrait2 ) navigator_set_orientation( 180, 0 );
			else if ( landscape ) navigator_set_orientation( 90, 0 );
			else if ( landscape2 ) navigator_set_orientation( 270, 0 );
		}

	}
}

bool agk::PlatformGetDeviceID( uString &out )
{
	// requesting the device PIN permission is probably not desired in most cases
	//deviceinfo_identifying_data_t data;
	//deviceinfo_get_identifying_data( &data );
	//out.Format( "%d", data.pin );
	return false;
}

float agk::PlatformDeviceScale()
{
	float max1 = m_iIntendedWidth;
	if ( m_iIntendedHeight > max1 ) max1 = m_iIntendedHeight;
	if ( max1 <= 0 ) return  1.0f;

	float max2 = m_iRenderWidth;
	if ( m_iRenderHeight > max2 ) max2 = m_iRenderHeight;
	if ( max2 <= 0 ) return  1.0f;

	float scale = 1.0f;
	while ( max1 / max2 >= 1.6 )
	{
		scale /= 2.0f;
		max2 *= 2;
	}

	if ( scale < 0.25f ) scale = 0.25f;
	return scale;
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

int agk::PlatformGetNumProcessors()
{
	// todo
	return 1;
}

struct AGKData
{
	int width;
	int height;
};

void agk::PlatformInitFilePaths()
{
	getcwd( szBaseDir, MAX_PATH );

	strcpy( szAppFolderName, "Unknown" );
	m_sAppName.SetStr( "Unknown" );
	
	strcpy( szRootDir, szBaseDir );
	strcat( szRootDir, "/app/native/assets/" );

	strcpy( szWriteDir, szBaseDir );
	strcat( szWriteDir, "/data/" );
	
	// make sure this value is set here incase restore is called without changing path
	strcpy( szOriginalWriteDir, szWriteDir );

	m_bUpdateFileLists = true;
}

void agk::PlatformUpdateWritePath()
{
	if ( m_sAppName.GetLength() == 0 )
	{
		PlatformRestoreWritePath();
		return;
	}

	// get my documents folder
	strcpy( szWriteDir, szBaseDir );
	strcat( szWriteDir, "/data/" );
	strcat( szWriteDir, m_sAppName );
	strcat( szWriteDir, "/" );
	
	chdir( szBaseDir );
	chdir( "data" );
	if ( chdir( m_sAppName ) < 0 )
	{
		mkdir( m_sAppName, 0777 );
		chdir( m_sAppName );
	}
	
	chdir( szBaseDir );

	m_bUpdateFileLists = true;
}

void agk::PlatformRestoreWritePath()
{
	if ( strlen ( szOriginalWriteDir ) > 0 ) strcpy( szWriteDir, szOriginalWriteDir );
	
	m_bUpdateFileLists = true;
}

void agk::InitJoysticks()
{
	
}

void agk::PlatformSetDevicePtr( void* ptr )
{
	egldata *ePtr = (egldata*)ptr;

	g_pDisplay = ePtr->display;
	g_pSurface = ePtr->surface;
	screen_win = ePtr->screen_win;
}

bool g_bDepthTextureSupported = false;
bool g_bDepth24Supported = false;

void agk::PlatformInitGL( void* ptr )
{
	egldata *ePtr = (egldata*)ptr;

	g_pDisplay = ePtr->display;
	g_pSurface = ePtr->surface;

	eglQuerySurface(g_pDisplay, g_pSurface, EGL_WIDTH, &m_iRenderWidth);
	eglQuerySurface(g_pDisplay, g_pSurface, EGL_HEIGHT, &m_iRenderHeight);
	cCamera::UpdateAllAspectRatio( m_iRenderWidth/(float)m_iRenderHeight );

	int screen_resolution[2];
	int rc = screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_SIZE, screen_resolution);
	m_iRealDeviceWidth = screen_resolution[0];
	m_iRealDeviceHeight = screen_resolution[1];

	if ( m_iRenderWidth > m_iRenderHeight && m_iRealDeviceWidth < m_iRealDeviceHeight )
	{
		m_iRealDeviceWidth = screen_resolution[1];
		m_iRealDeviceHeight = screen_resolution[0];
	}

	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	uFixTime = now.tv_sec;
	fFixTime2 = now.tv_nsec * 1e-9;
	SetRandomSeed( uFixTime + (now.tv_nsec % 1000) );
	
	const char *szVersion = (const char*) glGetString( GL_VERSION );
	if ( strncmp( szVersion, "1.", 2 ) == 0 ) g_bOpenGL2 = false;
	else g_bOpenGL2 = true;
	
	const char* extensions = (const char*) glGetString( GL_EXTENSIONS );
	if ( strstr( extensions, "OES_depth_texture " ) ) g_bDepthTextureSupported = true;
	if ( strstr( extensions, "OES_depth24 " ) ) g_bDepth24Supported = true;
	if ( strstr( extensions, "OES_element_index_uint " ) ) m_iCapFlags |= AGK_CAP_UINT_INDICES;

	cImage::GetMaxTextureSize(); // preloads the value so that any threads don't call the GL function behind it

	//agk::Warning( extensions );
	//agk::Warning( "" );

	glDisable( GL_SCISSOR_TEST );
	glDisable( GL_STENCIL_TEST );

	AGKShader::CreateDefaultShaders();

	// sensors
	if ( accelerometer_is_supported() ) agk::m_iAccelerometerExists = 1;
	else agk::m_iAccelerometerExists = 0;

	if ( sensor_is_supported(SENSOR_TYPE_GYROSCOPE) )
	{
		agk::m_iGyroSensorExists = 1;
		sensor_request_events( SENSOR_TYPE_GYROSCOPE );
	}
	else agk::m_iGyroSensorExists = 0;

	if ( sensor_is_supported(SENSOR_TYPE_MAGNETOMETER) )
	{
		agk::m_iMagneticSensorExists = 1;
		sensor_request_events( SENSOR_TYPE_MAGNETOMETER );
	}
	else agk::m_iMagneticSensorExists = 0;

	if ( sensor_is_supported(SENSOR_TYPE_LIGHT) )
	{
		agk::m_iLightSensorExists = 1;
		sensor_request_events( SENSOR_TYPE_LIGHT );
	}
	else agk::m_iLightSensorExists = 0;

	if ( sensor_is_supported(SENSOR_TYPE_PROXIMITY) )
	{
		agk::m_iProximitySensorExists = 1;
		sensor_request_events( SENSOR_TYPE_PROXIMITY );
	}
	else agk::m_iProximitySensorExists = 0;

	if ( sensor_is_supported(SENSOR_TYPE_ROTATION_VECTOR) )
	{
		agk::m_iRotationSensorExists = 1;
		sensor_request_events( SENSOR_TYPE_ROTATION_VECTOR );
	}
	else agk::m_iRotationSensorExists = 0;

	// all known Blacjberry 10 and Playbook devices have location API of some form, and no way to check otherwise
	m_iGPSSensorExists = 1;

	// text input setup
	float DevToVirX = 1.0f;
	float DevToVirY = 1.0f;
	if ( agk::m_fTargetViewportWidth > 0 ) DevToVirX = agk::GetVirtualWidth() / agk::m_fTargetViewportWidth;
	if ( agk::m_fTargetViewportHeight > 0 )  DevToVirY = agk::GetVirtualHeight() / agk::m_fTargetViewportHeight;
	
	float width = 250 * DevToVirX;
	float height = 22 * DevToVirY;
	
	if ( width > agk::GetVirtualWidth() ) width = (float) agk::GetVirtualWidth();
	
	g_fTextStartX = (agk::GetVirtualWidth() - width)/2.0f + 3*DevToVirX;
	g_fTextStartY = agk::GetVirtualHeight()/3.0f + 2*DevToVirY;
	
	g_pTextInputCursor = new cSprite();
	g_pTextInputCursor->SetSize( 2 * DevToVirX, 18 * DevToVirY );
	g_pTextInputCursor->SetColor( 102, 213, 255, 255 );
	g_pTextInputCursor->SetPosition( g_fTextStartX, g_fTextStartY );
	g_pTextInputCursor->SetOffset( 0,0 );
	g_pTextInputCursor->FixToScreen(1);
	
	g_pTextInputArea = new cSprite();
	g_pTextInputArea->SetSize( width, height );
	g_pTextInputArea->SetColor( 255,255,255,255 );
	g_pTextInputArea->SetPosition( (agk::GetVirtualWidth() - width) / 2.0f, agk::GetVirtualHeight()/3.0f );
	g_pTextInputArea->SetOffset( 0,0 );
	g_pTextInputArea->FixToScreen(1);
	
	width += 8 * DevToVirX;
	height += 8 * DevToVirY;
	
	g_pTextInputBack = new cSprite();
	g_pTextInputBack->SetSize( width, height );
	g_pTextInputBack->SetColor( 190,190,190,255 );
	g_pTextInputBack->SetPosition( (agk::GetVirtualWidth() - width) / 2.0f, agk::GetVirtualHeight()/3.0f - 4*DevToVirY );
	g_pTextInputBack->SetOffset( 0,0 );
	g_pTextInputBack->FixToScreen(1);
	
	g_pTextInputText = new cText(30);
	g_pTextInputText->SetPosition( g_fTextStartX, g_fTextStartY );
	g_pTextInputText->SetColor( 0,0,0 );
	g_pTextInputText->SetSpacing( 1 );
	g_pTextInputText->FixToScreen(1);
	
	cImage *pArial = new cImage( "/Arial.png" );
	g_pTextInputText->SetFontImage( pArial );
	
	pTextBackground = new cSprite();
	pTextBackground->SetColor( 0,0,0, 128 );
	pTextBackground->SetPosition( -m_iDisplayExtraX, -m_iDisplayExtraY );
	pTextBackground->SetSize( m_iDisplayWidth+m_iDisplayExtraX*2, m_iDisplayHeight+m_iDisplayExtraY*2 );
	pTextBackground->FixToScreen(1);
}

void agk::PlatformInitConsole()
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	uFixTime = now.tv_sec;
	fFixTime2 = now.tv_nsec * 1e-9;
	SetRandomSeed( uFixTime + (now.tv_nsec % 1000) );
}

UINT agk::IsSupportedDepthTexture()
//*****
{
	return g_bDepthTextureSupported ? 1 : 0;
}

void agk::PlatformDestroyGL ( void )
{
	
}

void agk::UpdatePtr( void *ptr )
{
	egldata *ePtr = (egldata*)ptr;

	g_pDisplay = ePtr->display;
	g_pSurface = ePtr->surface;

	eglQuerySurface(g_pDisplay, g_pSurface, EGL_WIDTH, &m_iRenderWidth);
	eglQuerySurface(g_pDisplay, g_pSurface, EGL_HEIGHT, &m_iRenderHeight);
	cCamera::UpdateAllAspectRatio( m_iRenderWidth/(float)m_iRenderHeight );

	int screen_resolution[2];
	int rc = screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_SIZE, screen_resolution);
	m_iRealDeviceWidth = screen_resolution[0];
	m_iRealDeviceHeight = screen_resolution[1];

	if ( m_iRenderWidth > m_iRenderHeight && m_iRealDeviceWidth < m_iRealDeviceHeight )
	{
		m_iRealDeviceWidth = screen_resolution[1];
		m_iRealDeviceHeight = screen_resolution[0];
	}

	RecalculateDisplay();

	agk::SetVideoDimensions( m_fVideoX, m_fVideoY, m_fVideoWidth, m_fVideoHeight );
}

void agk::UpdatePtr2( void *ptr )
{
	egldata *ePtr = (egldata*)ptr;

	g_pDisplay = ePtr->display;
	g_pSurface = ePtr->surface;
}

int agk::GetInternalDataI( int index )
{
	switch( index )
	{
		case 1: // does surface need updating
		{
			int temp = g_iUpdateSurface;
			g_iUpdateSurface = 0;
			return temp;
		}
		case 2: // new surface width
		{
			return g_iNewSurfaceWidth;
		}
		case 3: // new surface height
		{
			return g_iNewSurfaceHeight;
		}
		default: return 0;
	}
}

void agk::WindowMoved()
{

}

void agk::PlatformSetProjectionMatrix()
{

}

void agk::SetOrientationMatrix()
{

}

void agk::PlatformPrepareDebugDraw()
{
	agk::PlatformSetDepthTest( 0 );
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
	glFrontFace( GL_CCW );
}

void agk::PlatformEndDebugDraw()
{
	agk::PlatformSetDepthTest( 1 );
}

void agk::PlatformPrepare2DStates()
{

}

void agk::PlatformPrepare3DStates()
{

}

void agk::PlatformSetBlendMode( int mode )
{
	switch( mode )
	{
		case 0: // opaque
		{
			agk::PlatformSetDepthWrite( 1 );
			agk::PlatformSetDepthFunc( AGK_DEPTH_LESS );
			agk::PlatformSetBlendEnabled( 0 );
			break;
		}

		case 1: // alpha blending
		{
			//glDepthMask( GL_FALSE ); // must not draw depth when alpha blending as overlapping polygons will disappear
			////glDepthMask( GL_TRUE ); // need to write Z pixel as otherwise unculled sweetie renders badly!
			//glDepthFunc( GL_LEQUAL );
			//glEnable( GL_BLEND );
			//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			agk::PlatformSetDepthWrite( 0 );
			agk::PlatformSetDepthFunc( AGK_DEPTH_LEQUAL );
			agk::PlatformSetBlendEnabled( 1 );
			agk::PlatformSetBlendFunc( AGK_BLEND_SRC_ALPHA, AGK_BLEND_ONE_MINUS_SRC_ALPHA );
			break;
		}

		case 2: // additive blending
		{
			//glDepthMask( GL_FALSE );
			//glDepthFunc( GL_LEQUAL );
			//glEnable( GL_BLEND );
			//glBlendFunc( GL_ONE, GL_ONE );

			agk::PlatformSetDepthWrite( 0 );
			agk::PlatformSetDepthFunc( AGK_DEPTH_LEQUAL );
			agk::PlatformSetBlendEnabled( 1 );
			agk::PlatformSetBlendFunc( AGK_BLEND_ONE, AGK_BLEND_ONE );
			break;
		}
	}
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

	//glBlendFunc( final1, final2 );
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

void agk::SetVSync( int mode )
{
	if ( !g_pDisplay ) return;
	if ( mode < 0 ) mode = 0;

	eglSwapInterval( g_pDisplay, mode );
}

void agk::Sleep( UINT milliseconds )
//****
{
	// convert from milliseconds to microseconds
	usleep( milliseconds*1000 );
}

void agk::PlatformCleanUp()
{
	agk::StopSound(0); // stop all
	AGKMusicOGG::DeleteAll();

    if ( audioContext ) alcDestroyContext( audioContext );
	if ( audioDevice ) alcCloseDevice( audioDevice );
}

int agk::GetMaxDeviceWidth()
//****
{
	return m_iRealDeviceWidth;
}

int agk::GetMaxDeviceHeight()
//****
{
	return m_iRealDeviceHeight;
}

void agk::PlatformUpdateDeviceSize()
{
	if ( g_pDisplay == NULL || g_pSurface == NULL ) return;

	eglQuerySurface(g_pDisplay, g_pSurface, EGL_WIDTH, &m_iRenderWidth);
	eglQuerySurface(g_pDisplay, g_pSurface, EGL_HEIGHT, &m_iRenderHeight);
	cCamera::UpdateAllAspectRatio( m_iRenderWidth/(float)m_iRenderHeight );

	int screen_resolution[2];
	int rc = screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_SIZE, screen_resolution);
	m_iRealDeviceWidth = screen_resolution[0];
	m_iRealDeviceHeight = screen_resolution[1];

	if ( m_iRenderWidth > m_iRenderHeight && m_iRealDeviceWidth < m_iRealDeviceHeight )
	{
		m_iRealDeviceWidth = screen_resolution[1];
		m_iRealDeviceHeight = screen_resolution[0];
	}

	//uString str;
	//str.Format( "Width: %d, Height: %d", m_iRenderWidth, m_iRenderHeight );
	//agk::Warning( str );
}

void agk::PlatformUpdateTime ( void )
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	int time = now.tv_sec - uFixTime;
	m_fTimeCurr = time + (now.tv_nsec * 1e-9) - fFixTime2;
	m_iTimeMilliseconds = time*1000 + (now.tv_nsec / 1000000) - agk::Round(fFixTime2*1000);
}

void agk::PlatformResetTime ( void )
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	uFixTime = now.tv_sec;
	fFixTime2 = now.tv_nsec * 1e-9;
	
	m_fTimeCurr = 0;
	m_iTimeMilliseconds = 0;
}

double agk::PlatformGetRawTime ( void )
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	double dtime = now.tv_sec;
	dtime = dtime + (now.tv_nsec * 1e-9);
	return dtime;
}

void agk::PlatformBindDefaultFramebuffer()
{
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void agk::CompositionChanged()
{

}

void agk::PlatformFlush()
{
	glFlush();
}

void agk::PlatformSync()
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
	eglSwapBuffers(g_pDisplay, g_pSurface);

	//static int first = 0;
	//if ( first == 0 || GetFrameTime() > 0.19f ) PlatformSetViewport( m_iTargetViewportX, m_iTargetViewportY, m_iTargetViewportWidth, m_iTargetViewportHeight );
	//first = 1;
}

void agk::PlatformClearScreen()
{
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
			//PlatformScissor( agk::Round(m_fTargetViewportX), agk::Round(m_fTargetViewportY), agk::Round(m_fTargetViewportWidth), agk::Round(m_fTargetViewportHeight) );

			int ired = ((m_iClearColor >> 16) & 0xff);
			int igreen = ((m_iClearColor >> 8) & 0xff);
			int iblue = (m_iClearColor & 0xff);
			static cSprite* g_pClearSprite = 0;
			if ( !g_pClearSprite ) g_pClearSprite = new cSprite();
			g_pClearSprite->SetSize( (float)agk::GetVirtualWidth(), (float)agk::GetVirtualHeight() );
			g_pClearSprite->SetColor( ired, igreen, iblue, 0 );
			g_pClearSprite->SetTransparency( 0 );
			//glClearColor( red, green, blue, 0 );

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

void agk::PlatformCompleteInputInit()
{
	
}

void agk::KeyboardMode( int mode )
{
	m_iKeyboardMode = mode;
}

void agk::PlatformInputPointerPressed(float x, float y)
{
	virtualkeyboard_change_options( VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT );
	virtualkeyboard_show();

	if ( g_bEditBoxHack )
	{
		// check if pressed on an edit box
		if ( m_cEditBoxList.GetCount() > 0 )
		{
			cEditBox *pFound = 0;
			cEditBox *pEditBox = m_cEditBoxList.GetFirst();
			while ( pEditBox )
			{
				float fWorldX = x;
				float fWorldY = y;

				if ( !pEditBox->GetFixed() )
				{
					fWorldX = agk::ScreenToWorldX( x );
					fWorldY = agk::ScreenToWorldY( y );
				}

				if ( pEditBox->GetHitTest( fWorldX, fWorldY ) && pEditBox->GetVisible() && pEditBox->GetActive() && !pEditBox->HasFocus() )
				{
					pEditBox->SetFocus( true );
					pEditBox->SetCursorLocation( fWorldX, fWorldY );
					pFound = pEditBox;
				}

				pEditBox = m_cEditBoxList.GetNext();
			}

			if ( pFound )
			{
				pFound->GetText( m_sCurrInput );
				g_pTextInputText->SetString( m_sCurrInput );

				pEditBox = m_cEditBoxList.GetFirst();
				while ( pEditBox )
				{
					if ( pEditBox != pFound ) pEditBox->SetFocus( false );
					pEditBox = m_cEditBoxList.GetNext();
				}
			}
			else
			{
				virtualkeyboard_hide();
				m_bInputStarted = false;
				m_bInputCancelled = true;
				int editbox = agk::GetCurrentEditBox();
				if ( editbox > 0 ) agk::SetEditBoxFocus( editbox, 0 );
			}
		}
	}
}

void agk::PlatformStartTextInput( const char *sInitial )
{
	if ( m_bInputStarted ) return;

	virtualkeyboard_change_options( VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT );
	virtualkeyboard_show();

	m_sCurrInput.SetStr( sInitial );
	g_pTextInputText->SetString( sInitial );
	g_iTextCursorPos = g_pTextInputText->GetLength();

	g_bEditBoxHack = false;
	g_bPasswordMode = false;
}

void agk::PlatformStopTextInput() 
{
	// do nothing
}

void agk::PlatformChangeTextInput( const char* str )
{
	m_sCurrInput.SetStr( str );
}

void agk::PlatformUpdateTextInput()
{
	if ( !m_bInputStarted ) return;
	
	if ( m_iLastChar != m_iCurrChar )
	{
		if ( m_iInputMaxChars > 0 && m_sCurrInput.GetLength() >= m_iInputMaxChars )
		{
			g_iShowCursor = 1;
			g_fCursorBlinkTime = m_fCursorBlinkDelay;
		}
		else
		{
			if ( g_iTextCursorPos < (int) m_sCurrInput.GetLength() )
			{
				m_sCurrInput.InsertCharAt( g_iTextCursorPos, (char) m_iCurrChar );
			}
			else
			{
				m_sCurrInput.Append( (char) m_iCurrChar );
			}

			g_iTextCursorPos++;
			if ( g_iTextCursorPos > (int) m_sCurrInput.GetLength() ) g_iTextCursorPos = m_sCurrInput.GetLength();
		}
	}
	
	if ( agk::GetPointerPressed() && m_bInputStarted )
	{
		virtualkeyboard_hide();
		m_bInputStarted = false;
		m_bInputCancelled = true;
		int editbox = agk::GetCurrentEditBox();
		if ( editbox > 0 ) agk::SetEditBoxFocus( editbox, 0 );
	}

	// enter released
	if ( m_iPrevKeyDown[ AGK_KEY_ENTER ] && !m_iKeyDown[ AGK_KEY_ENTER ] )
	{
		m_bInputStarted = false;
		m_bInputCancelled = false;
		virtualkeyboard_hide();
		int editbox = agk::GetCurrentEditBox();
		if ( editbox > 0 ) agk::SetEditBoxFocus( editbox, 0 );
	}
	
	// escape released
	if ( m_iPrevKeyDown[ AGK_KEY_ESCAPE ] && !m_iKeyDown[ AGK_KEY_ESCAPE ] )
	{
		m_bInputCancelled = false;
		m_bInputStarted = false;
		virtualkeyboard_hide();
		int editbox = agk::GetCurrentEditBox();
		if ( editbox > 0 ) agk::SetEditBoxFocus( editbox, 0 );
	}
	
	// backspace held
	static float fBackspaceTimer = 0;
	bool bBackspaceRepeat = false;
	if ( !m_iKeyDown[ AGK_KEY_BACK ] ) fBackspaceTimer = 0;
	
	if ( fBackspaceTimer > 0 )
	{
		fBackspaceTimer -= agk::GetFrameTime();
		if ( fBackspaceTimer <= 0 ) 
		{
			fBackspaceTimer = 0.05f;
			bBackspaceRepeat = true;
		}
	}
	
	if ( (m_iPrevKeyDown[ AGK_KEY_BACK ] && !m_iKeyDown[ AGK_KEY_BACK ]) || bBackspaceRepeat )
	{
		if ( g_iTextCursorPos > 0 )
		{
			m_sCurrInput.DeleteCharAt( g_iTextCursorPos-1 );
			g_iTextCursorPos--;
		}
		g_iShowCursor = 1;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
		
		if ( fBackspaceTimer <= 0 ) fBackspaceTimer = 0.5f;
	}
	
	// delete held
	static float fDeleteTimer = 0;
	bool bDeleteRepeat = false;
	if ( !m_iKeyDown[ AGK_KEY_DELETE ] ) fDeleteTimer = 0;
	
	if ( fDeleteTimer > 0 )
	{
		fDeleteTimer -= agk::GetFrameTime();
		if ( fDeleteTimer <= 0 ) 
		{
			fDeleteTimer = 0.05f;
			bDeleteRepeat = true;
		}
	}
	
	// delete released
	if ( (m_iPrevKeyDown[ AGK_KEY_DELETE ] && !m_iKeyDown[ AGK_KEY_DELETE ]) || bDeleteRepeat )
	{
		if ( g_iTextCursorPos < (int)m_sCurrInput.GetLength() )
		{
			m_sCurrInput.DeleteCharAt( g_iTextCursorPos );
		}
		
		if ( fDeleteTimer <= 0 ) fDeleteTimer = 0.5f;
	}
	
	// left repeat
	static float fLeftTimer = 0;
	bool bLeftRepeat = false;
	if ( !m_iKeyDown[ AGK_KEY_LEFT ] ) fLeftTimer = 0;
	
	if ( fLeftTimer > 0 )
	{
		fLeftTimer -= agk::GetFrameTime();
		if ( fLeftTimer <= 0 ) 
		{
			fLeftTimer = 0.05f;
			bLeftRepeat = true;
		}
	}
	
	// left arrow released
	if ( (m_iPrevKeyDown[ AGK_KEY_LEFT ] && !m_iKeyDown[ AGK_KEY_LEFT ]) || bLeftRepeat )
	{
		g_iTextCursorPos--;
		if ( g_iTextCursorPos < 0 ) g_iTextCursorPos = 0;
		g_iShowCursor = 1;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
		
		if ( fLeftTimer <= 0 ) fLeftTimer = 0.5f;
	}
	
	// right repeat
	static float fRightTimer = 0;
	bool bRightRepeat = false;
	if ( !m_iKeyDown[ AGK_KEY_RIGHT ] ) fRightTimer = 0;
	
	if ( fRightTimer > 0 )
	{
		fRightTimer -= agk::GetFrameTime();
		if ( fRightTimer <= 0 ) 
		{
			fRightTimer = 0.05f;
			bRightRepeat = true;
		}
	}
	
	// right arrow released
	if ( (m_iPrevKeyDown[ AGK_KEY_RIGHT ] && !m_iKeyDown[ AGK_KEY_RIGHT ]) || bRightRepeat )
	{
		g_iTextCursorPos++;
		if ( g_iTextCursorPos > (int) m_sCurrInput.GetLength() ) g_iTextCursorPos = m_sCurrInput.GetLength();
		g_iShowCursor = 1;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
		
		if ( fRightTimer <= 0 ) fRightTimer = 0.5f;
	}
	
	// end released
	if ( m_iPrevKeyDown[ AGK_KEY_END ] && !m_iKeyDown[ AGK_KEY_END ] )
	{
		g_iTextCursorPos = m_sCurrInput.GetLength();
		g_iShowCursor = 1;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
	}
	
	// home released
	if ( m_iPrevKeyDown[ AGK_KEY_HOME ] && !m_iKeyDown[ AGK_KEY_HOME ] )
	{
		g_iTextCursorPos = 0;
		g_iShowCursor = 1;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
	}
}

void agk::PlatformDrawTextInput()
{
	if ( !m_bInputStarted ) return;
	if ( g_bEditBoxHack ) return;
	
	pTextBackground->SetPosition( -m_iDisplayExtraX, -m_iDisplayExtraY );
	pTextBackground->SetSize( m_iDisplayWidth+m_iDisplayExtraX*2, m_iDisplayHeight+m_iDisplayExtraY*2 );
	pTextBackground->Draw();
	
	float virtualWidth = m_iDisplayWidth;
	float virtualHeight = m_iDisplayHeight;
	
	float DevToVirX = virtualWidth / agk::m_fTargetViewportWidth;
	float DevToVirY = virtualHeight / agk::m_fTargetViewportHeight;
	
	/*
	if ( agk::GetOrientation() == 3 || agk::GetOrientation() == 4 )
	{
		DevToVirX = virtualWidth / agk::m_fTargetViewportHeight;
		DevToVirY = virtualHeight / agk::m_fTargetViewportWidth;
	}
	*/

	float width = 250 * DevToVirX;
	float height = 22 * DevToVirY;
	
	if ( width > virtualWidth ) width = virtualWidth;
	
	g_fTextStartX = (virtualWidth - width)/2.0f + 3*DevToVirX;
	g_fTextStartY = virtualHeight/3.0f + 2*DevToVirY;
	
	g_pTextInputCursor->SetSize( 2 * DevToVirX, 18 * DevToVirY );
	g_pTextInputCursor->SetColor( 102, 213, 255, 255 );
	
	g_pTextInputArea->SetSize( width, height );
	g_pTextInputArea->SetColor( 255,255,255,255 );
	g_pTextInputArea->SetPosition( (virtualWidth - width) / 2.0f, virtualHeight/3.0f );
	
	width += 8 * DevToVirX;
	height += 8 * DevToVirY;
	
	g_pTextInputBack->SetSize( width, height );
	g_pTextInputBack->SetColor( 190,190,190,255 );
	g_pTextInputBack->SetPosition( (virtualWidth - width) / 2.0f, virtualHeight/3.0f - 4*DevToVirY );
	
	g_pTextInputText->SetPosition( g_fTextStartX, g_fTextStartY );
	g_pTextInputText->SetColor( 0,0,0 );
	g_pTextInputText->SetSpacing( 0.5f * DevToVirY );
	g_pTextInputText->SetSize( 20 * DevToVirY );
	
	g_pTextInputBack->Draw();
	g_pTextInputArea->Draw();
	
	float fX = g_pTextInputArea->GetX();
	float fY = g_pTextInputArea->GetY();
	float fWidth = g_pTextInputArea->GetWidth();
	float fHeight = g_pTextInputArea->GetHeight();
	float fX2 = g_pTextInputArea->GetX() + g_pTextInputArea->GetWidth();
	float fY2 = g_pTextInputArea->GetY() + g_pTextInputArea->GetHeight();

	// swap Y for viewport
	if ( agk::m_bUsingFBO )
	{
		int iScissorX = agk::ScreenToViewportX( fX );
		int iScissorY = agk::ScreenToViewportY( fY );
		int iScissorWidth = agk::ScreenToViewportX( fX2 ) - iScissorX;
		int iScissorHeight = agk::ScreenToViewportY( fY2 ) - iScissorY;

		agk::PlatformScissor( iScissorX, iScissorY, iScissorWidth, iScissorHeight );
	}
	else
	{
		int iScissorX = agk::ScreenToViewportX( fX );
		int iScissorY = agk::ScreenToViewportY( fY2 );
		int iScissorWidth = agk::ScreenToViewportX( fX2 ) - iScissorX;
		int iScissorHeight = agk::ScreenToViewportY( fY ) - iScissorY;

		agk::PlatformScissor( iScissorX, iScissorY, iScissorWidth, iScissorHeight );
	}

	if ( g_bPasswordMode )
	{
		int length = m_sCurrInput.GetLength();
		uString pass("", length);
		for ( int i = 0; i < length; i++ ) pass.Append( "*" );
		g_pTextInputText->SetString( pass );
	}
	else g_pTextInputText->SetString( m_sCurrInput );
	g_pTextInputText->ChangedAspect();
	
	float posX = g_pTextInputText->GetX();
	float posY = g_pTextInputText->GetY();
	if ( g_iTextCursorPos >= 0 )
	{
		if ( g_iTextCursorPos >= (int)g_pTextInputText->GetLength() )
		{
			posX += g_pTextInputText->GetCharX( g_pTextInputText->GetLength()-1 ) + g_pTextInputText->GetCharWidth( g_pTextInputText->GetLength()-1 );
			posY += g_pTextInputText->GetCharY( g_pTextInputText->GetLength()-1 );
		}
		else
		{
			posX += g_pTextInputText->GetCharX( g_iTextCursorPos );
			posY += g_pTextInputText->GetCharY( g_iTextCursorPos );
		}
	}

	float padX = 2*DevToVirY;
	if ( posX + padX > fX + fWidth )
	{
		float newX = g_pTextInputText->GetX() - ((posX + padX) - (fX + fWidth));
		g_pTextInputText->SetPosition( newX, g_pTextInputText->GetY() );
	}
	else if ( posX < fX + padX )
	{
		float newX = g_pTextInputText->GetX() + ((fX + padX) - posX);
		if ( newX > fX + 1 ) newX = fX + 1;
		g_pTextInputText->SetPosition( newX, g_pTextInputText->GetY() );
	}

	g_pTextInputText->Draw();
	
	g_fCursorBlinkTime -= agk::GetFrameTime();
	if ( g_fCursorBlinkTime <= 0 )
	{
		g_iShowCursor = 1 - g_iShowCursor;
		g_fCursorBlinkTime = m_fCursorBlinkDelay;
	}

	if ( g_iShowCursor )
	{
		float posX = g_pTextInputText->GetX();
		float posY = g_pTextInputText->GetY();
		if ( g_iTextCursorPos >= 0 )
		{
			if ( g_iTextCursorPos >= (int)g_pTextInputText->GetLength() )
			{
				posX += g_pTextInputText->GetCharX( g_pTextInputText->GetLength()-1 ) + g_pTextInputText->GetCharWidth( g_pTextInputText->GetLength()-1 );
				posY += g_pTextInputText->GetCharY( g_pTextInputText->GetLength()-1 );
			}
			else
			{
				posX += g_pTextInputText->GetCharX( g_iTextCursorPos );
				posY += g_pTextInputText->GetCharY( g_iTextCursorPos );
			}
		}

		g_pTextInputCursor->SetPosition( posX, posY );
		g_pTextInputCursor->Draw();
	}
}

void agk::PlatformResumed()
{
	// do nothing
}

void agk::PlatformResumedOpenGL()
{
	// do nothing
}

void agk::PlatformDeviceVolume()
{

}

UINT agk::PlatformLittleEndian( UINT u )
{
	// translate from local endian to little endian
	return u;
}

int agk::PlatformLittleEndian( int i )
{
	return i;
}

UINT agk::PlatformLocalEndian( UINT u )
{
	// translate from little endian to local endian
	return u;
}

int agk::PlatformLocalEndian( int i )
{
	return i;
}

float agk::Sin( float a )
//****
{
	return sin( a * PI / 180 );
}

float agk::Cos( float a )
//****
{
	return cos( a * PI / 180 );
}

float agk::Tan( float a )
//****
{
	return tan( a * PI / 180 );
}

float agk::SinRad( float a )
//****
{
	return sin( a );
}

float agk::CosRad( float a )
//****
{
	return cos( a );
}

float agk::TanRad( float a )
//****
{
	return tan( a );
}

float agk::ASin( float a )
//****
{
	return asin( a ) * 180 / PI;
}

float agk::ACos( float a )
//****
{
	return acos( a ) * 180 / PI;
}

float agk::ATan( float a )
//****
{
	return atan( a ) * 180 / PI;
}

float agk::ATan2( float y, float x )
//****
{
	return atan2( y, x ) * 180 / PI;
}

float agk::ASinRad( float a )
//****
{
	return asin( a );
}

float agk::ACosRad( float a )
//****
{
	return acos( a );
}

float agk::ATanRad( float a )
//****
{
	return atan( a );
}

float agk::ATan2Rad( float y, float x )
//****
{
	return atan2( y, x );
}

int agk::Trunc( float a )
{
	if ( a > 0 ) return Floor( a );
	else return Ceil( a );
}

int agk::Floor( float a )
{
	return (int) ( floor( a ) );
}

int agk::Ceil( float a )
{
	return (int) ( ceil( a ) );
}

int agk::Round( float a )
{
	if ( a < 0 ) return (int) ( ceil( a-0.5f ) );
	else return (int) ( floor( a+0.5f ) );
}

float agk::Sqrt( float a )
{
	if ( a > 0 ) return sqrt( a );
	else return 0;
}

float agk::Abs( float a )
{
	if ( a < 0 ) a = -a;
	return a;
}

int agk::Mod( int a, int b )
//****
{
	return a % b;
}

float agk::FMod( float a, float b )
//****
{
	return fmod( a, b );
}

float agk::Pow( float a, float b )
//****
{
	return pow( a, b );
}

float agk::Log( float a )
//****
{
	return log( a );
}

void RemovePath( uString &sPath )
{
	int index = sPath.RevFind( '/' );
	if ( index >= 0 )
	{
		uString out;
		sPath.SubString( out, index+1 );
		sPath.SetStr( out );
	}
}

// Image

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

void cImage::PlatformSaveDataToFile( const char* szFile, unsigned char *pData, unsigned int width, unsigned int height )
{
	uString sPath( szFile );
	agk::PlatformGetFullPathWrite( sPath );

	uString ext;
	uString temp( szFile );
	int pos = temp.RevFind( '.' );
	if ( pos >= 0 )
	{
		temp.SubString( ext, pos+1 );
	}
	ext.Lower();

	if ( ext.CompareTo( "png" ) == 0 ) write_png( sPath.GetStr(), width, height, (unsigned int*)pData );
	else WriteJPEG( sPath.GetStr(), 95, pData, width, height );
}

void cImage::PlatformSetSubData( int x, int y, int width, int height, unsigned char* pData )
{
	cImage::BindTexture( m_iTextureID );

	glTexSubImage2D(GL_TEXTURE_2D, 0, x,y, width,height, GL_RGBA, GL_UNSIGNED_BYTE, pData );

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
	
	int iTexID = GetTextureID();
	if ( iTexID == 0 ) return 0;

	UINT iTrueWidthSrc = GetTotalWidth();
	UINT iTrueHeightSrc = GetTotalHeight();
	
	*pData = new unsigned char[ iTrueWidthSrc*iTrueHeightSrc*4 ];

	cImage::BindTexture( iTexID );

	GLuint framebuf;
	glGenFramebuffers(1, &framebuf);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iTexID, 0 );
	glReadPixels( 0, 0, iTrueWidthSrc, iTrueHeightSrc, GL_RGBA, GL_UNSIGNED_BYTE, *pData );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glDeleteFramebuffers( 1, &framebuf );

	return iTrueWidthSrc*iTrueHeightSrc*4;
}

bool cImage::ChooseFromSystem()
{
	// do nothing
	return false;
}

void agk::PlatformShowChooseScreen()
{
	// todo
	// show choose image screen on blackberry?
}

bool agk::PlatformShowCaptureScreen()
{
	// todo
	// show capture image screen on blackberry?
	return false;
}

void agk::PlatformHideCaptureScreen()
{
	// do nothing
}

bool cImage::CaptureFromCamera()
{
	// do nothing, use ShowImageCaptureScreen() instead
	return false;
}

int agk::GetNumDeviceCameras()
//****
{
	return 0;
}

int agk::SetDeviceCameraToImage( UINT cameraID, UINT imageID )
{
	return 0;
}

void agk::DeviceCameraUpdate()
{
	
}

void agk::DeviceCameraResumed()
{
	
}

int agk::GetDeviceCameraType( UINT cameraID )
//****
{
	return 0;
}

void cImage::Print( float size )
{
	
}

void cImage::PlatformLoadFromData( int width, int height, UINT *bits )
{
	if ( m_iTextureID == 0 ) glGenTextures( 1, &m_iTextureID );
	cImage::BindTexture( m_iTextureID );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits );
	
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
			glformat = GL_DEPTH_COMPONENT;
			glformat2 = GL_DEPTH_COMPONENT; 
			if ( g_bDepth24Supported ) glbytesize = GL_UNSIGNED_INT;
			else glbytesize = GL_UNSIGNED_SHORT;
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

bool cImage::PlatformGetDataFromFile( const char* szFile, unsigned char **pData, unsigned int *out_width, unsigned int *out_height, int absolute_path )
{
	uString sPath( szFile );

	if ( absolute_path == 0 )
	{
		// try write folder
		if ( cFile::ExistsWrite( szFile ) )
		{
			agk::PlatformGetFullPathWrite( sPath );
		}
		else
		{
			if ( cFile::ExistsRead( szFile ) )
			{
				agk::PlatformGetFullPathRead( sPath );
			}
			else
			{
				if ( strcmp( szFile, "ascii.png" ) == 0 || strcmp( szFile, "/ascii.png" ) == 0 )
				{
					// load a default ascii texture
					GetAsciiFromFile( pData, out_width, out_height );
					return true;
				}

				if ( strcmp( szFile, "asciiExt.png" ) == 0 || strcmp( szFile, "/asciiExt.png" ) == 0 )
				{
					// load a default extended (128-255) ascii texture
					GetAsciiExtFromFile( pData, out_width, out_height );
					return true;
				}

	#ifdef _AGK_ERROR_CHECK
				char str[1024];
				sprintf( str, "Could not find image: %s, %s", szFile, sPath.GetStr() );
				agk::Error( uString( str ) );
	#endif
				return false;
			}
		}
	}
	
	int width;
	int height;
	bool hasAlpha;
	
	bool result = false;
	const char *szExt = strrchr( szFile, '.' );
	char *szExtL = agk::Lower( szExt );

	bool bIsPNG = false;
	bool bIsJPG = false;

	if ( strcmp( szExtL, ".png" ) == 0 ) bIsPNG = true;
	else bIsJPG = true;

	delete [] szExtL;

	if ( bIsPNG ) result = loadPngImage( sPath.GetStr(), width, height, hasAlpha, pData );
	else if ( bIsJPG ) result = loadJpegImage( sPath.GetStr(), width, height, hasAlpha, pData );

	if ( !result )
	{	
		uString str( "Failed to load raw image ", 100 ); str.Append( szFile );
		str.Append( ", " ); str.Append( sPath.GetStr() );
		agk::Error( str );
		return false;
	}
	
	if ( out_width ) *out_width = width;
	if ( out_height ) *out_height = height;
	
	if ( !hasAlpha )
	{
		// add alpha channel
		unsigned char *tempData = (unsigned char*) new unsigned int[ width * height * 4 ];
		for ( int y = 0; y < height; y++ )
		{
			unsigned int index = y*width;

			for ( int x = 0; x < width; x++ )
			{
				tempData[ index*4 ] = (*pData)[ index*3 ];
				tempData[ index*4 + 1 ] = (*pData)[ index*3 + 1 ];
				tempData[ index*4 + 2 ] = (*pData)[ index*3 + 2 ];
				tempData[ index*4 + 3 ] = 255;
				index++;
			}
		}

		delete [] (*pData);
		*pData = tempData;
	}

	return true;
}

void cImage::PlatformReloadFromData()
{
	if ( m_pParentImage ) return;

	UINT glformat = GL_RGBA;
	UINT glformat2 = GL_RGBA;
	UINT glbytesize = GL_UNSIGNED_BYTE;

	switch( m_iImageMode )
	{
		case 1: 
		{
			glformat = GL_DEPTH_COMPONENT;
			glformat2 = GL_DEPTH_COMPONENT; 
			glbytesize = GL_UNSIGNED_INT;
			break;
		}
	}

	uLong size = m_iWidth*m_iHeight*4;
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
	    
		if ( size != m_iWidth*m_iHeight*4 )
		{
			delete [] tempbuf;
			agk::Warning( "Uncompressed image data is the wrong size" );
			return;
		}
	}
	else
	{
		// if no data exists for the image create it as a blank image
		for( unsigned int i = 0; i < size; i++ )
		{
			tempbuf[ i ] = 0;
		}
	}

	// generate OepnGL texture
	if ( m_iTextureID == 0 ) glGenTextures( 1, &m_iTextureID );
	cImage::BindTexture( m_iTextureID );

	glTexImage2D(GL_TEXTURE_2D, 0, glformat, m_iWidth, m_iHeight, 0, glformat2, glbytesize, tempbuf );

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

void cSprite::PlatformDraw( float *vertices, float *uv, unsigned char *color )
{
	AGKShader *pShader = m_pShader;

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( !pShader ) return;
	pShader->MakeActive();
	
	int locPos = pShader->GetAttribPosition();
	int locColor = pShader->GetAttribColor();
	int locTex = pShader->GetAttribTexCoord();

	if ( locPos >= 0 ) pShader->SetAttribFloat( locPos, 3, 0, vertices );
	if ( locColor >= 0 ) pShader->SetAttribUByte( locColor, 4, 0, true, color );
	if ( locTex >= 0 ) pShader->SetAttribFloat( locTex, 2, 0, uv );

	agk::PlatformSetCullMode( 0 );

	agk::PlatformSetBlendMode( m_iTransparencyMode );
	agk::PlatformSetDepthRange( 0, 1 );

	// Draw
	pShader->DrawPrimitives( AGK_TRIANGLE_STRIP, 0, 4 );
}

void AGK::cText::PlatformDraw()
{
	if ( !m_bVisible ) return; 

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	AGKShader *pShader = AGKShader::g_pShaderTexColor;
	
	if ( !pShader ) return;
	pShader->MakeActive();
	
	int locPos = pShader->GetAttribPosition();
	int locColor = pShader->GetAttribColor();
	int locTex = pShader->GetAttribTexCoord();

	if ( locPos >= 0 ) pShader->SetAttribFloat( locPos, 3, 0, pVertices );
	if ( locColor >= 0 ) pShader->SetAttribUByte( locColor, 4, 0, true, pColor );
	if ( locTex >= 0 ) pShader->SetAttribFloat( locTex, 2, 0, pUV );

	agk::PlatformSetCullMode( 1 );

	agk::PlatformSetBlendMode( m_iTransparency );
	agk::PlatformSetDepthRange( 0, 1 );

	// extended characters
	UINT iCurrImage = 0;
	if ( m_pDefaultFontExt ) iCurrImage = m_pDefaultFontExt->GetTextureID();
	if ( m_pFontImageExt ) iCurrImage = m_pFontImageExt->GetTextureID();

	int iLength = m_sText.GetLength();
	int count = 0;
	for ( int i = 0; i < iLength; i++ )
	{
		if ( m_pSprites[ i ]->GetImagePtr() && m_pSprites[ i ]->GetImagePtr()->GetTextureID() == iCurrImage )
		{
			if ( m_pSprites[ i ]->GetInScreen() )
			{
				m_pSprites[ i ]->BatchDrawQuad( pVertices + count*12, pUV + count*8, pColor + count*16 );
				count++;
			}
		}
	}
	
	if ( count > 0 )
	{
		cImage::BindTexture( iCurrImage );

		//glDrawElements( GL_TRIANGLES, count*6, GL_UNSIGNED_SHORT, pIndices );
		pShader->DrawIndices( count*6, pIndices );
	}

	int oldImage = iCurrImage;

	// normal characters
	iCurrImage = 0;
	if ( m_pDefaultFont ) iCurrImage = m_pDefaultFont->GetTextureID();
	if ( m_pFontImage ) iCurrImage = m_pFontImage->GetTextureID();

	if ( oldImage == iCurrImage ) return;

	count = 0;
	for ( int i = 0; i < iLength; i++ )
	{
		if ( m_pSprites[ i ]->GetImagePtr() && m_pSprites[ i ]->GetImagePtr()->GetTextureID() == iCurrImage )
		{
			if ( m_pSprites[ i ]->GetInScreen() )
			{
				m_pSprites[ i ]->BatchDrawQuad( pVertices + count*12, pUV + count*8, pColor + count*16 );
				count++;
			}
		}
	}
	
	if ( count > 0 )
	{
		cImage::BindTexture( iCurrImage );

		//glDrawElements( GL_TRIANGLES, count*6, GL_UNSIGNED_SHORT, pIndices );
		pShader->DrawIndices( count*6, pIndices );
	}
}

float cParticleEmitter::GetMaxPointSize()
{
	float maxSize = 0.0f;
    //glGetFloatv( GL_POINT_SIZE_MAX_ARB, &maxSize );
	return maxSize;
}

void cParticleEmitter::PlatformDrawQuadParticles( UINT count, unsigned short *pIndices, float *pVertices, float *pUV, unsigned char *pColors )
{
	// set drawing parameters for quad rendering
	agk::PlatformSetBlendMode( m_iTransparencyMode );
	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );
	agk::PlatformSetCullMode( 0 );
	agk::PlatformSetDepthRange( 0, 1 );

	AGKShader *pShader = AGKShader::g_pShaderTexColor;
	if ( m_pImage ) cImage::BindTexture( m_pImage->GetTextureID() );
	else
	{
		cImage::BindTexture( 0 );
		pShader = AGKShader::g_pShaderColor;
	}

	if ( !pShader ) return;
	pShader->MakeActive();

	int locPos = pShader->GetAttribPosition();
	int locColor = pShader->GetAttribColor();
	int locTex = pShader->GetAttribTexCoord();

	if ( locPos >= 0 ) pShader->SetAttribFloat( locPos, 3, 0, pVertices );
	if ( locColor >= 0 ) pShader->SetAttribUByte( locColor, 4, 0, true, pColors );
	if ( locTex >= 0 ) pShader->SetAttribFloat( locTex, 2, 0, pUV );

	pShader->DrawIndices( count*6, pIndices );
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
				agk::PlatformBindBuffer( m_iVBOIndices[ i ] );
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

void cMesh::PlatformDraw()
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

	// set shader parameters
	//int locPos = m_pShader->GetAttribPosition();
	//int locNorm = m_pShader->GetAttribNormal();
	//int locTex = m_pShader->GetAttribTexCoord();

	for ( UINT i = 0; i < pVertexOwner->m_iNumArrays; i++ )
	{
		agk::m_iVerticesProcessed += pVertexOwner->m_iNumVertices[ i ];

		agk::PlatformBindBuffer( pVertexOwner->m_iVBOVertices[ i ] );

		for ( unsigned char a = 0; a < pVertexOwner->m_iNumAttribs; a++ )
		{
			cVertexAttrib *pAttrib = pVertexOwner->m_pVertexAttribs[ a ];
			int shaderLoc = pAttrib->m_iShaderLoc;
			if ( m_pSharedVertices ) shaderLoc = m_pShader->GetAttribByName( pAttrib->m_sName );
			if ( shaderLoc >= 0 )
			{
				UINT stride = 0;
				void* ptr = pAttrib->m_pData;
				if ( pAttrib->m_iOffset >= 0 )
				{
					stride = pVertexOwner->m_iVertexStride;
					ptr = (void*)(pAttrib->m_iOffset);
				}

				if ( pAttrib->m_iType == 0 ) m_pShader->SetAttribFloat( shaderLoc, pAttrib->m_iComponents, stride, (float*)ptr );
				else m_pShader->SetAttribUByte( shaderLoc, pAttrib->m_iComponents, stride, pAttrib->m_bNormalize, (unsigned char*)ptr );
			}
		}

		if ( pVertexOwner->m_pDummyAttribs && i == 0 )
		{
			agk::PlatformBindBuffer( 0 );
			cDummyAttrib *pAttrib = pVertexOwner->m_pDummyAttribs;
			while( pAttrib )
			{
				int shaderLoc = pAttrib->m_iShaderLoc;
				if ( m_pSharedVertices ) shaderLoc = m_pShader->GetAttribByName( pAttrib->m_sName );
				if ( shaderLoc >= 0 )
				{
					// ATI doesn't seem to like glVertexAttrib4fv as a substitute for vertex arrays
					//glVertexAttrib4fv( pAttrib->m_iShaderLoc, &(pAttrib->m_pData[0]) );
					m_pShader->SetAttribUByte( shaderLoc, 4, 0, false, pAttrib->m_pData );
				}
				pAttrib = pAttrib->m_pNextAttrib;
			}
		}

		// Draw
		if ( pVertexOwner->m_iVBOIndices && pVertexOwner->m_iVBOIndices[ i ] ) 
		{
			agk::PlatformBindIndexBuffer( pVertexOwner->m_iVBOIndices[ i ] );

			if ( m_iPrimitiveType == AGK_TRIANGLES ) agk::m_iPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] / 3;
			else if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP ) agk::m_iPolygonsDrawn += pVertexOwner->m_iNumIndices[ i ] - 2;

			if ( m_iFlags & AGK_MESH_UINT_INDICES )
			{
				m_pShader->DrawIndicesInt( pVertexOwner->m_iNumIndices[ i ], 0, m_iPrimitiveType );
			}
			else
			{
				m_pShader->DrawIndices( pVertexOwner->m_iNumIndices[ i ], 0, m_iPrimitiveType );
			}
		}
		else
		{
			agk::PlatformBindIndexBuffer( 0 );

			agk::m_iPolygonsDrawn += pVertexOwner->m_iNumVertices[ i ] / 3;
			m_pShader->DrawTriangles( 0, pVertexOwner->m_iNumVertices[ i ] );
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

	// check if we should update everything anyway
	if ( (m_bFlags & AGK_SHADER_RELOAD_UNIFORMS) != 0 )
	{
		bWorldChanged = true;
		bViewChanged = true;
		bProjChanged = true;
		bOrthoChanged = true;
		bNormChanged = true;

		// texture samplers only need to be updated once, do it here for simplicity
		for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
		{
			glUniform1i( m_iTexture2D[ i ], i );
			glUniform1i( m_iTextureCube[ i ], i );
		}

		m_bTextureChanged = 0xffffffff;
		m_bFlags &= ~AGK_SHADER_RELOAD_UNIFORMS;
	}

	// check if texture UV offsets changed
	for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
	{
		UINT index = 1 << i;
		if ( m_bTextureChanged & index )
		{
			glUniform4f( m_iTextureBounds[ i ], m_fTextureU2[ i ]-m_fTextureU1[ i ], m_fTextureV2[ i ]-m_fTextureV1[ i ], m_fTextureU1[ i ], m_fTextureV1[ i ] );
		}

		m_bTextureChanged &= ~index;
	}

	if ( m_iUniformWorldMat >= 0 && bWorldChanged ) glUniformMatrix4fv( m_iUniformWorldMat, 1, GL_FALSE, &m_matWorld.mat[0][0] );
	if ( m_iUniformViewMat >= 0 && bViewChanged ) glUniformMatrix4fv( m_iUniformViewMat, 1, GL_FALSE, &m_matView.mat[0][0] );
	if ( bProjChanged && m_iUniformProjMat >= 0 ) glUniformMatrix4fv( m_iUniformProjMat, 1, GL_FALSE, &m_matProj.mat[0][0] );
	if ( bOrthoChanged && m_iUniformOrthoMat >= 0 ) glUniformMatrix4fv( m_iUniformOrthoMat, 1, GL_FALSE, &g_matOrtho.mat[0][0] );
	if ( m_iUniformNormalMat >= 0 && bNormChanged ) glUniformMatrix3fv( m_iUniformNormalMat, 1, GL_FALSE, &m_matWorldNormal.mat[0][0] );

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
		      | AGK_SHADER_WORLDNORMAL_CHANGED | AGK_SHADER_ORTHO_CHANGED;
	m_bFlags &= ~mask;
}

void AGKShader::SetShaderSource( const char* vertex, const char* fragment )
{
	GLint logLength=0, logLength2=0, status=0;
	GLuint vertShader=0, fragShader=0;

	m_sVSSource.SetStr( vertex );
	m_sPSSource.SetStr( fragment );

	m_sVSLog.SetStr( "" );
	m_sPSLog.SetStr( "" );

	// add precision if not alread present
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
		strcpy( fragment2, "precision mediump float;\n" );
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

	// get uniform locations
	m_iUniformWorldMat = glGetUniformLocation ( m_iShaderID, "agk_World" );
	m_iUniformNormalMat = glGetUniformLocation ( m_iShaderID, "agk_WorldNormal" );
	m_iUniformViewMat = glGetUniformLocation ( m_iShaderID, "agk_View" );
	m_iUniformProjMat = glGetUniformLocation ( m_iShaderID, "agk_Proj" );
	m_iUniformOrthoMat = glGetUniformLocation ( m_iShaderID, "agk_Ortho" );
	m_iUniformVPMat = glGetUniformLocation ( m_iShaderID, "agk_ViewProj" );
	m_iUniformWVPMat = glGetUniformLocation ( m_iShaderID, "agk_WorldViewProj" );
	m_iUniformWVOMat = glGetUniformLocation ( m_iShaderID, "agk_WorldOrtho" );

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
		m_iTextureBounds[ i ] = glGetUniformLocation( m_iShaderID, name.GetStr() );
	}

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
	//glBindFramebuffer( GL_FRAMEBUFFER, m_iFBO );
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
			glRenderbufferStorage( GL_RENDERBUFFER, 0x81A6, pColor ? pColor->GetTotalWidth() : 32, pColor ? pColor->GetTotalHeight() : 32 );
			glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_iRBODepth );
		}
	}
	else
	{
		if ( forceDepth )
		{
			glGenRenderbuffers( 1, &m_iRBODepth );
			glBindRenderbuffer( GL_RENDERBUFFER, m_iRBODepth );
			glRenderbufferStorage( GL_RENDERBUFFER, 0x81A6, pColor ? pColor->GetTotalWidth() : 32, pColor ? pColor->GetTotalHeight() : 32 );
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

// Music

void cMusicMgr::PlatformAddFile( cMusic *pMusic )
{
	uString sFilename = pMusic->m_sFile;
	sFilename.Lower();
	if ( sFilename.FindStr(".mp3") && !cFile::Exists( pMusic->m_sFile ) )
	{
		//pMusic->m_sFile.Trunc( '.' );
		//pMusic->m_sFile.Append( ".m4a" );
	}
}

bool cMusicMgr::PlatformPrepare( UINT iID )
{
	cMusic *pMusic = m_pMusicFiles[ iID ];

	m_pCurrentlyPlaying = UNDEF;
	
	/*
	if ( pMusicPlayer )
	{
		if ( pMusicPlayer )
		{
			(*pMusicPlayer)->Destroy(pMusicPlayer);
			pMusicPlayer = 0;
		}
	}
	*/
	
	// try write folder
	uString sPath( pMusic->m_sFile );
	agk::PlatformGetFullPathWrite( sPath );
	
	off_t start = 0;
	off_t length;
	int fd = open( sPath.GetStr(), O_RDONLY, 0777 );
	if ( fd < 0 )
	{
		// try asset folder
		sPath.SetStr( pMusic->m_sFile );
		agk::PlatformGetFullPathRead( sPath );
		
		fd = open( sPath.GetStr(), O_RDONLY, 0777 );
		if ( fd < 0 )
		{
			uString err( "Failed to load music file ", 50 );
			err.Append( pMusic->m_sFile );
			agk::Error( err );
			return false;
		}
	}
	else
	{
		struct stat buf;
		fstat(fd, &buf);
		length = buf.st_size;
	}

	sPath.Prepend( "file://" );

	// volume doesn't work
	/*
	strm_dict_t* dict = strm_dict_new();
	strm_dict_set( dict, "volume", "10" );
	mmr_output_parameters( g_mediaContext, g_audioID, dict );
	*/

	mmr_input_attach( g_mediaContext, sPath.GetStr(), "track" );

	return true;
}

void cMusicMgr::Play( UINT iID, bool bLoop, UINT iStartID, UINT iEndID )
{
	if ( iStartID < 1 ) iStartID = 1;
	if ( iStartID >= MAX_MUSIC_FILES ) iStartID = MAX_MUSIC_FILES - 1;
	if ( iEndID < iStartID ) iEndID = iStartID;
	if ( iEndID >= MAX_MUSIC_FILES ) iEndID = MAX_MUSIC_FILES - 1;

	m_iStartID = iStartID;
	m_iEndID = iEndID;
	m_bLoop = bLoop;

	if ( iID < 1 || iID >= MAX_MUSIC_FILES )
	{
		uString str( "Could not play music file ", 100 );
		str.Append( (int)iID ).Append( " - ID must be between 1 and " ).Append( MAX_MUSIC_FILES-1 );
		agk::Error( str );
		return;
	}
	
	if ( !m_pMusicFiles[ iID ] )
	{
		uString str( "Could not play music file ", 100 );
		str.Append( (int)iID ).Append(" - ID does not exist" );
		agk::Error( str );
		return;
	}

	if ( !PlatformPrepare( iID ) ) return;

	cMusic *pMusic = m_pMusicFiles[ iID ];

	m_pCurrentlyPlaying = pMusic;
	
	mmr_play( g_mediaContext );
	mmr_speed_set( g_mediaContext, 1000 );

	float volume = m_iMasterVolume / 100.0f;
	volume *= pMusic->m_iVolume / 100.0f;

	// convert to centibels
	if ( volume > 0.00001f ) volume = 100.0f * 20.0f * log10(volume);
	else volume = -10000;

	// volume doesn't work
}

void cMusicMgr::Pause()
{
	//mmr_stop( g_mediaContext );
	mmr_speed_set( g_mediaContext, 0 );
}

void cMusicMgr::Resume()
{
	//mmr_play( g_mediaContext );
	mmr_speed_set( g_mediaContext, 1000 );
}

void cMusicMgr::Stop()
{
	mmr_stop( g_mediaContext );
	mmr_seek( g_mediaContext, "0" );
	m_pCurrentlyPlaying = UNDEF;
}

void cMusicMgr::SetMasterVolume( int vol )
{
	// todo
	/*
	if ( vol > 100 ) vol = 100;
	if ( vol < 0 ) vol = 0;
	m_iMasterVolume = vol;
	
	float volume = 1.0f;
	if ( m_pCurrentlyPlaying ) volume = m_pCurrentlyPlaying->m_iVolume / 100.0f;
	volume *= m_iMasterVolume / 100.0f;
	
	// convert to centibels
	if ( volume > 0.00001f ) volume = 100.0f * 20.0f * log10(volume);
	else volume = -10000;
*/

}

float cMusicMgr::GetDuration( UINT ID )
{
	// todo
	return 0;

	/*
	if ( m_pCurrentlyPlaying )
	{
		if ( m_pCurrentlyPlaying->m_iID != ID )
		{
#ifdef _AGK_ERROR_CHECK
			agk::Error( "Cannot get music file duration whilst another file is playing" );
#endif
			return 0;
		}
	}
	else
	{
		if ( !PlatformPrepare( ID ) ) return 0;
	}

	SLmillisecond dur = 0;
	(*pMusicPlayerPlay)->GetDuration( pMusicPlayerPlay, &dur );
	float seconds = dur / 1000.0f;
	return seconds;
	*/
}

void cMusicMgr::Seek( float seconds, int mode )
{
	if ( !m_pCurrentlyPlaying )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot seek as no music is playing" );
#endif
		return;
	}

	// todo
}

float cMusicMgr::GetPosition()
{
	if ( !m_pCurrentlyPlaying ) return 0;

	// todo
	return 0;
}

void cMusicMgr::HandleEvent()
{
	if ( m_pCurrentlyPlaying == UNDEF ) return;
	if ( !g_mediaConnection ) return;
		
	//if ( [musicPlayer rate] == 0 )
	{
		UINT iCurrID = m_pCurrentlyPlaying->m_iID;
		UINT iNextID = 0;
		
		if ( iCurrID >= m_iEndID )
		{
			if ( m_bLoop ) 
			{
				for ( UINT i = m_iStartID; i <= m_iEndID; i++ )
				{
					if ( m_pMusicFiles[ i ] ) 
					{
						iNextID = i;
						break;
					}
				}
			}
		}
		else
		{
			for ( UINT i = iCurrID+1; i <= m_iEndID; i++ )
			{
				if ( m_pMusicFiles[ i ] ) 
				{
					iNextID = i;
					break;
				}
			}
			
			if ( iNextID == 0 && m_bLoop ) 
			{
				for ( UINT i = m_iStartID; i <= iCurrID; i++ )
				{
					if ( m_pMusicFiles[ i ] ) 
					{
						iNextID = i;
						break;
					}
				}
			}
		}
		
		if ( iNextID > 0 ) Play( iNextID, m_bLoop, m_iStartID, m_iEndID );
		else m_pCurrentlyPlaying = UNDEF;
	}
}

// OGG Music

struct AGKOGGSoundData
{
	ALuint bufferID[ 3 ];
	ALuint sourceID;
	ALenum format;
	
	unsigned int m_iCurrBuffer;
    int m_iResetFlag[ 3 ];
};

void AGKMusicOGG::PlatformInit()
{
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	if ( !m_pSoundData ) 
	{
		m_pSoundData = (void*) new AGKOGGSoundData();
		pTemp = (AGKOGGSoundData*)m_pSoundData;
		
		pTemp->m_iCurrBuffer = 0;
        pTemp->m_iResetFlag[ 0 ] = 0;
        pTemp->m_iResetFlag[ 1 ] = 0;
        pTemp->m_iResetFlag[ 2 ] = 0;
    }

	if ( m_fmt.nChannels == 2 ) pTemp->format = AL_FORMAT_STEREO16;
	else if ( m_fmt.nChannels == 1 ) pTemp->format = AL_FORMAT_MONO16;
	else
	{
		agk::Error( "Unsupported ogg file number of channels, only 1 (mono) and 2 (stereo) channels supported." );
		return;
	}
	
	alGenBuffers(3, pTemp->bufferID);
	
	alGenSources(1, &(pTemp->sourceID)); 
	alSourcei(pTemp->sourceID, AL_LOOPING, AL_FALSE);
    alSourcei(pTemp->sourceID, AL_SOURCE_RELATIVE, AL_TRUE);

	float fVol = m_iVolume / 100.0f;
	alSourcef(pTemp->sourceID, AL_GAIN, fVol);
}

void AGKMusicOGG::PlatformCleanUp()
{
	if ( m_pSoundData ) 
	{
		AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

		if( pTemp->sourceID ) 
		{
			alSourceStop(pTemp->sourceID);
			alDeleteSources(1, &pTemp->sourceID);
		}
		if ( pTemp->bufferID[0] ) alDeleteBuffers( 1, &pTemp->bufferID[0] );
		if ( pTemp->bufferID[1] ) alDeleteBuffers( 1, &pTemp->bufferID[1] );
        if ( pTemp->bufferID[2] ) alDeleteBuffers( 1, &pTemp->bufferID[2] );

		delete pTemp;
		m_pSoundData = 0;
	}
}

int AGKMusicOGG::PlatformPlay()
{
	if ( !m_pSoundData ) return 0;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	alSourcePlay(pTemp->sourceID);

	return 1;
}

void AGKMusicOGG::PlatformSetVolume()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	float fVol = (m_iVolume * g_iMasterVolume) / 10000.0f;
	alSourcef(pTemp->sourceID, AL_GAIN, fVol);
}

void AGKMusicOGG::PlatformPause()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	alSourcePause(pTemp->sourceID);
}

void AGKMusicOGG::PlatformResume()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	alSourcePlay(pTemp->sourceID);
}

void AGKMusicOGG::PlatformStop()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	alSourceStop( pTemp->sourceID );
    alSourcei( pTemp->sourceID, AL_BUFFER, 0 );
    
    pTemp->m_iResetFlag[ 0 ] = 0;
    pTemp->m_iResetFlag[ 1 ] = 0;
    pTemp->m_iResetFlag[ 2 ] = 0;
}

void AGKMusicOGG::PlatformClearBuffers()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;
		
	alSourceStop( pTemp->sourceID );
    alSourcei( pTemp->sourceID, AL_BUFFER, 0 );
    
    pTemp->m_iResetFlag[ 0 ] = 0;
    pTemp->m_iResetFlag[ 1 ] = 0;
    pTemp->m_iResetFlag[ 2 ] = 0;
}

void AGKMusicOGG::PlatformReset()
{
	if ( !m_pSoundData ) return;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;

	if( pTemp->sourceID ) 
	{
		alSourceStop(pTemp->sourceID);
		alDeleteSources(1, &pTemp->sourceID);
	}
	if ( pTemp->bufferID[0] ) alDeleteBuffers( 1, &pTemp->bufferID[0] );
	if ( pTemp->bufferID[1] ) alDeleteBuffers( 1, &pTemp->bufferID[1] );
    if ( pTemp->bufferID[2] ) alDeleteBuffers( 1, &pTemp->bufferID[2] );
	
	pTemp->sourceID = 0;
	pTemp->bufferID[0] = 0;
	pTemp->bufferID[1] = 0;
    pTemp->bufferID[2] = 0;
    
    pTemp->m_iResetFlag[ 0 ] = 0;
    pTemp->m_iResetFlag[ 1 ] = 0;
    pTemp->m_iResetFlag[ 2 ] = 0;
}

int AGKMusicOGG::PlatformGetNumBuffers()
{
	if ( !m_pSoundData ) return 0;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;
	
	int processed = 0;
	alGetSourcei( pTemp->sourceID, AL_BUFFERS_PROCESSED, &processed);
	
	int queued = 0;
	alGetSourcei( pTemp->sourceID, AL_BUFFERS_QUEUED, &queued);
    
    //NSLog( @"Queued: %d, Processed: %d, Curr: %d", queued, processed, pTemp->m_iCurrBuffer );
	
	return queued - processed;
}

int AGKMusicOGG::PlatformGetMaxBuffers() { return 3; }

int AGKMusicOGG::PlatformAddBuffer( int *reset )
{
	if ( !m_pSoundData ) return 0;
	AGKOGGSoundData *pTemp = (AGKOGGSoundData*)m_pSoundData;
	
    // get rid of used buffers
    int processed = 0;
    alGetSourcei( pTemp->sourceID, AL_BUFFERS_PROCESSED, &processed);
    
    while( processed > 0 )
    {
        processed--;
        ALuint buf;
        alSourceUnqueueBuffers( pTemp->sourceID, 1, &buf );
    }
    
    // add new buffer
	int currBuf = pTemp->m_iCurrBuffer;
    if ( reset ) *reset = pTemp->m_iResetFlag[ currBuf ];
    pTemp->m_iResetFlag[ currBuf ] = (m_iFlags & AGK_MUSIC_OGG_BUFFER_END) ? 1 : 0;
    
   	pTemp->m_iCurrBuffer++;
    if ( pTemp->m_iCurrBuffer >= PlatformGetMaxBuffers() ) pTemp->m_iCurrBuffer = 0;
    
	alBufferData( pTemp->bufferID[currBuf], pTemp->format, m_pDecodeBuffer, m_iBufferSize, m_fmt.nSamplesPerSec);
	alSourceQueueBuffers( pTemp->sourceID, 1, &(pTemp->bufferID[currBuf]) );
    
    return 1;
}

// Sound

void cSoundMgr::PlatformInit()
{	
	if ( !audioDevice )
	{
		audioDevice = alcOpenDevice(NULL); // select the "preferred device"
		if(!audioDevice) agk::Error( "Failed to setup audio device" );
		else
		{
			audioContext = alcCreateContext(audioDevice, NULL);
			if(!audioContext) agk::Error( "Failed to create audio context" );
			alcMakeContextCurrent(audioContext);
		}
	}

	if ( !g_mediaConnection )
	{
		g_mediaConnection = mmr_connect(NULL);
		g_mediaContext = mmr_context_create( g_mediaConnection, "testplayer", 0, S_IRUSR | S_IXUSR );

		g_audioID = mmr_output_attach( g_mediaContext, "audio:default", "audio" );
		//mmr_output_parameters( g_mediaContext, audio_oid, NULL );

		mmrenderer_request_events( "testplayer", 0, 0 );
	}

	m_fMinPlaybackRate = 0.5f;
	m_fMaxPlaybackRate = 2.0f;
	m_fStepPlaybackRate = 0.01f; // unknown
}

void cSoundMgr::AppPaused()
{
    
}

void cSoundMgr::AppResumed()
{
    
}

void cSoundMgr::PlatformAddFile( cSoundFile *pSound )
{
	// no further processing needed on Blackberry
	return;
}

void cSoundMgr::PlatformUpdate()
{
	int buffers = 0;
	cSoundInst *pSound = m_pSounds;
	cSoundInst *pNext = UNDEF;
	while ( pSound )
	{
		pNext = pSound->m_pNextInst;
		
		if ( pSound->sourceID )
		{
			//alGetSourcei(pSound->sourceID, AL_SOURCE_STATE, &state);
			alGetSourcei(pSound->sourceID, AL_BUFFERS_PROCESSED, &buffers);
			if ( buffers > 0 )
			{
				while( buffers > 0 )
				{
					buffers--;
					ALuint buf;
					alSourceUnqueueBuffers( pSound->sourceID, 1, &buf );
					pSound->m_iLoopCount++;
				}
				
				if ( pSound->m_iLoop == 1 || pSound->m_iLoopCount+1 < pSound->m_iLoop )
				{
					alSourceQueueBuffers(pSound->sourceID, 1, &(pSound->bufferID));
				}
			}

			int state = 0;
			alGetSourcei( pSound->sourceID, AL_SOURCE_STATE, &state );
			if ( state != AL_PLAYING )
			{
				pSound->m_iLoopCount++;

				if ( m_pSoundFiles[ pSound->m_iParent ] ) m_pSoundFiles[ pSound->m_iParent ]->m_iInstances--;
				// sound is finished, cleanup and move to used list
				pSound->Reset();
				pSound->m_uLastUsed = agk::GetSeconds();
				if ( pSound->m_pPrevInst ) pSound->m_pPrevInst->m_pNextInst = pSound->m_pNextInst;
				else m_pSounds = pSound->m_pNextInst;

				if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound->m_pPrevInst;

				m_cSoundInstances.RemoveItem( pSound->m_iID );

				// add to head of use list
				pSound->m_pPrevInst = UNDEF;
				pSound->m_pNextInst = m_pUsedSounds;
				m_pUsedSounds = pSound;

				if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound;
			}
		}
		
		pSound = pNext;
	}
	
	int iSeconds = agk::GetSeconds();
	
	// check for used sounds that haven't been used in a while and delete them
	pSound = m_pUsedSounds;
	while ( pSound )
	{
		pNext = pSound->m_pNextInst;
		
		// delete after 10 seconds on the used list
		if ( iSeconds - pSound->m_uLastUsed > 10 )
		{
			if ( pSound->m_pPrevInst ) pSound->m_pPrevInst->m_pNextInst = pSound->m_pNextInst;
			else m_pUsedSounds = pSound->m_pNextInst;
			
			if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound->m_pPrevInst;
			delete pSound;
		}
		
		pSound = pNext;
	}
}

void cSoundMgr::PlatformUpdateVolume()
{
	cSoundInst *pSound = m_pSounds;
	while ( pSound )
	{
		float fVol = m_iGlobalVolume / 100.0f;
		fVol *= ( pSound->m_iVolume / 100.0f );
		if ( pSound->sourceID ) alSourcef(pSound->sourceID, AL_GAIN, fVol);
		
		pSound = pSound->m_pNextInst;
	}
}

void cSoundMgr::PlatformDelete()
{
	m_cSoundInstances.ClearAll();

	cSoundInst *pSound;
	while ( m_pSounds )
	{
		pSound = m_pSounds;
		m_pSounds = m_pSounds->m_pNextInst;
		delete pSound;
	}
	
	while ( m_pUsedSounds )
	{
		pSound = m_pUsedSounds;
		m_pUsedSounds = m_pUsedSounds->m_pNextInst;
		delete pSound;
	}
}

UINT cSoundMgr::PlatformCreateInstance( cSoundMgr::cSoundFile *pSound, int iVol, int iLoop, int iPriority )
{
	if ( !pSound ) return 0;
	if ( iVol < 0 ) iVol = 0;
	if ( iVol > 100 ) iVol = 100;
	
	cSoundInst *pSoundInst = UNDEF;
	// find any used instances
	if ( m_pUsedSounds )
	{
		pSoundInst = m_pUsedSounds;
		m_pUsedSounds = m_pUsedSounds->m_pNextInst;
		if ( m_pUsedSounds ) m_pUsedSounds->m_pPrevInst = UNDEF;
	}
	
	if ( iLoop < 0 ) iLoop = 0;

	// if no used sounds awaiting re-use, create a new one
	if ( !pSoundInst ) pSoundInst = new cSoundInst();
	pSoundInst->m_iID = m_cSoundInstances.GetFreeID();
	pSoundInst->m_iParent = pSound->m_iID;
	pSoundInst->m_iVolume = iVol;
	pSoundInst->m_iLoop = iLoop;
	pSoundInst->m_iLoopCount = 0;
	
	// add it to the running list
	pSoundInst->m_pPrevInst = UNDEF;
	pSoundInst->m_pNextInst = m_pSounds;
	m_pSounds = pSoundInst;
	if ( pSoundInst->m_pNextInst ) pSoundInst->m_pNextInst->m_pPrevInst = pSoundInst;
	
	ALenum format;
	if ( pSound->m_fmt.nChannels == 2 )
	{
		if ( pSound->m_fmt.wBitsPerSample == 8 ) format = AL_FORMAT_STEREO8;
		else if ( pSound->m_fmt.wBitsPerSample == 16 ) format = AL_FORMAT_STEREO16;
		else
		{
			agk::Error( "Unsupported wav file bit size, only 8bit and 16bit supported." );
			return 0;
		}
	}
	else if ( pSound->m_fmt.nChannels == 1 )
	{
		if ( pSound->m_fmt.wBitsPerSample == 8 ) format = AL_FORMAT_MONO8;
		else if ( pSound->m_fmt.wBitsPerSample == 16 ) format = AL_FORMAT_MONO16;
		else
		{
			agk::Error( "Unsupported wav file bit size, only 8bit and 16bit supported." );
			return 0;
		}
	}
	else
	{
		agk::Error( "Unsupported wav file number of channels, only 1 (mono) and 2 (stereo) channels supported." );
		return 0;
	}

	m_cSoundInstances.AddItem( pSoundInst, pSoundInst->m_iID );

	alGenBuffers(1, &(pSoundInst->bufferID));
	alBufferData(pSoundInst->bufferID,  format, pSound->m_pRawData, pSound->m_uDataSize, pSound->m_fmt.nSamplesPerSec);
	alGenSources(1, &(pSoundInst->sourceID));
	alSourcei(pSoundInst->sourceID, AL_LOOPING, AL_FALSE);
	alSourcei(pSoundInst->sourceID, AL_SOURCE_RELATIVE, AL_TRUE);

	if ( pSoundInst->m_iLoop == 0 )
	{
		alSourcei(pSoundInst->sourceID, AL_BUFFER, pSoundInst->bufferID);
	}
	else
	{
		alSourceQueueBuffers( pSoundInst->sourceID, 1, &(pSoundInst->bufferID) );
		alSourceQueueBuffers( pSoundInst->sourceID, 1, &(pSoundInst->bufferID) );
	}

	float fVol = m_iGlobalVolume / 100.0f;
	fVol *= ( pSoundInst->m_iVolume / 100.0f );
	alSourcef(pSoundInst->sourceID, AL_GAIN, fVol);
	
	alSourcePlay(pSoundInst->sourceID);
	pSound->m_iInstances++;

	return pSoundInst->m_iID;
}

void cSoundMgr::PlatformStopInstances( UINT iID )
{
	cSoundInst *pNext = UNDEF;
	cSoundInst *pSound = m_pSounds;
	while ( pSound )
	{
		pNext = pSound->m_pNextInst;
		
		if ( iID == 0 || pSound->m_iParent == iID )
		{
			if ( m_pSoundFiles[ pSound->m_iParent ] ) m_pSoundFiles[ pSound->m_iParent ]->m_iInstances = 0;

			// stop sound, remove from playing list
			pSound->Reset();
			pSound->m_uLastUsed = agk::GetSeconds();
			if ( pSound->m_pPrevInst ) pSound->m_pPrevInst->m_pNextInst = pSound->m_pNextInst;
			else m_pSounds = pSound->m_pNextInst;
			
			m_cSoundInstances.RemoveItem( pSound->m_iID );

			if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound->m_pPrevInst;
			
			// add to head of use list
			pSound->m_pPrevInst = UNDEF;
			pSound->m_pNextInst = m_pUsedSounds;
			m_pUsedSounds = pSound;
			
			if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound;
		}
		
		pSound = pNext;
	}
}

void cSoundMgr::SetInstanceVolume( UINT instance, int vol )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return;

	if ( vol < 0 ) vol = 0;
	if ( vol > 100 ) vol = 100;

	pSound->m_iVolume = vol;

	float fVol = m_iGlobalVolume / 100.0f;
	fVol *= ( pSound->m_iVolume / 100.0f );

	alSourcef(pSound->sourceID, AL_GAIN, fVol);
}

void cSoundMgr::SetInstanceRate( UINT instance, float rate )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return;

	if ( rate < m_fMinPlaybackRate ) rate = m_fMinPlaybackRate;
	if ( rate > m_fMaxPlaybackRate ) rate = m_fMaxPlaybackRate;

	pSound->m_fRate = rate;

	alSourcef( pSound->sourceID, AL_PITCH, rate );
}

void cSoundMgr::SetInstanceBalance( UINT instance, float balance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return;

	if ( balance < -1 ) balance = -1;
	if ( balance > 1 ) balance = 1;

	pSound->m_fBalance = balance;

	// not possible on iOS with only 1 speaker?
}

int cSoundMgr::GetInstanceVolume( UINT instance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return 0;
	return pSound->m_iVolume;
}

float cSoundMgr::GetInstanceRate( UINT instance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return 0;
	return pSound->m_fRate;
}

int cSoundMgr::GetInstancePlaying( UINT instance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return 0;
	else return 1;
}

int cSoundMgr::GetInstanceLoopCount( UINT instance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return 0;
	return pSound->m_iLoopCount;
}

void cSoundMgr::StopInstance( UINT instance )
{
	cSoundInst *pSound = m_cSoundInstances.GetItem( instance );
	if ( !pSound ) return;

	if ( m_pSoundFiles[ pSound->m_iParent ] ) m_pSoundFiles[ pSound->m_iParent ]->m_iInstances--;

	// stop sound, remove from playing list
	pSound->Reset();
	pSound->m_uLastUsed = agk::GetSeconds();
	if ( pSound->m_pPrevInst ) pSound->m_pPrevInst->m_pNextInst = pSound->m_pNextInst;
	else m_pSounds = pSound->m_pNextInst;

	m_cSoundInstances.RemoveItem( pSound->m_iID );

	if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound->m_pPrevInst;

	// add to head of use list
	pSound->m_pPrevInst = UNDEF;
	pSound->m_pNextInst = m_pUsedSounds;
	m_pUsedSounds = pSound;

	if ( pSound->m_pNextInst ) pSound->m_pNextInst->m_pPrevInst = pSound;
}

// video commands
int agk::LoadVideo( const char *szFilename )
//****
{
	if ( !g_videoConnection )
	{
		g_videoConnection = mmr_connect(NULL);
		g_videoContext = mmr_context_create( g_videoConnection, "testvideoplayer", 0, S_IRWXU | S_IRWXG | S_IRWXO );

		char str[256];
		sprintf( str, "screen:?winid=%s&wingrp=%s", g_szWindowGroup, g_szWindowGroup );
		g_videoOutputID = mmr_output_attach( g_videoContext, str, "video" );
		g_audioOutputID = mmr_output_attach( g_videoContext, "audio:default", "audio");
		//mmr_output_parameters( g_mediaContext, audio_oid, NULL );

		mmrenderer_request_events( "testvideoplayer", 0, 1 );
	}

	g_sVideoFile.SetStr("");

	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );

	off_t start = 0;
	off_t length;
	int fd = open( sPath.GetStr(), O_RDONLY, 0777 );
	if ( fd < 0 )
	{
		// try asset folder
		sPath.SetStr( szFilename );
		agk::PlatformGetFullPathRead( sPath );

		fd = open( sPath.GetStr(), O_RDONLY, 0777 );
		if ( fd < 0 )
		{
			uString err( "Failed to load video file ", 50 );
			err.Append( szFilename );
			agk::Error( err );
			return 0;
		}
	}
	else
	{
		struct stat buf;
		fstat(fd, &buf);
		length = buf.st_size;
	}

	g_sVideoFile.SetStr(szFilename);

	sPath.Prepend( "file://" );

	mmr_input_attach( g_videoContext, sPath.GetStr(), "track" );

	g_videoPlaying = 0;
	g_videoPosition = 0;

	return 1;
}

void agk::ChangeVideoPointer( void *ptr )
{
	if ( !ptr ) return;

	videoData *data = (videoData*) ptr;

	cAutoLock autolock(&g_videoLock);

	if ( data->type == 0 )
	{
		g_videoWindow = *((screen_window_t*)data->window);
		SetVideoDimensions( m_fVideoX, m_fVideoY, m_fVideoWidth, m_fVideoHeight );
	}

	if ( data->type == 1 )
	{
		g_videoPosition = data->position;
		g_videoPositionTime = agk::Timer();
	}

	delete data;
}

void agk::HandleVideoEvents()
{
	g_videoPlaying = 0;

	cAutoLock autolock(&g_videoLock);

	if ( g_videoContext ) mmr_seek(g_videoContext,"0");

	if ( g_videoWindow )
	{
		int visible = 0; screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_VISIBLE, &visible);
	}
}

void agk::DeleteVideo()
//****
{
	g_sVideoFile.SetStr("");
	g_videoPlaying = 0;
	if ( !g_videoContext ) return;

	cAutoLock autolock(&g_videoLock);

	mmr_stop(g_videoContext);

	mmr_output_detach(g_videoContext, g_videoOutputID);
	mmr_output_detach(g_videoContext, g_audioOutputID);
	mmr_context_destroy(g_videoContext);

	g_videoContext = 0;
	g_videoOutputID = -1;
	g_audioOutputID = -1;

	mmr_disconnect( g_videoConnection );

	g_videoConnection = 0;

	if ( g_videoWindow )
	{
		int order = -1; screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_ZORDER, &order);
		int visible = 0; screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_VISIBLE, &visible);
		screen_destroy_window(g_videoWindow);
		g_videoWindow = 0;
	}
}

void agk::SetVideoDimensions( float x, float y, float width, float height )
//****
{
	m_fVideoX = x;
	m_fVideoY = y;
	m_fVideoWidth = width;
	m_fVideoHeight = height;

	if ( g_videoContext )
	{
		int iX = agk::ScreenToDeviceX( x );
		int iY = agk::ScreenToDeviceY( y );
		int iWidth = agk::ScreenToDeviceX( x+width ) - iX;
		int iHeight = agk::ScreenToDeviceY( y+height ) - iY;

		int size[2];
		size[0] = 100;
		size[1] = 100;

		strm_dict_t *dict = strm_dict_new();

		char buffer[16];
		dict = strm_dict_set(dict, "video_dest_x", itoa(iX,buffer,10));
		dict = strm_dict_set(dict, "video_dest_y", itoa(iY,buffer,10));
		dict = strm_dict_set(dict, "video_dest_w", itoa(iWidth,buffer,10));
		dict = strm_dict_set(dict, "video_dest_h", itoa(iHeight,buffer,10));

		mmr_output_parameters(g_videoContext, g_videoOutputID, dict);

		strm_dict_destroy(dict);
	}

	if ( g_videoWindow )
	{
		int order = 1;
		screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_ZORDER, &order);

		//screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_SIZE, size);
		//screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_BUFFER_SIZE, size);
	}
}

void agk::VideoUpdate()
{
	// do nothing
}

void agk::PlayVideoToImage( UINT imageID )
//****
{
	agk::Message("Video to texture is not currently supported on Blackberry");

	//m_iVideoPlayMode = 2;
}

void agk::PlayVideo()
//****
{
	if ( !g_videoContext ) return;

	g_videoPlaying = 1;
	mmr_play( g_videoContext );
	mmr_speed_set(g_videoContext, 1000);

	if ( g_videoWindow )
	{
		int visible = 1; screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_VISIBLE, &visible);
	}
}

void agk::PauseVideo()
//****
{
	g_videoPlaying = 0;
	g_videoPositionTime = 0;
	if ( !g_videoContext ) return;

	mmr_speed_set(g_videoContext, 0);

	m_iVideoPlayMode = 1;
}

void agk::StopVideo()
//****
{
	g_videoPlaying = 0;
	if ( !g_videoContext ) return;

	mmr_stop( g_videoContext );
	mmr_seek( g_videoContext, "0" );

	if ( g_videoWindow )
	{
		int visible = 0; screen_set_window_property_iv(g_videoWindow, SCREEN_PROPERTY_VISIBLE, &visible);
	}

	m_iVideoPlayMode = 0;
}

int agk::GetVideoPlaying()
//****
{
	return g_videoPlaying;
}

float agk::GetVideoPosition()
//****
{
	if ( g_videoPlaying && g_videoPositionTime > 0 )
	{
		float diff = agk::Timer() - g_videoPositionTime;
		g_videoPosition += (int)(diff*1000);
		g_videoPositionTime = agk::Timer();
	}

	return g_videoPosition / 1000.0f;
}

float agk::GetVideoDuration()
//****
{
	// hack for Hazard Perception app
	if ( g_sVideoFile.FindStr("intro.mp4") >= 0 ) return 75;
	if ( g_sVideoFile.FindStr("intro3.mp4") >= 0 ) return 146;

	uString clip;
	g_sVideoFile.SubString(clip,6,10);
	if ( clip.CompareTo("Hazardclip") == 0 )
	{
		g_sVideoFile.SubString(clip,16,2);
		int iClip = clip.ToInt();
		switch( iClip )
		{
			case 1: return 61;
			case 2: return 58;
			case 3: return 58;
			case 4: return 56;
			case 5: return 53;
			case 8: return 60;
			case 9: return 58;
			case 10: return 51;
			case 11: return 53;
			case 12: return 52;
			case 13: return 76;
			case 14: return 52;
			case 15: return 56;
			case 16: return 59;
			default: return 0;
		}
	}

	g_sVideoFile.SubString(clip,7,10);
	if ( clip.CompareTo("Hazard2Vid") == 0 )
	{
		g_sVideoFile.SubString(clip,17,2);
		int iClip = clip.ToInt();
		switch( iClip )
		{
			case 1: return 60;
			case 2: return 60;
			case 3: return 60;
			case 4: return 60;
			case 5: return 60;
			case 8: return 60;
			case 9: return 60;
			case 10: return 60;
			case 11: return 60;
			case 12: return 60;
			case 13: return 60;
			case 14: return 60;
			case 15: return 60;
			case 16: return 60;
			default: return 0;
		}
	}

	g_sVideoFile.SubString(clip,7,10);
	if ( clip.CompareTo("Hazard3Vid") == 0 )
	{
		g_sVideoFile.SubString(clip,17,2);
		int iClip = clip.ToInt();
		switch( iClip )
		{
			case 1: return 60;
			case 2: return 60;
			case 3: return 60;
			case 4: return 60;
			case 5: return 60;
			case 6: return 60;
			case 7: return 60;
			case 8: return 60;
			case 9: return 60;
			case 10: return 60;
			default: return 0;
		}
	}

	static int warned = 0;
	if ( warned == 0 )
	{
		warned = 1;
		//agk::Message("GetVideoDuration is not supported on Blackberry");
	}

	return 0;
}

void agk::SetVideoVolume( float volume )
//****
{
	// todo
}

float agk::GetVideoWidth()
//****
{
	// todo
	return -1;
}

float agk::GetVideoHeight()
//****
{
	// todo
	return -1;
}

void agk::SetVideoPosition( float seconds )
//****
{
	
}

// string

int uString::ToInt() const
{
	if ( m_iLength == 0 || !m_pData ) return 0;
	return atoi(m_pData);
}

float uString::ToFloat() const
{
	return (float)atof(m_pData);
}


void agk::PlatformReportError( const uString &sMsg )
{
	fprintf( stderr, "%s\n", sMsg.GetStr());
}

void agk::DialogGroup( const char* group )
{
	if ( !group ) return;
	strcpy( g_szWindowGroup, group );
}

void agk::PlatformMessage( const char* msg )
{
	static dialog_instance_t dialog = NULL;
	if ( dialog ) dialog_destroy(dialog);

	dialog_create_alert(&dialog);
	dialog_set_alert_message_text(dialog, msg);
	dialog_add_button(dialog, "Ok", true, NULL, true);
	dialog_set_title_text( dialog, "Message");
	dialog_set_group_id(dialog,g_szWindowGroup);
	dialog_show(dialog);
}

// Thread functions

void AGKThread::PlatformInit( )
{
	pThread = new pthread_t;
	m_pStop = new pthread_cond_t;
	
	pthread_condattr_t attr;

	pthread_condattr_init( &attr);
	pthread_condattr_setclock( &attr, CLOCK_MONOTONIC);
	pthread_cond_init( (pthread_cond_t*)m_pStop, &attr );
}

/*
UINT STDCALL StartNewThread( void* param )
{
	UINT result = ((AGKThread*)param)->EntryPoint( param );
	return result;
}
*/

void AGKThread::PlatformStart( )
{
	int result = pthread_create( (pthread_t*)pThread, NULL, (void*(*)(void*))EntryPoint, this );
	if ( result != 0 ) agk::Warning( "Failed to start pthread" );
}

void AGKThread::PlatformStop( )
{
	pthread_cond_signal( (pthread_cond_t*)m_pStop );
}

void AGKThread::PlatformTerminate( )
{
	if ( m_bRunning ) 
	{
		agk::Warning("Forcing a thread to terminate, this may cause a crash...");
		//pthread_cancel(*pThread);
	}
}

void AGKThread::PlatformCleanUp( )
{
	if ( m_pStop ) 
	{
		pthread_cond_destroy( (pthread_cond_t*)m_pStop );
		delete ((pthread_cond_t*)m_pStop);
	}
	m_pStop = 0;
	
	if ( pThread )
	{
		delete (pthread_t*)pThread;
		pThread = 0;
	}
}

void AGKThread::PlatformJoin( )
{
	if ( !pThread ) return; 
	pthread_join( *((pthread_t*)pThread), NULL );
}

void AGKThread::PlatformSleepSafe( UINT milliseconds )
{
	if ( m_bTerminate ) return;
	pthread_mutex_t mutex;
	pthread_mutex_init( &mutex, NULL );
	pthread_mutex_lock( &mutex );

	timespec waittime;
	clock_gettime(CLOCK_MONOTONIC, &waittime);

	waittime.tv_sec += milliseconds / 1000;
	waittime.tv_nsec += (milliseconds%1000) * 1000000;

	pthread_cond_timedwait( (pthread_cond_t*)m_pStop, &mutex, &waittime );
	pthread_mutex_unlock( &mutex );
	pthread_mutex_destroy( &mutex );
}

// filesystem commands

void agk::SetRawWritePath( const char* str )
//****
{
	// do nothing
	return;
}

char* agk::GetWritePath()
{
	// allow write directory to be determined
	char *szStr = new char[ strlen(szWriteDir)+1 ];
	strcpy( szStr, szWriteDir );
	return szStr;
}

char* agk::GetReadPath()
{
	char *szStr = new char[ strlen(szRootDir)+1 ];
	strcpy( szStr, szRootDir );
	return szStr;
}

char* agk::GetDocumentsPath()
{
	char *szStr = new char[ strlen(szWriteDir)+1 ];
	strcpy( szStr, szWriteDir );
	return szStr;
}

bool agk::PlatformChooseFile( uString &out, const char *ext )
{
	// do nothing on blackberry
}

void agk::PlatformGetFullPathWrite( uString &inout )
{
	inout.Replace( '\\', '/' );
	if ( inout.FindStr( "../" ) >= 0 )
	{
		uString sub;
		inout.SubString( sub, inout.RevFind( '/' )+1 );
		inout.SetStr( sub );
	}

	if ( inout.CharAt( 0 ) == '/' )
	{
		uString sub;
		inout.SubString( sub, 1 );
		inout.SetStr( sub );
		inout.Prepend( szWriteDir );
	}
	else
	{
		inout.Prepend( m_sCurrentDir );
		inout.Prepend( szWriteDir );
	}

	cFileEntry::ConvertToReal( inout );
}

void agk::PlatformGetFullPathRead( uString &inout )
{
	inout.Replace( '\\', '/' );
	if ( inout.FindStr( "../" ) >= 0 )
	{
		uString sub;
		inout.SubString( sub, inout.RevFind( '/' )+1 );
		inout.SetStr( sub );
	}

	if ( inout.CharAt( 0 ) == '/' )
	{
		uString sub;
		inout.SubString( sub, 1 );
		inout.SetStr( sub );
		inout.Prepend( szRootDir );
	}
	else
	{
		inout.Prepend( m_sCurrentDir );
		inout.Prepend( szRootDir );
	}

	cFileEntry::ConvertToReal( inout );
}

void agk::PlatformCreatePath( uString path )
{
	path.Replace( '\\', '/' );
	if ( path.FindStr( "../" ) >= 0 )
	{
		uString sub;
		path.SubString( sub, path.RevFind( '/' )+1 );
		path.SetStr( sub );
	}
	if ( path.GetLength() + agk::m_sCurrentDir.GetLength() >= 1024 ) return;

	chdir( szWriteDir );

	// check if directory exists
	char checkPath[ 1024 ];
	if ( path.CharAt(0) == '/' || path.CharAt(0) == '\\' )
	{
		strcpy( checkPath, path.GetStr()+1 );
	}
	else
	{
		strcpy( checkPath, agk::m_sCurrentDir );
		strcat( checkPath, path.GetStr() );
	}

	const char *szRemaining = checkPath;
	const char *szSlash;
	char szFolder[ MAX_PATH ];
	while ( szSlash = strchr( szRemaining, '/' ) )
	{
		UINT length = (UINT)(szSlash-szRemaining);
		if ( length == 0 )
		{
#ifdef _AGK_ERROR_CHECK
			agk::Error( "Invalid path for OpenToWrite file, must not have empty folders" );
#endif
			return;
		}

		strncpy( szFolder, szRemaining, length );
		szFolder[ length ] = '\0';

		if ( chdir( szFolder ) < 0 )
		{
			mkdir( szFolder, 0777 );
			chdir( szFolder );
		}

		szRemaining = szSlash+1;
	}

	chdir( szWriteDir );
}

bool AGK::cFile::ExistsWrite( const char *szFilename )
{
	if ( !szFilename || !*szFilename ) return false;
	if ( strchr( szFilename, ':' ) ) return false;
	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") ) return false;

	UINT length = strlen(szFilename);
	if ( szFilename[length-1] == '/' || szFilename[length-1] == '\\' ) return false;

	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );

	struct stat buf;
	if ( stat( sPath.GetStr(), &buf ) != 0 ) return false;

	return true;
}

bool AGK::cFile::ExistsRead( const char *szFilename )
{
	if ( !szFilename || !*szFilename ) return false;
	if ( strchr( szFilename, ':' ) ) return false;
	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") ) return false;

	UINT length = strlen(szFilename);
	if ( szFilename[length-1] == '/' || szFilename[length-1] == '\\' ) return false;

	uString sPath( szFilename );
	agk::PlatformGetFullPathRead( sPath );

	struct stat buf;
	if ( stat( sPath.GetStr(), &buf ) != 0 ) return false;

	return true;
}

bool AGK::cFile::Exists( const char *szFilename )
{
	if ( !ExistsWrite( szFilename ) )
	{
		if ( !ExistsRead( szFilename ) ) return false;
	}

	return true;
}

bool AGK::cFile::GetModified( const char *szFilename, int &time )
{
	time = 0;
	if ( !szFilename || !*szFilename ) return false;
	if ( strchr( szFilename, ':' ) ) return false;
	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") ) return false;
	
	UINT length = strlen(szFilename);
	if ( szFilename[length-1] == '/' || szFilename[length-1] == '\\' ) return false;
	
	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );
	
	struct stat fileInfo;
	int result = stat( sPath, &fileInfo );
	if ( result != 0 )
	{
		sPath.SetStr( szFilename );
		agk::PlatformGetFullPathRead( sPath );

		int result = stat( sPath, &fileInfo );
		if ( result != 0 )
		{
			return false;
		}
	}
	
	time = (int) fileInfo.st_mtime;
	return true;
}

void cFile::SetModified( const char *szFilename, int time )
{
	if ( !szFilename || !*szFilename ) return;
	if ( strchr( szFilename, ':' ) ) return;
	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") ) return;

	UINT length = strlen(szFilename);
	if ( szFilename[length-1] == '/' || szFilename[length-1] == '\\' ) return;

	uString sPath( szWriteDir, MAX_PATH );
	if ( szFilename[0] == '/' || szFilename[0] == '\\' )
	{
		sPath.Trunc( '/' );
		sPath += szFilename;
	}
	else
	{
		sPath += agk::m_sCurrentDir;
		sPath += szFilename;
	}
	cFileEntry::ConvertToReal( sPath );

	struct utimbuf fileInfo;
	fileInfo.actime = time;
	fileInfo.modtime = time;
	utime( sPath.GetStr(), &fileInfo );
}

UINT AGK::cFile::GetFileSize( const char *szFilename )
{
	cFile pFile;
	if ( !pFile.OpenToRead( szFilename ) ) return 0;
	UINT size = pFile.GetSize();
	pFile.Close();
	
	return size;
}

void AGK::cFile::DeleteFile( const char *szFilename )
{
	if ( !szFilename || !*szFilename ) return;
	
	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") )
	{
		agk::Error( "Invalid path for DeleteFile, must not traverse backwards up the directory tree using ../" );
		return;
	}
	
	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );
	
	remove( sPath.GetStr() );
	agk::m_bUpdateFileLists = true;
}

bool AGK::cFile::OpenToWrite( const char *szFilename, bool append )
{
	if ( !szFilename || !*szFilename ) return false;
	if ( pFile ) Close();
	mode = 1;

	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") )
	{
		uString sErr( "Invalid path for OpenToWrite file, must not traverse backwards up the directory tree using ../  " );
		sErr += szFilename;
		agk::Error( sErr );
		return false;
	}
	
	UINT length = strlen( szFilename );
	if ( szFilename[ length-1 ] == '/' || szFilename[ length-1 ] == '\\' )
	{
		agk::Error( "Invalid path for OpenToWrite file, must not end in a forward or backward slash" );
		return false;
	}
	
	chdir( szWriteDir );

	uString sTemp( szFilename );
	sTemp.Replace( '\\', '/' );

	// check if directory exists
	char checkPath[ 1024 ];
	if ( szFilename[0] == '/' || szFilename[0] == '\\' )
	{
		strcpy( checkPath, sTemp.GetStr()+1 );
	}
	else
	{
		strcpy( checkPath, agk::m_sCurrentDir );
		strcat( checkPath, sTemp.GetStr() );
	}

	const char *szRemaining = checkPath;
	const char *szSlash;
	char szFolder[ MAX_PATH ];
	while ( szSlash = strchr( szRemaining, '/' ) )
	{
		UINT length = (UINT)(szSlash-szRemaining);
		if ( length == 0 )
		{
#ifdef _AGK_ERROR_CHECK
			agk::Error( "Invalid path for OpenToWrite file, must not have empty folders, and must not begin with a forward or backward slash" );
#endif
			return false;
		}
		
		strncpy( szFolder, szRemaining, length );
		szFolder[ length ] = '\0';

		if ( chdir( szFolder ) < 0 )
		{
			mkdir( szFolder, 0777 );
			chdir( szFolder );
		}

		szRemaining = szSlash+1;
	}

	chdir( szWriteDir );
		
	// write to my documents folder only
	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );
	
	pFilePtr = 0;
	if ( append ) pFile = fopen( sPath.GetStr(), "ab" );
	else pFile = fopen( sPath.GetStr(), "wb" );
	
	if ( !pFile )
	{
		uString err = "Failed to open file for writing ";
		err += szFilename;
		agk::Error( err );
		return false;
	}
	
	// refresh any stored directory details for the new file
	cFileEntry::AddNewFile( sPath.GetStr() );
	//agk::ParseCurrentDirectory();
	agk::m_bUpdateFileLists = true;
	
	return true;
}

bool AGK::cFile::OpenToRead( const char *szFilename )
{
	if ( !szFilename || !*szFilename ) return false;
	if ( pFile ) Close();
	mode = 0;

	if ( strstr(szFilename, "..\\") || strstr(szFilename, "../") )
	{
		agk::Error( "Invalid path for OpenToRead file, must not traverse backwards up the directory tree using ../" );
		return false;
	}
	
	UINT length = strlen( szFilename );
	if ( szFilename[ length-1 ] == '/' || szFilename[ length-1 ] == '\\' )
	{
		agk::Error( "Invalid path for OpenToRead file, must not end in a forward or backward slash" );
		return false;
	}
	
	// attempt read from my documents folder first, then exe directory, otherwise fail
	uString sPath( szFilename );
	agk::PlatformGetFullPathWrite( sPath );
	
	pFilePtr = 0;
	pFile = fopen( sPath, "rb" );
	if ( !pFile )
	{
		// try asset folder
		sPath.SetStr( szFilename );
		agk::PlatformGetFullPathRead( sPath );

		pFile = fopen( sPath, "rb" );
	}
	
	if ( !pFile )
	{
		uString err = "Failed to open file for reading ";
		err += szFilename;
		agk::Error( err );
		return false;
	}
	
	return true;
}

void AGK::cFile::Close()
{
	if ( pFile ) fclose( pFile );
	pFile = UNDEF;
}

UINT AGK::cFile::GetPos()
{
	if ( !pFile ) return 0;

	return ftell( pFile );
}

void AGK::cFile::Seek( UINT pos )
{
	if ( !pFile ) return;
	fseek( pFile, pos, SEEK_SET );
}

void AGK::cFile::Flush()
{
	if ( !pFile ) return;
	fflush( pFile );
}

UINT AGK::cFile::GetSize()
{
	if ( !pFile ) return 0;
	fpos_t pos;
	fgetpos( pFile, &pos );
	fseek( pFile, 0, SEEK_END );
	UINT size = ftell( pFile );
	fsetpos( pFile, &pos );
	return size;
}

void AGK::cFile::Rewind()
{
	if ( !pFile ) return;
	rewind( pFile );
}

bool AGK::cFile::IsEOF()
{
	if ( !pFile ) return true;
	return feof( pFile ) != 0;
}

void cFile::WriteByte( unsigned char b )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot write to file opened for reading" );
#endif
		return;
	}

	fwrite( &b, 1, 1, pFile );
}

void AGK::cFile::WriteInteger( int i )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
		agk::Error( "Cannot write to file opened for reading" );
		return;
	}
	
	//convert everything to little endian for cross platform compatibility
	i = agk::PlatformLittleEndian( i );
	fwrite( &i, 4, 1, pFile );
}

void AGK::cFile::WriteFloat( float f )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
		agk::Error( "Cannot write to file opened for reading" );
		return;
	}
	fwrite( &f, 4, 1, pFile );
}

void AGK::cFile::WriteString( const char *str )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
		agk::Error( "Cannot write to file opened for reading" );
		return;
	}
	UINT length = strlen( str );
	fwrite( str, 1, length+1, pFile );
}

void cFile::WriteString2( const char *str )
{
	if ( !str ) return;
	if ( !pFile ) return;
	if ( mode != 1 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot write to file opened for reading" );
#endif
		return;
	}
	UINT length = strlen( str );
	UINT l = agk::PlatformLittleEndian( length );
	fwrite( &l, 4, 1, pFile );
	fwrite( str, 1, length, pFile );
}

void AGK::cFile::WriteData( const char *str, UINT bytes )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
		agk::Error( "Cannot write to file opened for reading" );
		return;
	}
	
	fwrite( str, 1, bytes, pFile );
}

void AGK::cFile::WriteLine( const char *str )
{
	if ( !pFile ) return;
	if ( mode != 1 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot write to file opened for reading" );
#endif
		return;
	}
	UINT length = strlen( str );
	fwrite( str, 1, length, pFile );

	// strings terminate with CR (13,10) - so it resembles a regular text file when file viewed
	char pCR[2];
	pCR[0]=13;
	pCR[1]=10;
	fwrite( &pCR[0], 1, 1, pFile );
	fwrite( &pCR[1], 1, 1, pFile );
}

unsigned char cFile::ReadByte( )
{
	if ( !pFile ) return 0;
	if ( mode != 0 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file opened for writing" );
#endif
		return 0;
	}
	unsigned char b = 0;
	fread( &b, 1, 1, pFile );
	return b;
}

int AGK::cFile::ReadInteger( )
{
	if ( !pFile ) return 0;
	if ( mode != 0 )
	{
		agk::Error( "Cannot read from file opened for writing" );
		return 0;
	}
	int i = 0;
	fread( &i, 4, 1, pFile );
	// convert back to local endian, everything in the file is little endian.
	return i = agk::PlatformLocalEndian( i );
}

float AGK::cFile::ReadFloat( )
{
	if ( !pFile ) return 0;
	if ( mode != 0 )
	{
		agk::Error( "Cannot read from file opened for writing" );
		return 0;
	}
	float f;
	fread( &f, 4, 1, pFile );
	return f;
}

int AGK::cFile::ReadString( uString &str )
{
	if ( !pFile ) return 0;
	if ( mode != 0 )
	{
		agk::Error( "Cannot read from file opened for writing" );
		return 0;
	}
	
	str.SetStr( "" );
	
	// read until a null terminator is found, or eof.
	bool bNullFound = false;
	do
	{
		long lPos = GetPos();//ftell( pFile );
		UINT written = ReadData( readbuf, 256 );
		for ( UINT i = 0; i < written; i++ )
		{
			if ( readbuf[ i ] == 0 ) 
			{
				bNullFound = true;
				Seek( lPos+i+1 );
				if ( i > 0 ) str.Append( readbuf );
				break;
			}
		}
		
		if ( !bNullFound ) str.AppendN( readbuf, written );
		else break;
		
	} while( !IsEOF() );
	
	return str.GetLength();
}

int cFile::ReadString2( uString &str )
{
	if ( !pFile ) 
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file, file not open" );
#endif
		return -1;
	}

	if ( mode != 0 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file opened for writing" );
#endif
		return -1;
	}

	str.ClearTemp();

	UINT length = 0;
	fread( &length, 4, 1, pFile );
	// convert back to local endian, everything in the file is little endian.
	length = agk::PlatformLocalEndian( length );

	char buffer[ 256 ];
	while ( length > 256 )
	{
		fread( buffer, 1, 256, pFile );
		str.AppendN( buffer, 256 );
		length -= 256;
	}

	fread( buffer, 1, length, pFile );
	str.AppendN( buffer, length );
	return str.GetLength();
}

int AGK::cFile::ReadLine( uString &str )
{
	if ( !pFile ) 
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file, file not open" );
#endif
		return -1;
	}

	if ( mode != 0 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file opened for writing" );
#endif
		return -1;
	}

	str.SetStr( "" );
	
	// read until a CR (13,10) line terminator is found, or eof.
	// so it resembles a regular text file when file viewed
	bool bNullFound = false;
	do
	{
		long lPos = GetPos();//ftell( pFile );
		UINT written = ReadData( readbuf, 256 );
		for ( UINT i = 0; i < written; i++ )
		{
			if ( readbuf[ i ] == '\n' ) 
			{
				bNullFound = true;
				fseek( pFile, lPos+i+1, SEEK_SET );
				str.AppendN ( readbuf, i );
				break;
			}
		}
		
		if ( !bNullFound ) str.AppendN( readbuf, written );
		else break;

	} while( !IsEOF() );

	str.Trim( "\r\n" );
	return str.GetLength();
}

int AGK::cFile::ReadLineFast( uString &str )
{
	if ( !pFile ) 
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file, file not open" );
#endif
		return -1;
	}

	if ( mode != 0 )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Cannot read from file opened for writing" );
#endif
		return -1;
	}

	str.ClearTemp();

	char tempstr[ 1024 ];
	if ( !fgets( tempstr, 1024, pFile ) ) return 0;
	str.SetStr( tempstr );

	while ( !IsEOF() && str.CharAt( str.GetLength()-1 ) != '\n' )
	{
		if ( !fgets( tempstr, 1024, pFile ) ) break;
		str.Append( tempstr );
	}

	str.Trim( "\n\r" );
	return str.GetLength();
}

int AGK::cFile::ReadData( char *str, UINT length )
{
	if ( !pFile ) return 0;
	if ( mode != 0 )
	{
		agk::Error( "Cannot read from file opened for writing" );
		return 0;
	}
	
	return fread( str, 1, length, pFile );
}

void agk::ParseCurrentDirectory()
{
	m_bUpdateFileLists = false;

	// clear any old directories
	while ( m_pCurrentDirectories )
	{
		cDirectoryItem *pItem = m_pCurrentDirectories;
		m_pCurrentDirectories = m_pCurrentDirectories->m_pNext;
		delete pItem;
	}
	
	m_pCurrentDirectoryIter = 0;
	
	// clear any old files
	while ( m_pCurrentFiles )
	{
		cDirectoryItem *pItem = m_pCurrentFiles;
		m_pCurrentFiles = m_pCurrentFiles->m_pNext;
		delete pItem;
	}
	
	m_pCurrentFileIter = 0;
	
	// check root directory for files and directories (non-recursive)
	uString sPath( szRootDir );
	sPath.Append( m_sCurrentDir );
	cFileEntry::ConvertToReal( sPath );
	
	// get files and directories from root folder
	DIR *pDir = opendir( sPath.GetStr() );
	if ( pDir )
	{
		dirent* item = readdir( pDir );
		while( item )
		{
			struct stat info;
			uString sFilePath( sPath );
			sFilePath.Append( item->d_name );
			if ( stat( sFilePath.GetStr(), &info ) == 0 )
			{
				//uString sDir;
				//sDir.Format( "%s - %d", item->d_name, info.st_mode );
				//agk::Warning( sDir );

				if ( (info.st_mode & S_IFDIR) )
				{
					// directory
					if ( strcmp( item->d_name, "." ) != 0 && strcmp( item->d_name, ".." ) != 0 )
					{
						cDirectoryItem *pNewItem = new cDirectoryItem();
						pNewItem->m_sItem.SetStr( item->d_name );
						pNewItem->m_pNext = m_pCurrentDirectories;
						m_pCurrentDirectories = pNewItem;
					}
				}
				else if ( (info.st_mode & S_IFREG) )
				{
					// file
					cDirectoryItem *pNewItem = new cDirectoryItem();
					pNewItem->m_sItem.SetStr( item->d_name );
					pNewItem->m_pNext = m_pCurrentFiles;
					m_pCurrentFiles = pNewItem;
				}
			}

			item = readdir( pDir );
		}

		closedir( pDir );
	}

	// check write directory for files and directories (non-recursive)
	sPath.SetStr( szWriteDir );
	sPath.Append( m_sCurrentDir );
	cFileEntry::ConvertToReal( sPath );
	
	pDir = opendir( sPath.GetStr() );
	if ( pDir )
	{
		dirent* item = readdir( pDir );
		while( item )
		{
			struct stat info;
			uString sFilePath( sPath );
			sFilePath.Append( item->d_name );
			if ( stat( sFilePath.GetStr(), &info ) == 0 )
			{
				if ( (info.st_mode & S_IFDIR) )
				{
					// directory
					if ( strcmp( item->d_name, "." ) != 0 && strcmp( item->d_name, ".." ) != 0 )
					{
						bool bExists = false;
						cDirectoryItem *pItem = m_pCurrentDirectories;
						while ( pItem )
						{
							if ( pItem->m_sItem.CompareTo( item->d_name ) == 0 )
							{
								bExists = true;
								break;
							}
							pItem = pItem->m_pNext;
						}

						if ( !bExists )
						{
							cDirectoryItem *pNewItem = new cDirectoryItem();
							pNewItem->m_sItem.SetStr( item->d_name );
							pNewItem->m_pNext = m_pCurrentDirectories;
							m_pCurrentDirectories = pNewItem;
						}
					}
				}
				else if ( (info.st_mode & S_IFREG) )
				{
					// file
					bool bExists = false;
					cDirectoryItem *pItem = m_pCurrentFiles;
					while ( pItem )
					{
						if ( pItem->m_sItem.CompareTo( item->d_name ) == 0 )
						{
							bExists = true;
							break;
						}
						pItem = pItem->m_pNext;
					}

					if ( !bExists )
					{
						cDirectoryItem *pNewItem = new cDirectoryItem();
						pNewItem->m_sItem.SetStr( item->d_name );
						pNewItem->m_pNext = m_pCurrentFiles;
						m_pCurrentFiles = pNewItem;
					}
				}
			}

			item = readdir( pDir );
		}

		closedir( pDir );
	}
}

int agk::SetCurrentDir( const char* szPath )
{
	if ( !szPath || strlen( szPath ) == 0 )
	{
		m_sCurrentDir.SetStr( "" );
		//ParseCurrentDirectory();
		m_bUpdateFileLists = true;
		return 1;
	}
	
	if ( strcmp( szPath, ".." ) == 0 )
	{
		UINT pos = m_sCurrentDir.Find( '/' );
		if ( pos >= 0 && pos < m_sCurrentDir.GetLength()-1 )
		{
			m_sCurrentDir.Trunc( '/' );
			m_sCurrentDir.Trunc( '/' );
			m_sCurrentDir.Append( '/' );
		}
		else
		{
			m_sCurrentDir.SetStr( "" );
		}
		//ParseCurrentDirectory();
		m_bUpdateFileLists = true;
		return 1;
	}

	if ( strstr(szPath, "..") )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Invalid path for SetCurrentDir, must not traverse backwards up the directory tree using ../" );
#endif
		return 0;
	}
	
	uString sPath( szPath );

	if ( szPath[0] == '\\' || szPath[0] == '/' )
	{
		m_sCurrentDir.SetStr( "" );
		sPath.SetStr( szPath+1 );
	}
	
	sPath.Replace( '\\', '/' );

	// add a final slash
	UINT length = strlen( szPath );
	if ( szPath[ length-1 ] != '/' && szPath[ length-1 ] != '\\' ) sPath.Append( '/' );

	m_sCurrentDir.Append( sPath );
	m_sCurrentDir.Replace( '\\', '/' );
	
	//ParseCurrentDirectory();
	m_bUpdateFileLists = true;
	
	return 1;
}

int agk::MakeFolder( const char* szName )
{
	chdir( szWriteDir );
	uString sTemp( szName );
	sTemp.Replace( '\\', '/' );
	sTemp.Trunc( '/' );
	sTemp.Append( '/' );

	// check if directory exists
	char checkPath[ 1024 ];
	if ( szName[0] == '/' || szName[0] == '\\' )
	{
		strcpy( checkPath, sTemp.GetStr()+1 );
	}
	else
	{
		strcpy( checkPath, agk::m_sCurrentDir );
		strcat( checkPath, sTemp.GetStr() );
	}

	const char *szRemaining = checkPath;
	const char *szSlash;
	char szFolder[ MAX_PATH ];
	while ( szSlash = strchr( szRemaining, '/' ) )
	{
		UINT length = (UINT)(szSlash-szRemaining);
		if ( length == 0 )
		{
#ifdef _AGK_ERROR_CHECK
			agk::Error( "Invalid path for MakeFolder, path must not contain empty folders" );
#endif
			return 0;
		}

		strncpy( szFolder, szRemaining, length );
		szFolder[ length ] = '\0';

		if ( chdir( szFolder ) < 0 )
		{
			mkdir( szFolder, 0777 );
			chdir( szFolder );
		}

		szRemaining = szSlash+1;
	}

	chdir( szWriteDir );

	return 1;
}

void agk::DeleteFolder( const char* szName )
{
	if ( !szName || strlen( szName ) == 0 )
	{
		return;
	}

	// no special characters
	if ( strchr( szName, ':' ) || strchr( szName, '/' ) || strchr( szName, '\\' ) || strstr( szName, ".." )  )
	{
#ifdef _AGK_ERROR_CHECK
		agk::Error( "Invalid folder name for DeleteFolder, it must not contain the special characters / : \\ .." );
#endif
		return;
	}

	uString sDirPath( szWriteDir );
	sDirPath.Append( m_sCurrentDir );
	if ( chdir( sDirPath.GetStr() ) < 0 ) return;

	rmdir( szName );
	chdir( szWriteDir );

	m_bUpdateFileLists = true;
}

// input commands
int agk::GetMultiTouchExists()
{
	return 1;
}

int agk::GetMouseExists()
{
	return 0;
}

int agk::GetKeyboardExists()
{
	return 2;
}

int agk::GetCameraExists()
{
	return 1; // todo
}

void agk::SetRawMouseVisible( int visible )
//****
{
	// do nothing
}

void agk::SetRawMousePosition( float x, float y )
//****
{
	// do nothing
}

void cJoystick::DetectJoysticks()
{

}

void cJoystick::PlatformUpdate()
{

}

INT64 agk::GetUnixTime64()
{
	time_t unix = time(NULL);
	return unix;
}

int agk::GetUnixTime()
{
	time_t unix = time(NULL);
	if ( unix > 2147483646 ) unix = 2147483647;
	if ( unix < -2147483647 ) unix = -2147483647;
	return (int) unix;
}

int agk::GetDayOfWeek()
//****
{
	time_t a = time ( NULL );
	tm*    b = localtime ( &a );
	int    c = b->tm_wday;
	return c;
}

char* agk::GetCurrentDate()
//****
{
	time_t a = time ( NULL );
	tm*    b = localtime ( &a );

	int year = b->tm_year + 1900;
	int month = b->tm_mon+1;
	int days = b->tm_mday;

	uString date;
	date.Format( "%04d-%02d-%02d", year, month, days );

	char* str = new char[ date.GetLength()+1 ];
	strcpy( str, date.GetStr() );
	return str;
}

char* agk::GetCurrentTime()
//****
{
	time_t a = time ( NULL );
	tm*    b = localtime ( &a );

	int hours = b->tm_hour;
	int minutes = b->tm_min;
	int seconds = b->tm_sec;

	uString time;
	time.Format( "%02d:%02d:%02d", hours, minutes, seconds );

	char* str = new char[ time.GetLength()+1 ];
	strcpy( str, time.GetStr() );
	return str;
}

// advert commands

int agk::PlatformGetAdPortal()
{
	//return 659; // Windows Phone 7
	//return 642; // iPhone
	//return 551; // Ovi
	//return 559; // Android
	return 635; // Blackberry
	//return 641; // SonyEricsson
	//return 661; // Bada
	//return 738; // Palm
	//return 818; // Samsung
	//return 947; // iPad, doesn't seem to produce ads
}

///

void cEditBox::PlatformStartText()
{
	if ( !m_bUseAlternateInput || m_fY < agk::GetVirtualHeight()/2 ) 
	{
		g_bEditBoxHack = true;
		virtualkeyboard_change_options( VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT );
		virtualkeyboard_show();
	}
	else
	{
		agk::StartTextInput( m_sCurrInput );
		g_bPasswordMode = m_bIsPassword;
	}
}

void cEditBox::PlatformEndText()
{
	if ( g_bEditBoxHack ) virtualkeyboard_hide();
}

bool cEditBox::PlatformUpdateText()
{
	if ( g_bEditBoxHack ) return false;
	else
	{
		agk::GetTextInput( m_sCurrInput );
		    
		bool bChanged = false;
    
		m_pInputText->SetString( m_sCurrInput );
		while ( (m_iMaxLines > 0 && m_pInputText->GetLines() > m_iMaxLines) || (m_iMaxChars > 0 && m_sCurrInput.GetLength() >= m_iMaxChars) )
		{
			m_sCurrInput.Trunc2( 1 );
			m_pInputText->SetString( m_sCurrInput );
			bChanged = true;
		}
	    
		if ( !m_bSupportsExtendedAscii )
		{
			int length = m_sCurrInput.GetLength();
			m_sCurrInput.StripUTF8();
			if ( length != m_sCurrInput.GetLength() )
			{
				if ( !m_bSupportWarned )
				{
					m_bSupportWarned = true;
					agk::Message("This edit box does not support extended characters");
				}
				m_pInputText->SetString( m_sCurrInput );
				bChanged = true;
			}
		}
	    
		if ( bChanged )
		{
			agk::PlatformChangeTextInput( m_sCurrInput );
		}
	    
		m_iCursorPos = m_sCurrInput.GetLength();
		
		return true;
	}
}

void cEditBox::PlatformUpdateTextEnd()
{
	// do nothing
}

void agk::OpenBrowser( const char *url )
//****
{
	if ( !url || strlen(url) == 0 ) return;
	navigator_invoke(url, NULL);
}

UINT agk::RunApp( const char *szFilename, const char *szParameters )
{
	return 0;
}

UINT agk::GetAppRunning( UINT appID )
{
	return 0;
}

void agk::TerminateApp( UINT appID )
{

}

void agk::ViewFile( const char* szFilename )
{
	
}

void agk::ShareText( const char* szText )
{
	
}

void agk::ShareImage( const char* szFilename )
{
	
}

int agk::GetInternetState()
//****
{
	bool avail;
	netstatus_get_availability(&avail);
	return avail ? 1 : 0;
}

void agk::SetPushNotificationKeys( const char* data1, const char* reserved )
{
	// do nothing on blackberry
}

int agk::PushNotificationSetup()
//****
{
	return 0;
}

char* agk::GetPushNotificationToken()
//****
{
	char *str = new char[ 7 ];
	strcpy( str, "Error" );
	return str;
}

// ********************
// Social commands
// ********************

// todo all from here

// internally called by AGK
void agk::PlatformSocialPluginsSetup( void )
{
	// do nothing
}

void agk::PlatformSocialPluginsDestroy( void )
{
	// do nothing
}

// RATING COMMANDS
void agk::PlatformRateApp( const char* szID, const char* title, const char* message )
{
}

// IN APP PURCHASE COMMANDS
#define MAX_PRODUCTS 10
static int	productCount = 0;
static char	productID [ MAX_PRODUCTS ] [ 128 ];
static int	purchasedProducts [ MAX_PRODUCTS ];
static int lastProduct = -1;

void agk::PlatformInAppPurchaseSetKeys( const char* szData1, const char* szData2 )
{
	// do nothing
}

void agk::PlatformInAppPurchaseSetTitle( const char* szTitle )
{
	// do nothing
}

void  agk::PlatformInAppPurchaseAddProductID    ( const char* szID, int type )
{

}

void  agk::PlatformInAppPurchaseSetup           ( void )
{

}

void  agk::PlatformInAppPurchaseActivate        ( int iID )
{

}

int   agk::PlatformGetInAppPurchaseState        ( void )
{
	return 1;
}

int   agk::PlatformGetInAppPurchaseAvailable    ( int iID )
{
	return 0;
}

char* agk::PlatformGetInAppPurchaseLocalPrice( int iID )
{
	char *str = new char[1];
	*str = 0;
	return str;
}

char* agk::PlatformGetInAppPurchaseDescription( int iID )
{
	char *str = new char[1];
	*str = 0;
	return str;
}

bool  agk::PlatformHasInAppPurchase             ( void )
{
	return false;
}

void agk::PlatformInAppPurchaseRestore()
{
	
}


// ADMOB COMMANDS

// szID: publisher ID provided by AdMob
// horz: 0=left, 1=center, 2=right
// vert: 0=top, 1=center, 2=right
// offsetX: AGK distance to offset the horz position from the left or right edge. Ignored when centered
// offsetY: AGK distance to offset the vert position from the top or bottom edge. Ignored when centered
void agk::PlatformAdMobSetupRelative( const char* szID, int horz, int vert, float offsetX, float offsetY )
{

}

void  agk::PlatformAdMobFullscreen ()
{
}

void  agk::PlatformAdMobCacheFullscreen ()
{
}

void  agk::PlatformSetAdMobVisible ( int iVisible )
{

}

void  agk::PlatformAdMobPosition ( int horz, int vert, float offsetX, float offsetY )
{

}

void agk::PlatformAdMobRequestNewAd( void )
{

}

void agk::PlatformAdMobDestroy( void )
{

}

bool agk::PlatformHasAdMob( void )
{
	return false;
}

int  agk::PlatformAdMobGetFullscreenLoaded ()
{
	return 0;
}

// CHARTBOOST

void  agk::PlatformChartboostSetup ()
{
}

void  agk::PlatformChartboostFullscreen ()
{
}

int  agk::PlatformChartboostGetFullscreenLoaded ()
{
	return 0;
}

// Amazon Ads

void  agk::PlatformAmazonAdSetup ()
{

}

void agk::PlatformAmazonAdSetTesting( int testing )
{

}

void  agk::PlatformAmazonAdFullscreen ()
{

}

int  agk::PlatformAmazonGetFullscreenLoaded ()
{
	return 0;
}

// FACEBOOK COMMANDS
void  agk::PlatformFacebookSetup                ( const char* szID )
{

}

void RefreshAccessToken()
{

}

void  agk::PlatformFacebookLogin                ( void )
{

}

void  agk::PlatformFacebookLogout               ( void )
{

}

void  agk::PlatformFacebookShowLikeButton       ( const char* szURL, int iX, int iY, int iWidth, int iHeight )
{
}

void  agk::PlatformFacebookDestroyLikeButton    ( void )
{
}

void  agk::PlatformFacebookPostOnMyWall         ( const char* szLink, const char* szPicture, const char* szName, const char* szCaption, const char* szDescription )
{

}

void  agk::PlatformFacebookPostOnFriendsWall    ( const char* szID, const char* szLink, const char* szPicture, const char* szName, const char* szCaption, const char* szDescription )
{

}

void  agk::PlatformFacebookInviteFriend         ( const char* szID, const char* szMessage )
{
}

void  agk::PlatformFacebookGetFriends           ( void )
{

}

int FacebookStringCompare( const void* a, const void* b )
{
	//FacebookUser* user1 = (FacebookUser*)a;
	//FacebookUser* user2 = (FacebookUser*)b;

	//return user1->name.CompareTo( user2->name );
	return 0;
}

int   agk::PlatformFacebookGetFriendsState      ( void )
{
	return -1;
}

int   agk::PlatformFacebookGetFriendsCount      ( void )
{
	return 0;
}

char* agk::PlatformFacebookGetFriendsName       ( int iIndex )
{
	char *str = new char[1]; *str = 0;
	return str;
}

char* agk::PlatformFacebookGetFriendsID         ( int iIndex )
{
	char *str = new char[1]; *str = 0;
	return str;
}

void  agk::PlatformFacebookDownloadFriendsPhoto ( int iIndex )
{

}

int   agk::PlatformGetFacebookDownloadState     ( void )
{
	return -1;
}

char* agk::PlatformGetFacebookDownloadFile      ( void )
{
	char *str = new char[1]; *str = 0;
	return str;
}

int   agk::PlatformGetFacebookLoggedIn          ( void )
{
	return -1;
}

bool  agk::PlatformHasFacebook             ( void )
{
	return false;
}

char* agk::PlatformFacebookGetUserID			  ( void )
{
	char *str = new char[1]; *str = 0;
	return str;
}

char* agk::PlatformFacebookGetUserName		  ( void )
{
	char *str = new char[1]; *str = 0;
	return str;
}

char* agk::PlatformFacebookGetAccessToken		  ( void )
{
	char *str = new char[1]; *str = 0;
	return str;
}


// TWITTER COMMANDS
void  agk::PlatformTwitterSetup                 ( const char* szKey, const char* szSecret )
{
}

void  agk::PlatformTwitterMessage               ( const char* szMessage )
{
}

bool  agk::PlatformHasTwitter             ( void )
{
	return false;
}

// local notifications

void agk::PlatformCreateLocalNotification( int iID, int datetime, const char *szMessage )
{
	
}

void agk::PlatformCancelLocalNotification( int iID )
{
	
}

// V108 ULTRABOOK COMMANDS

int agk::GetNotificationType()
{
	return 0;
}

void agk::SetNotificationImage(int iImageIndex)
{
}

void agk::SetNotificationText( const char* pText)
{

}

int agk::GetNFCExists()
{
	return 0;
}

UINT agk::GetRawNFCCount()
{
	return 0;
}

UINT agk::GetRawFirstNFCDevice()
{
	return 0;
}

UINT agk::GetRawNextNFCDevice()
{
	return 0;
}

char* agk::GetRawNFCName(UINT iIndex)
{
	return 0;
}

int agk::SendRawNFCData(UINT iIndex, const char* pMessageText)
{
	return 0;
}

int agk::GetRawNFCDataState(UINT iIndex)
{
	return 0;
}

char* agk::GetRawNFCData(UINT iIndex)
{
	return 0;
}

// GPS
int agk::GetGPSSensorExists()
{
	return m_iGPSSensorExists;
}

void agk::StartGPSTracking()
{
	geolocation_request_events(0);
	geolocation_set_period(1);
}

void agk::StopGPSTracking()
//****
{
	geolocation_stop_events(0);
}

float agk::GetRawGPSLatitude()
{
	return m_fGPSLat;
}

float agk::GetRawGPSLongitude()
{
	return m_fGPSLong;
}

float agk::GetRawGPSAltitude()
{
	return m_fGPSAltitude;
}

// GameCenter

int agk::GetGameCenterExists()
{
	return 0;
}

void agk::GameCenterSetup()
{

}

void agk::GameCenterLogin()
{

}

int agk::GetGameCenterLoggedIn()
{
	return 0;
}

void agk::GameCenterSubmitScore( int iScore, const char* szBoardID )
{

}

void agk::GameCenterShowLeaderBoard ( const char* szBoardID )
{

}

void agk::GameCenterSubmitAchievement ( const char* szAchievementID, int iPercentageComplete )
{

}

void agk::GameCenterAchievementsShow ( void )
{

}

void agk::GameCenterAchievementsReset ( void )
{

}

void agk::SetSharedVariableAppGroup( const char* group )
{
	
}

void agk::SaveSharedVariable( const char *varName, const char *varValue )
{
	
}

char* agk::LoadSharedVariable( const char *varName, const char *defaultValue )
{
	if ( !defaultValue )
	{
		char *str = new char[1];
		*str = 0;
		return str;
	}
	else
	{
		char *str = new char[ strlen(defaultValue)+1 ];
		strcpy( str, defaultValue );
		return str;
	}
}

void agk::DeleteSharedVariable( const char *varName )
{
	
}

void agk::FirebaseSetup()
{
	
}

void agk::FirebaseLogEvent( const char *event_name )
{
	
}