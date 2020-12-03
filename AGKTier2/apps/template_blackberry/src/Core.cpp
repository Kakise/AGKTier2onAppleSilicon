#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keycodes.h>
#include <screen/screen.h>
#include <assert.h>
#include <bps/accelerometer.h>
#include <bps/sensor.h>
#include <bps/geolocation.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <bps/bps.h>
#include <bps/event.h>
#include <bps/orientation.h>
#include <math.h>
#include <time.h>
#include <screen/screen.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <screen/screen.h>
#include <sys/platform.h>
#include <bps/mmrenderer.h>
#include <bps/dialog.h>

#include "agk.h"
#include "template.h"

#include "bbutil.h"

extern EGLDisplay egl_disp;
extern EGLSurface egl_surf;

extern EGLConfig egl_conf;
extern EGLContext egl_ctx;

extern screen_window_t screen_win;
static screen_context_t screen_ctx;

char g_szWindowGroupID[128] = "";

struct egldata
{
	EGLDisplay display;
	EGLSurface surface;
	screen_window_t screen_win;
};

static bool shutdown_app;
static int orientation_angle;
bool g_isTablet = true;

int wasPlaying = 0;
int appPaused = 0;

static int TranslateKey( int key )
{
	if ( key > 0xFF ) key &= 0xFF;

	// backspace, enter, and space
	if ( key == 8 || key == 13 || key == 32 ) return key;

	// numbers
	if ( key >= 48 && key <= 57 ) return key;

	// upper case
	if ( key >= 65 && key <= 90 ) return key;

	// lower case
	if ( key >= 97 && key <= 122 ) return key-32;

	return 0;
}

struct videoData
{
	int type;
	void* window;
	int position;
};

static void handleScreenEvent(bps_event_t *event) {
	int screen_val, buttons;
	int pair[2];

	static bool mouse_pressed = false;

	screen_event_t screen_event = screen_event_get_event(event);

	//Query type of screen event and its location on the screen
	screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);
	screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_SOURCE_POSITION, pair);

	//There is a difference between touch screen events and mouse events
	switch( screen_val )
	{
		case SCREEN_EVENT_MTOUCH_TOUCH:
		{
			int id;
			screen_get_event_property_iv( screen_event, SCREEN_PROPERTY_TOUCH_ID, &id );
			agk::TouchPressed( id, pair[0], pair[1] );
			break;
		}

		case SCREEN_EVENT_MTOUCH_MOVE:
		{
			int id;
			screen_get_event_property_iv( screen_event, SCREEN_PROPERTY_TOUCH_ID, &id );
			agk::TouchMoved( id, pair[0], pair[1] );
			break;
		}

		case SCREEN_EVENT_MTOUCH_RELEASE:
		{
			int id;
			screen_get_event_property_iv( screen_event, SCREEN_PROPERTY_TOUCH_ID, &id );
			agk::TouchReleased( id, pair[0], pair[1] );
			break;
		}

		case SCREEN_EVENT_KEYBOARD:
		{
			int event_flags;
			screen_get_event_property_iv( screen_event, SCREEN_PROPERTY_KEY_FLAGS, &event_flags );

			int key;
			screen_get_event_property_iv( screen_event, SCREEN_PROPERTY_KEY_SYM, &key );
			int agkkey = TranslateKey( key );

			if ( event_flags & KEY_DOWN )
			{
				agk::KeyDown( agkkey );
				agk::CharDown( key & 0xFF );
				agk::KeyUp( agkkey ); // no separate key up event for virtual keyboard
			}
			break;
		}

		case SCREEN_EVENT_CREATE:
		{
			screen_window_t video_window;
			screen_get_event_property_pv(screen_event, SCREEN_PROPERTY_WINDOW, (void **)&video_window);

			char id[256];
			screen_get_window_property_cv( video_window, SCREEN_PROPERTY_ID_STRING, 256, id );

			if ( strcmp( g_szWindowGroupID, id ) == 0 )
			{
				videoData *data = new videoData();
				data->type = 0;
				data->window = &video_window;

				agk::ChangeVideoPointer( data );
			}
			break;
		}
	}

	/*
	if (screen_val == SCREEN_EVENT_POINTER)
	{
		//This is a mouse move event, it is applicable to a device with a usb mouse or simulator
		screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS,
				&buttons);

		if (buttons == SCREEN_LEFT_MOUSE_BUTTON) {
			//Left mouse button is pressed
			mouse_pressed = true;
		} else {
			if (mouse_pressed) {
				//Left mouse button was released, add a cube
			//	add_cube((float) pair[0], (float) pair[1]);
				mouse_pressed = false;
			}
		}
	}
	*/
}

static void handleNavigatorEvent(bps_event_t *event) {
	static int rotateAngle = -1;

	switch (bps_event_get_code(event))
	{
		case NAVIGATOR_SWIPE_DOWN:
		{
			break;
		}

		case NAVIGATOR_EXIT:
		{
			shutdown_app = true;
			break;
		}

		/*
		// User choice, option A
		// simpler orientation handling, use the bar-descriptor.xml file to restrict orientations to portrait or landscape
		case NAVIGATOR_ORIENTATION_CHECK:
		{
			navigator_orientation_check_response(event, true);

			break;
		}

		case NAVIGATOR_ORIENTATION:
		{
			int angle = navigator_event_get_orientation_angle( event );
			bbutil_rotate_screen_surface(angle);

			egldata ptr;
			ptr.display = egl_disp;
			ptr.surface = egl_surf;
			agk::UpdatePtr( &ptr );

			navigator_done_orientation(event);
			break;
		}
		*/

		// User choice, option B
		// control app orientation with AGK SetOrientationAllowed command, set bar-descriptor.xml to auto-orient
		case NAVIGATOR_ORIENTATION_CHECK:
		{
			int angle = navigator_event_get_orientation_angle( event );
			//uString str;
			//str.Format( "Orientation Check %d", angle );
			//agk::Warning( str );

			int mode = 0;
			if ( g_isTablet )
			{
				//agk::Warning("Tablet");
				switch(angle)
				{
					case 90: mode = 1; break;
					case 270: mode = 2; break;
					case 180: mode = 3; break;
					case 0: mode = 4; break;
				}
			}
			else
			{
				//agk::Warning("Phone");
				switch(angle)
				{
					case 90: mode = 3; break;
					case 270: mode = 4; break;
					case 180: mode = 2; break;
					case 0: mode = 1; break;
				}
			}

			if ( mode > 0 )
			{
				if ( agk::CanOrientationChange(mode) )
				{
					agk::OrientationChanged(mode);
					//agk::Warning( "Orientation Check Response Yes" );
					navigator_orientation_check_response(event, true);
				}
				else
				{
					//agk::Warning( "Orientation Check Response No" );
					navigator_orientation_check_response(event, false);
				}
			}

			break;
		}

		case NAVIGATOR_ORIENTATION:
		{
			int angle = navigator_event_get_orientation_angle(event);
			//uString str;
			//str.Format( "Orientation Event %d", angle );
			//agk::Warning( str );

			int mode = 0;
			if ( g_isTablet )
			{
				switch(angle)
				{
					case 90: mode = 1; break;
					case 270: mode = 2; break;
					case 180: mode = 3; break;
					case 0: mode = 4; break;
				}
			}
			else
			{
				switch(angle)
				{
					case 90: mode = 3; break;
					case 270: mode = 4; break;
					case 180: mode = 2; break;
					case 0: mode = 1; break;
				}
			}

			if ( mode > 0 && agk::CanOrientationChange(mode) )
			{
				//str.Format( "Orientation Used %d", angle );
				//agk::Warning( str );

				bbutil_rotate_screen_surface(angle);

				egldata ptr;
				ptr.display = egl_disp;
				ptr.surface = egl_surf;
				agk::UpdatePtr( &ptr );
			}
			else
			{
				//agk::Warning( "Orientation Ignored" );
			}

			navigator_done_orientation(event);
			break;
		}

		// end choice

		case NAVIGATOR_ORIENTATION_RESULT:
		{
			//agk::Warning( "Orientation Event Completed" );
			break;
		}

		case NAVIGATOR_WINDOW_INACTIVE:
		{
			if ( agk::GetMusicPlaying() )
			{
				wasPlaying = 1;
				agk::PauseMusic();
			}

			agk::StopSound(0);

			appPaused = 1;

			/*
			//Wait for NAVIGATOR_WINDOW_ACTIVE event
			int rc;
			bps_event_t *activation_event = NULL;
			for (;;) {
				rc = bps_get_event(&activation_event, 0);
				//assert(rc == BPS_SUCCESS);

				if ( activation_event )
				{
					if ( bps_event_get_code(activation_event) == NAVIGATOR_WINDOW_ACTIVE)
					{
						if ( wasPlaying ) agk::ResumeMusic();
						agk::Resumed();
						break;
					}
					else if ( bps_event_get_code(activation_event) == NAVIGATOR_EXIT)
					{
						shutdown_app = true;
						break;
					}
				}
				else agk::Sleep(1000);
			}
			*/
			break;
		}

		case NAVIGATOR_WINDOW_ACTIVE:
		{
			appPaused = 0;
			if ( wasPlaying ) agk::ResumeMusic();
			agk::Resumed();
			break;
		}
	}
}

static void handleAccelerometer() {
	static bool first = true;
	double result_x = 0.0f, result_y = -1.0f;

	//At this point accelerometer is not supported on simulator
	if (accelerometer_is_supported()) {
		int rc;

		if (first) {
			//Set accelerometer update frequency on the first pass.
			rc = accelerometer_set_update_frequency(FREQ_40_HZ);
			assert(rc == BPS_SUCCESS);

			first = false;
		}

		double x, y, z;
		rc = accelerometer_read_forces(&x, &y, &z);
		assert(rc == BPS_SUCCESS);

		if ( g_isTablet ) agk::Accelerometer( -y, -x, z );
		else agk::Accelerometer( x, -y, z );
	}
}

static void handleMusicEvent(bps_event_t *event) {
	static mmrenderer_state_t oldstate = MMR_DESTROYED;
	mmrenderer_state_t st = mmrenderer_event_get_state( event );
	if ( st != oldstate ) // Could also be a speed change without a state change
	{
		if ( st == MMR_STOPPED)
		{
			const mmr_error_info_t *ei  = mmrenderer_event_get_error( event );
			if ( ei == NULL )
			{
				// stopped by mmr_stop()
			}
			else
			{
				const char *pos = mmrenderer_event_get_error_position( event );
				if ( ei->error_code == MMR_ERROR_NONE )
				{
					agk::HandleMusicEvents( 0 );
				}
				else
				{
					// stopped by error
				}
			}
		}
	}
	oldstate = st;
}

static void handleVideoEvent(bps_event_t *event) {
	/*
	static mmrenderer_state_t oldstate = MMR_DESTROYED;
	mmrenderer_state_t st = mmrenderer_event_get_state( event );
	const mmr_error_info_t *ei2  = mmrenderer_event_get_error( event );

	if ( st != oldstate ) // Could also be a speed change without a state change
	{
		if ( st == MMR_STOPPED)
		{
			const mmr_error_info_t *ei  = mmrenderer_event_get_error( event );
			if ( ei == NULL )
			{
				// stopped by mmr_stop()
			}
			else
			{
				//const char *pos = mmrenderer_event_get_error_position( event );
				if ( ei->error_code == MMR_ERROR_NONE )
				{
					agk::HandleVideoEvents();
				}
				else
				{
					// stopped by error
				}
			}
		}
	}
	oldstate = st;
	*/

	const char *position = mmrenderer_event_get_position(event);
	if ( position )
	{
		videoData *data = new videoData();
		data->type = 1;
		data->position = atoi(position);
		agk::ChangeVideoPointer(data);
	}

	// horrible hack for BB10 not sending the MMR_STOPPED state, detect an idle buffer near the end of the video
	const char *buffer = mmrenderer_event_get_bufferstatus( event );
	if ( buffer && position )
	{
		int pos = atoi(position);
		float duration = agk::GetVideoDuration() * 1000 - 1000;
		if ( strcmp(buffer,"idle") == 0 && pos >= duration )
		{
			agk::HandleVideoEvents();
		}
	}
}

static void handleKeyboardEvent(bps_event_t *event)
{
	int type = bps_event_get_code(event);
	switch( type )
	{
		case VIRTUALKEYBOARD_EVENT_HIDDEN:
		{
			// pretend the back button has been pressed, AGK will catch this as the keyboard closing like on Android
			//agk::KeyDown( 27 );
			//agk::KeyUp( 27 );
			break;
		}
	}
}

static void handleSensorEvent(bps_event_t *event)
{
	int type = bps_event_get_code(event);
	switch( type )
	{
		case SENSOR_GYROSCOPE_READING:
		{
			float x,y,z;
			sensor_event_get_xyz(event, &x, &y, &z);
			if ( g_isTablet ) agk::Gyro(-y,x,z);
			else agk::Gyro(x,y,z);
			break;
		}
		case SENSOR_MAGNETOMETER_READING:
		{
			float x,y,z;
			sensor_event_get_xyz(event, &x, &y, &z);
			agk::Magnetic(x,y,z);
			break;
		}
		case SENSOR_LIGHT_READING:
		{
			float light = sensor_event_get_illuminance(event);
			agk::LightSensor(light);
			break;
		}
		case SENSOR_PROXIMITY_READING:
		{
			float prox = sensor_event_get_proximity(event);
			agk::Proximity(prox);
			break;
		}
		case SENSOR_ROTATION_VECTOR_READING:
		{
			sensor_rotation_vector_t rot;
			sensor_event_get_rotation_vector(event, &rot);
			agk::RotationSensor( rot.vector[0], rot.vector[1], rot.vector[2], rot.vector[3] );
			break;
		}
	}
}

static void handleGPSEvent(bps_event_t *event)
{
	int type = bps_event_get_code(event);
	if ( type != GEOLOCATION_INFO )
	{
		return;
	}

	double latitude = geolocation_event_get_latitude(event);
	double longitude = geolocation_event_get_longitude(event);

	double altitude = 0;
	if ( geolocation_event_is_altitude_valid(event) ) altitude = geolocation_event_get_altitude(event);

	agk::GPS(longitude, latitude, altitude);
}

static void handle_events() {
	int rc;

	handleAccelerometer();

	//Request and process available BPS events
	for(;;) {
		bps_event_t *event = NULL;
		rc = bps_get_event(&event, 0);

		assert(rc == BPS_SUCCESS);

		if (event) {
			int domain = bps_event_get_domain(event);

			if ( domain == sensor_get_domain() )
			{
				handleSensorEvent(event);
			}
			else if ( domain == geolocation_get_domain() )
			{
				handleGPSEvent(event);
			}
			else if ( domain == screen_get_domain() )
			{
				handleScreenEvent(event);
			}
			else if ( domain == navigator_get_domain() )
			{
				handleNavigatorEvent(event);
			}
			else if ( domain == mmrenderer_get_domain() )
			{
				int type = mmrenderer_event_get_userdata(event);
				if ( type == 1 ) handleVideoEvent(event);
				else if ( type == 0 ) handleMusicEvent(event);
			}
			else if ( domain == virtualkeyboard_get_domain() )
			{
				handleKeyboardEvent(event);
			}
		} else {
			break;
		}
	}
}

void reporterror( const char *msg )
{
	fprintf( stderr, msg );
	fprintf( stderr, "\n" );
}

/*
class MyThread : public AGKThread
{

public:
	UINT Run()
	{
		while ( 1 )
		{
			SleepSafe( 1000 );
			agk::Warning( "Thread Here" );
		}
	}
};

MyThread myThread;
*/

int main(int argc, char **argv) {
	shutdown_app = false;

	//Create a screen context that will be used to create an EGL surface to to receive libscreen events
	int rc = screen_create_context(&screen_ctx, 0);
	if ( rc )
	{
		reporterror( "screen_create_context failed" );
		return 0;
	}

	//Initialize BPS library
	bps_initialize();

	//Determine initial orientation angle
	orientation_direction_t direction;
	orientation_get(&direction, &orientation_angle);

	srand(time(NULL));
	for ( int i=0; i < 10; i++ )
	{
		int c = (rand() % 25) + 65;
		g_szWindowGroupID[ i ] = c;
	}
	g_szWindowGroupID[ 10 ] = 0;
	agk::DialogGroup( g_szWindowGroupID );

	//Use utility code to initialize EGL for 2D rendering with GL ES 2.0
	if (EXIT_SUCCESS != bbutil_init_egl(screen_ctx, GL_ES_2, PORTRAIT)) {
		fprintf(stderr, "bbutil_init_egl failed\n");
		bbutil_terminate();
		screen_destroy_context(screen_ctx);
		return 0;
	}

	screen_display_t screen_disp;
	rc = screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_DISPLAY, (void **)&screen_disp);
	int screen_resolution[2];
	rc = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, screen_resolution);
	int angle = atoi(getenv("ORIENTATION"));

	g_isTablet = true;
	if ( screen_resolution[0] < screen_resolution[1] && (angle == 0 || angle == 180) ) g_isTablet = false;
	if ( screen_resolution[0] > screen_resolution[1] && (angle == 90 || angle == 270) ) g_isTablet = false;

	//int angle2 = 90;
	//screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_ROTATION, &angle2);

	int idle_mode = SCREEN_IDLE_MODE_KEEP_AWAKE;
	screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_IDLE_MODE, &idle_mode);

	dialog_request_events(0);

	navigator_rotation_lock( false );

	agk::SetExtraAGKPlayerAssetsMode(1);

	egldata ptr;
	ptr.display = egl_disp;
	ptr.surface = egl_surf;
	ptr.screen_win = screen_win;
	agk::InitGL( &ptr );

	//Signal BPS library that navigator and screen events will be requested
	if (BPS_SUCCESS != screen_request_events(screen_ctx)) {
		reporterror("screen_request_events failed");
		screen_destroy_context(screen_ctx);
		return 0;
	}

	if (BPS_SUCCESS != navigator_request_events(0)) {
		reporterror("navigator_request_events failed");
		screen_destroy_context(screen_ctx);
		return 0;
	}

	//Signal BPS library that navigator orientation is not to be locked
	if (BPS_SUCCESS != navigator_rotation_lock(false)) {
		reporterror("navigator_rotation_lock failed");
		screen_destroy_context(screen_ctx);
		return 0;
	}

	App.Begin();
	agk::SetOrientationAllowed( 1,1,1,1 );

	// keyboard
	virtualkeyboard_request_events(0);

	while (!shutdown_app) {
		// Handle user input and accelerometer
		handle_events();

		if ( !shutdown_app )
		{
			if ( appPaused ) agk::Sleep(1000);
			else App.Loop();

			if ( agk::GetInternalDataI(1) == 1 )
			{
				int g_userWidth = agk::GetInternalDataI(2);
				int g_userHeight = agk::GetInternalDataI(3);

				//fprintf(stderr,"Resizing to %dx%d", g_userWidth, g_userHeight);

				bbutil_resize_screen_surface(g_userWidth, g_userHeight);

				egldata ptr;
				ptr.display = egl_disp;
				ptr.surface = egl_surf;
				agk::UpdatePtr( &ptr );
			}
		}
	}

	App.End();

	//Stop requesting events from libscreen
	screen_stop_events(screen_ctx);

	//Shut down BPS library for this process
	bps_shutdown();

	bbutil_terminate();

	//Destroy libscreen context
	screen_destroy_context(screen_ctx);
	return 0;
}
