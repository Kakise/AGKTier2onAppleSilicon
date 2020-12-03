#ifndef _H_TMEPLATE_
#define _H_TEMPLATE_

// Link to GDK libraries
#include "AGK.h"

// Global values for the app
class app
{
	public:

		// constructor
		app() { memset ( this, 0, sizeof(app)); }

		// main app functions
		void Begin( void );
		void Loop( void );
		void End( void );
};

extern app App;

#endif
