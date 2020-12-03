#include "agk.h"

using namespace AGK;

// Particle

cParticle::cParticle()
{
	m_fX = 0;
	m_fY = 0;
	m_fVX = 0;
	m_fVY = 0;
	m_fAngle = 0;
	m_fAngleDelta = 0;
	m_fScale = 1;
	m_fTime = 0;
	m_bAlive = false;
}

// Emitter

int cParticleEmitter::m_iParticlesDrawn = 0;
int cParticleEmitter::m_iQuadParticlesDrawn = 0;

cParticleEmitter::cParticleEmitter()
{
	m_fX = 0;
	m_fY = 0;
	m_fVX = 0;
	m_fVY = 10.0f;
	m_fAngle = 10.0f;
	m_fVMin = 1.0f;
	m_fVMax = 1.0f;
	m_fSize = 0;
	m_fDepth = 0.001f;
	m_iDepth = 10;
	m_fLife = 3.0f;
	m_bInterpolateColor = true;
	m_fFreq = 10;
	m_fNumStart = 0;
	m_iCurrParticle = 0;
	m_iNumParticles = 12;
	m_bVisible = true;
	m_bActive = true;
	m_fAMin = 0;
	m_fAMax = 0;
	m_bFaceDir = false;
	m_iTransparencyMode = 1;

	m_iMaxParticles = -1;
	m_iEmittedParticles = 0;
	m_bSomeAlive = false;

	m_fStartX1 = 0;
	m_fStartY1 = 0;
	m_fStartX2 = 0;
	m_fStartY2 = 0;

	m_iImageID = 0;
	m_pImage = 0;
	m_pForces = 0;
	m_pColors = 0;
	m_pSizes = 0;

	m_pParticles = new cParticle*[ m_iNumParticles ];
	for ( UINT i = 0; i < m_iNumParticles; i++ )
	{
		m_pParticles[ i ] = new cParticle();
	}

	m_iNumVertices = 0;
	m_pVertexArray = 0;
	m_pUVArray = 0;
	m_pColorArray = 0;
	m_pIndices = 0;

	m_bDepthChanged = false;
	m_bTextureChanged = false;

	m_bFixedToScreen = false;
	m_bManagedDrawing = false;
	m_pSpriteManager = UNDEF;
}

cParticleEmitter::~cParticleEmitter()
{
	if ( m_bManagedDrawing )
	{
		if ( m_pSpriteManager )
		{
			m_pSpriteManager->RemoveParticles( this );
		}

		m_bManagedDrawing = false;
	}

	if ( m_pParticles ) 
	{
		for ( UINT i = 0; i < m_iNumParticles; i++ ) if ( m_pParticles[ i ] ) delete m_pParticles[ i ];
		delete [] m_pParticles;
	}
	if ( m_pForces ) delete [] m_pForces;
	if ( m_pColors ) delete [] m_pColors;

	if ( m_pVertexArray ) delete [] m_pVertexArray;
	if ( m_pUVArray ) delete [] m_pUVArray;
	if ( m_pColorArray ) delete [] m_pColorArray;
	if ( m_pIndices ) delete [] m_pIndices;
}

void cParticleEmitter::UpdateNumParticles()
{
	UINT iNewNum = agk::Ceil( m_fFreq * m_fLife ) + 2;  //final addition is a buffer to leave some room for error.
	if ( iNewNum > m_iNumParticles )
	{
		cParticle **pNewParticles = new cParticle*[ iNewNum ];
		if ( m_iNumParticles > 0 && m_pParticles )
		{
			// copy old particles to new list
			for ( UINT i = 0; i < m_iCurrParticle; i++ )
			{
				pNewParticles[ i ] = m_pParticles[ i ];
			}

			// important to create new particles in the gap between oldest and newest particle
			// m_iCurrParticle points to oldest particle and will be used in next emission.
			UINT remaining = m_iNumParticles - m_iCurrParticle;
			for ( UINT i = m_iCurrParticle; i < iNewNum-remaining; i++ )
			{
				pNewParticles[ i ] = new cParticle();
			}

			// add oldest particles after new ones
			for ( UINT i = iNewNum-remaining; i < iNewNum; i++ )
			{
				pNewParticles[ i ] = m_pParticles[ i - (iNewNum - m_iNumParticles) ];
			}
		}
		else
		{
			for ( UINT i = 0; i < iNewNum; i++ )
			{
				pNewParticles[ i ] = new cParticle();
			}
		}

		if ( m_pParticles ) delete [] m_pParticles;
		m_pParticles = pNewParticles;
		m_iNumParticles = iNewNum;
	}
}

void cParticleEmitter::SetSpriteManager( cSpriteMgrEx *pMgr )
{
	if ( m_pSpriteManager == pMgr ) return;

	// remove from any old manager
	if ( m_bManagedDrawing && m_pSpriteManager ) 
	{
		m_pSpriteManager->RemoveParticles( this );
	}

	m_bManagedDrawing = false;
	m_pSpriteManager = pMgr;

	UpdateManager();
}

void cParticleEmitter::UpdateManager()
{
	// always use managed drawing
	if ( !m_bManagedDrawing )
	{
		// need to hand the sprites off to the sprite manager for proper ordered rendering
		if ( m_pSpriteManager )
		{
			m_pSpriteManager->AddParticles( this );
			m_bManagedDrawing = true;
		}
	}

	/*
	if ( m_fDepth > 0 )
	{
		if ( !m_bManagedDrawing )
		{
			// need to hand the sprites off to the sprite manager for proper ordered rendering
			if ( m_pSpriteManager )
			{
				m_pSpriteManager->AddParticles( this );
				m_bManagedDrawing = true;
			}
		}
	}
	else
	{
		if ( m_bManagedDrawing )
		{
			// remove sprites from sprite manager for faster (one call) rendering
			if ( m_pSpriteManager )
			{
				m_pSpriteManager->RemoveParticles( this );
			}

			m_bManagedDrawing = false;
		}
	}
	*/
}

void cParticleEmitter::FixToScreen( bool fix )
{
	m_bFixedToScreen = fix;
}

void cParticleEmitter::SetPosition( float x, float y )
{
	m_fX = x;
	m_fY = y;
}

void cParticleEmitter::SetDepth( int depth )
{
	if ( depth / 10000.0f == m_fDepth ) return;

	m_iDepth = depth;
	m_fDepth = (depth / 10000.0f);
	m_bDepthChanged = true;

	UpdateManager();
}

void cParticleEmitter::SetFrequency( float freq )
{
	//if ( freq < 0.1f ) freq = 0.1f;
	if ( freq < 0 ) freq = 0;
	if ( freq > 500.0f ) freq = 500.0f;

	m_fFreq = freq;

	// adjust number of particles based on life and frequency
	UpdateNumParticles();
}

void cParticleEmitter::SetStartZone( float x1, float y1, float x2, float y2 )
{
	if ( x1 > x2 ) 
	{
		float temp = x1;
		x1 = x2;
		x2 = temp;
	}

	if ( y1 > y2 ) 
	{
		float temp = y1;
		y1 = y2;
		y2 = temp;
	}

	m_fStartX1 = x1;
	m_fStartY1 = y1;
	m_fStartX2 = x2;
	m_fStartY2 = y2;
}

void cParticleEmitter::SetDirection( float vx, float vy )
{
	m_fVX = vx;
	m_fVY = vy;
}

void cParticleEmitter::SetVelocityRange( float v1, float v2 )
{
	if ( v1 < 0.001f ) v1 = 0.001f;
	if ( v2 < 0.001f ) v2 = 0.001f;

	if ( v2 < v1 )
	{
		float temp = v1;
		v1 = v2;
		v2 = temp;
	}

	m_fVMin = v1;
	m_fVMax = v2;
}

void cParticleEmitter::SetAngle( float angle )
{
	if ( angle < 0 ) angle = 0;
	if ( angle > 360 ) angle = 360;
	m_fAngle = angle * PI / 180.0f;
}

void cParticleEmitter::SetAngleRad( float angle )
{
	if ( angle < 0 ) angle = 0;
	if ( angle > 2*PI ) angle = 2*PI;
	m_fAngle = angle;
}

void cParticleEmitter::SetSize( float size )
{
	if ( size < 0.0f ) size = 0.0f;

	m_fSize = size;
}

void cParticleEmitter::SetRotationRate( float a1, float a2 )
{
	a1 = a1 * PI / 180.0f;
	a2 = a2 * PI / 180.0f;
	SetRotationRateRad( a1, a2 );
}

void cParticleEmitter::SetRotationRateRad( float a1, float a2 )
{
	if ( a2 < a1 )
	{
		float temp = a1;
		a1 = a2;
		a2 = temp;
	}

	m_fAMin = a1;
	m_fAMax = a2;
}

void cParticleEmitter::SetFaceDirection( int flag )
{
	m_bFaceDir = (flag != 0);
}

void cParticleEmitter::SetTransparency( int mode )
{
	m_iTransparencyMode = mode;
}

void cParticleEmitter::SetLife( float time )
{
	if ( time < 0.001f ) time = 0.001f;
	if ( time > 120.0f ) time = 120.0f;

	m_fLife = time;

	// adjust number of particles based on life and frequency
	UpdateNumParticles();
}

void cParticleEmitter::SetImage( cImage *pImage )
{
	if ( m_pImage == pImage ) return;

	m_pImage = pImage;
	m_iImageID = 0;
	if ( !pImage ) return;

	m_iImageID = pImage->GetID();
//	int width = pImage->GetWidth();
//	int height = pImage->GetHeight();

	//if ( (width & (width-1)) || (height & (height-1)) )
	//{
		//agk::Warning( "Particle images should have a power of 2 width and height, i.e. 8,16,32,64,128,etc pixels" );
	//}

	//if ( pImage->HasParent() )
	//{
	//	agk::Warning( "Particle images should not be derived from atlas textures, as they will display the entire atlas texture." );
	//}

	m_bTextureChanged = true;
}

void cParticleEmitter::SetColorInterpolation( int mode )
{
	m_bInterpolateColor = mode != 0;
}

void cParticleEmitter::SetMaxParticles( int max )
{
	m_iMaxParticles = max;
}

void cParticleEmitter::ResetParticleCount()
{
	m_iEmittedParticles = 0;
}

int cParticleEmitter::GetDepth()
{
	return m_iDepth;
}

int cParticleEmitter::GetMaxParticlesReached()
{
	if ( m_iMaxParticles < 0 ) return false;
	return (!m_bSomeAlive) && (m_iEmittedParticles >= m_iMaxParticles);
}

void cParticleEmitter::AddForce( float starttime, float endtime, float vx, float vy )
{
	if ( starttime < 0 ) starttime = 0;
	if ( endtime <= starttime ) return;

	cParticleForce *pForce = new cParticleForce();
	pForce->m_fStartTime = starttime;
	pForce->m_fEndTime = endtime;
	pForce->m_fX = vx;
	pForce->m_fY = vy;
	pForce->m_pNext = m_pForces;

	m_pForces = pForce;
}

void cParticleEmitter::ClearForces()
{
	cParticleForce *pForce;
	while( m_pForces )
	{
		pForce = m_pForces;
		m_pForces = m_pForces->m_pNext;
		delete pForce;
	}
}

void cParticleEmitter::AddColorKeyFrame( float time, UINT red, UINT green, UINT blue, UINT alpha )
{
	if ( time < 0 ) time = 0;
	if ( red > 255 ) red = 255;
	if ( green > 255 ) green = 255;
	if ( blue > 255 ) blue = 255;
	if ( alpha > 255 ) alpha = 255;
	
	cParticleColor *pColor = new cParticleColor();
	pColor->m_fTime = time;
	pColor->red = red;
	pColor->green = green;
	pColor->blue = blue;
	pColor->alpha = alpha;
	pColor->m_pNext = m_pColors;

	m_pColors = pColor;
}

void cParticleEmitter::ClearColors()
{
	cParticleColor *pColor;
	while( m_pColors )
	{
		pColor = m_pColors;
		m_pColors = m_pColors->m_pNext;
		delete pColor;
	}
}

void cParticleEmitter::AddScaleKeyFrame( float time, float scale )
{
	if ( time < 0 ) time = 0;
	if ( scale < 0 ) scale = 0;
	
	cParticleSize *pSize = new cParticleSize();
	pSize->m_fTime = time;
	pSize->m_fScale = scale;
	pSize->m_pNext = m_pSizes;

	m_pSizes = pSize;
}

void cParticleEmitter::ClearScales()
{
	cParticleSize *pSize;
	while( m_pSizes )
	{
		pSize = m_pSizes;
		m_pSizes = m_pSizes->m_pNext;
		delete pSize;
	}
}

void cParticleEmitter::Offset( float x, float y )
{
	for ( UINT i = 0; i < m_iNumParticles; i++ )
	{
		if ( !m_pParticles[ i ]->m_bAlive ) continue;

		m_pParticles[ i ]->m_fX += x;
		m_pParticles[ i ]->m_fY += y;
	}
}

void cParticleEmitter::Update( float time )
{
	if ( !m_bActive ) return;
	if ( time <= 0 ) return;

	// how many particles should start this frame, builds up over many frames in the case of slow emitters
	if ( m_iMaxParticles < 0 || m_iEmittedParticles < m_iMaxParticles )
	{
		m_fNumStart += (m_fFreq * time);
	}

	while ( m_fNumStart >= 1 )
	{
		// find the start point for this particle, could be anywhere within a box
		float x = m_fStartX1;
		float y = m_fStartY1;
		if ( m_fStartX2 > m_fStartX1 ) x = (agk::Random() / 65535.0f) * (m_fStartX2 - m_fStartX1) + m_fStartX1;
		if ( m_fStartY2 > m_fStartY1 ) y = (agk::Random() / 65535.0f) * (m_fStartY2 - m_fStartY1) + m_fStartY1;

		m_pParticles[ m_iCurrParticle ]->m_fX = m_fX + x;
		m_pParticles[ m_iCurrParticle ]->m_fY = m_fY + y;

		// find the start angle for this particle, could be anywhere +/- m_fAngle/2 radians from the emitter direction
		float vx = m_fVX;
		float vy = m_fVY;

		if ( m_fAngle > 0 )
		{
			float stretch = agk::GetStretchValue();
			float angle = m_fAngle*(agk::Random()/65535.0f - 0.5f);
			float cosA = agk::CosRad( angle );
			float sinA = agk::SinRad( angle );
			
			vx = (m_fVX*cosA - m_fVY*sinA/stretch);
			vy = (m_fVY*cosA + m_fVX*sinA*stretch);
		}

		// adjust the length of the direction vector (speed)
		if ( m_fVMin != 1 || m_fVMax != 1 )
		{
			float velocity = m_fVMin + (agk::Random() / 65535.0f) * (m_fVMax - m_fVMin);
			vx *= velocity;
			vy *= velocity;
		}

		// adjust the rotation speed
		m_pParticles[ m_iCurrParticle ]->m_fAngle = 0;
		m_pParticles[ m_iCurrParticle ]->m_fAngleDelta = 0;
		if ( m_fAMin != 0 || m_fAMax != 0 )
		{
			m_pParticles[ m_iCurrParticle ]->m_fAngleDelta = m_fAMin + (agk::Random() / 65535.0f) * (m_fAMax - m_fAMin);
		}

		if ( m_bFaceDir )
		{
			m_pParticles[ m_iCurrParticle ]->m_fAngle = agk::ATan2Rad( vy, vx );
		}

		m_pParticles[ m_iCurrParticle ]->m_fVX = vx;
		m_pParticles[ m_iCurrParticle ]->m_fVY = vy;
		m_pParticles[ m_iCurrParticle ]->m_fTime = 0;

		// look for closest color
		float first = 10000;
		cParticleColor *pFirst = 0;
		cParticleColor *pColor = m_pColors;
		while ( pColor )
		{
			if ( pColor->m_fTime < first ) 
			{
				first = pColor->m_fTime;
				pFirst = pColor;
			}
			
			pColor = pColor->m_pNext;
		}

		if ( pFirst ) m_pParticles[ m_iCurrParticle ]->SetColor( pFirst->red, pFirst->green, pFirst->blue, pFirst->alpha );
		else m_pParticles[ m_iCurrParticle ]->SetColor( 255,255,255,255 );

		// scale
		first = 10000;
		cParticleSize *pSFirst = 0;
		cParticleSize *pSize = m_pSizes;
		while ( pSize )
		{
			if ( pSize->m_fTime < first ) 
			{
				first = pSize->m_fTime;
				pSFirst = pSize;
			}
			
			pSize = pSize->m_pNext;
		}

		if ( pSFirst ) m_pParticles[ m_iCurrParticle ]->m_fScale = pSFirst->m_fScale;
		else m_pParticles[ m_iCurrParticle ]->m_fScale = 1;

		// make it active
		m_pParticles[ m_iCurrParticle ]->m_bAlive = true;

		m_iCurrParticle++;
		if ( m_iCurrParticle >= m_iNumParticles ) m_iCurrParticle = 0;
		m_fNumStart--;

		m_iEmittedParticles++;
	}

	m_bSomeAlive = false;

	// update live particles
	for ( UINT i = 0; i < m_iNumParticles; i++ )
	{
		if ( !m_pParticles[ i ]->m_bAlive ) continue;

		m_bSomeAlive = true;

		m_pParticles[ i ]->m_fTime += time;
		if ( m_pParticles[ i ]->m_fTime > m_fLife ) m_pParticles[ i ]->m_bAlive = false;

		// check forces
		cParticleForce *pForce = m_pForces;
		while ( pForce )
		{
			if ( pForce->m_fStartTime <= m_pParticles[ i ]->m_fTime 
			  && pForce->m_fEndTime > m_pParticles[ i ]->m_fTime )
			{
				m_pParticles[ i ]->m_fVX += pForce->m_fX*time;
				m_pParticles[ i ]->m_fVY += pForce->m_fY*time;
			}

			pForce = pForce->m_pNext;
		}

		// modify angle
		if ( m_bFaceDir )
		{
			m_pParticles[ i ]->m_fAngle = agk::ATan2Rad( m_pParticles[ i ]->m_fVY, m_pParticles[ i ]->m_fVX ) + PI/2;
		}
		else
		{
			m_pParticles[ i ]->m_fAngle += m_pParticles[ i ]->m_fAngleDelta*time;
		}

		// check colors
		cParticleColor *pBegin = 0;
		cParticleColor *pEnd = 0;
		float min = 10000;
		float max = -10000;
		cParticleColor *pColor = m_pColors;
		while ( pColor )
		{
			if ( pColor->m_fTime <= m_pParticles[ i ]->m_fTime && pColor->m_fTime > max )
			{
				max = pColor->m_fTime;
				pBegin = pColor;
			}

			if ( pColor->m_fTime > m_pParticles[ i ]->m_fTime && pColor->m_fTime < min )
			{
				min = pColor->m_fTime;
				pEnd = pColor;
			}

			pColor = pColor->m_pNext;
		}

		if ( !m_bInterpolateColor )
		{
			if ( pBegin )
			{
				m_pParticles[ i ]->SetColor( pBegin->red, pBegin->green, pBegin->blue, pBegin->alpha );
			}
		}
		else
		{
			if ( pBegin && pEnd )
			{
				float curr = m_pParticles[ i ]->m_fTime - pBegin->m_fTime;
				float total = pEnd->m_fTime - pBegin->m_fTime;
				if ( total > 0 )
				{
					float s = curr / total;
					UINT red = agk::Floor( pBegin->red*(1-s) + pEnd->red*s );
					UINT green = agk::Floor( pBegin->green*(1-s) + pEnd->green*s );
					UINT blue = agk::Floor( pBegin->blue*(1-s) + pEnd->blue*s );
					UINT alpha = agk::Floor( pBegin->alpha*(1-s) + pEnd->alpha*s );

					m_pParticles[ i ]->SetColor( red, green, blue, alpha );
				}
			}
		}

		// check size
		cParticleSize *pSBegin = 0;
		cParticleSize *pSEnd = 0;
		min = 10000;
		max = -10000;
		cParticleSize *pSize = m_pSizes;
		while ( pSize )
		{
			if ( pSize->m_fTime <= m_pParticles[ i ]->m_fTime && pSize->m_fTime > max )
			{
				max = pSize->m_fTime;
				pSBegin = pSize;
			}

			if ( pSize->m_fTime > m_pParticles[ i ]->m_fTime && pSize->m_fTime < min )
			{
				min = pSize->m_fTime;
				pSEnd = pSize;
			}

			pSize = pSize->m_pNext;
		}

		if ( pSBegin && pSEnd )
		{
			float curr = m_pParticles[ i ]->m_fTime - pSBegin->m_fTime;
			float total = pSEnd->m_fTime - pSBegin->m_fTime;
			if ( total > 0 )
			{
				float s = curr / total;
				float scale = pSBegin->m_fScale + (pSEnd->m_fScale - pSBegin->m_fScale)*s;
				m_pParticles[ i ]->m_fScale = scale;
			}
		}

		m_pParticles[ i ]->m_fX += m_pParticles[ i ]->m_fVX*time;
		m_pParticles[ i ]->m_fY += m_pParticles[ i ]->m_fVY*time;
	}
}

void cParticleEmitter::DrawAll()
{
	if ( !m_bVisible ) return;
	if ( !m_bSomeAlive ) return;

	if ( m_iImageID )
	{
		// check image
		cImage *pRealPtr = agk::GetImagePtr( m_iImageID );
		if ( pRealPtr != m_pImage )
		{
			SetImage( 0 );
		}
	}

	// quad setup
	if ( m_iNumParticles > m_iNumVertices )
	{
		if ( m_pVertexArray ) delete [] m_pVertexArray;
		m_pVertexArray = new float[ m_iNumParticles*4*3 ];

		if ( m_pUVArray ) delete [] m_pUVArray;
		m_pUVArray = new float[ m_iNumParticles*4*2 ];

		if ( m_pColorArray ) delete [] m_pColorArray;
		m_pColorArray = new UINT[ m_iNumParticles*4 ];

		if ( m_pIndices ) delete [] m_pIndices;
		m_pIndices = new unsigned short[ m_iNumParticles*6 ];

		m_iNumVertices = m_iNumParticles;
	}

	float fNewSize = m_fSize / 2.0f;
	if ( !m_bFixedToScreen ) fNewSize *= agk::GetViewZoom();

	UINT count = 0;
	// start with the oldest particle
	if ( m_iCurrParticle > 0 )
	{
		for ( int i = (int)m_iCurrParticle-1; i >= 0; i-- )
		{
			if ( !m_pParticles[ i ]->m_bAlive ) continue;

			float x = m_pParticles[ i ]->m_fX;
			float y = m_pParticles[ i ]->m_fY;
			float size = m_pParticles[ i ]->m_fScale*fNewSize;

			if ( !m_bFixedToScreen )
			{
				x = agk::WorldToScreenX( x );
				y = agk::WorldToScreenY( y );
			}

			if ( x < agk::GetScreenBoundsLeft() - size*1.42 
			  || y < agk::GetScreenBoundsTop() - size*1.42*agk::m_fStretchValue
			  || x > agk::GetScreenBoundsRight() + size*1.42 
			  || y > agk::GetScreenBoundsBottom() + size*1.42*agk::m_fStretchValue ) continue; // 1.42 - rough distince from center to the corner of a 1x1 square

			// vertices
			float x1,x2,x3,x4;
			float y1,y2,y3,y4;

			if ( m_pParticles[ i ]->m_fAngle != 0 )
			{
				float stretch = agk::m_fStretchValue;
				float fSinA = agk::SinRad(m_pParticles[ i ]->m_fAngle);
				float fCosA = agk::CosRad(m_pParticles[ i ]->m_fAngle);
				float fSinA1 = fSinA/stretch;
				float fSinA2 = fSinA*stretch;
				
				float fTempX = -size;
				float fTempY = -size*stretch;
				x1 = x + (fTempX*fCosA - fTempY*fSinA1);
				y1 = y + (fTempY*fCosA + fTempX*fSinA2);
				
				fTempX = -size;
				fTempY = size*stretch;
				x2 = x + (fTempX*fCosA - fTempY*fSinA1);
				y2 = y + (fTempY*fCosA + fTempX*fSinA2);
				
				fTempX = size;
				fTempY = -size*stretch;
				x3 = x + (fTempX*fCosA - fTempY*fSinA1);
				y3 = y + (fTempY*fCosA + fTempX*fSinA2);
				
				fTempX = size;
				fTempY = size*stretch;
				x4 = x + (fTempX*fCosA - fTempY*fSinA1);
				y4 = y + (fTempY*fCosA + fTempX*fSinA2);
			}
			else
			{
				x1 = x - size;
				y1 = y - size*agk::m_fStretchValue;
				
				x2 = x - size;
				y2 = y + size*agk::m_fStretchValue;
				
				x3 = x + size;
				y3 = y - size*agk::m_fStretchValue;
				
				x4 = x + size;
				y4 = y + size*agk::m_fStretchValue;
			}
			
			m_pVertexArray[ (count*4 + 0)*3 + 0 ] = x1;
			m_pVertexArray[ (count*4 + 0)*3 + 1 ] = y1;
			m_pVertexArray[ (count*4 + 0)*3 + 2 ] = m_fDepth;

			m_pVertexArray[ (count*4 + 1)*3 + 0 ] = x2;
			m_pVertexArray[ (count*4 + 1)*3 + 1 ] = y2;
			m_pVertexArray[ (count*4 + 1)*3 + 2 ] = m_fDepth;

			m_pVertexArray[ (count*4 + 2)*3 + 0 ] = x3;
			m_pVertexArray[ (count*4 + 2)*3 + 1 ] = y3;
			m_pVertexArray[ (count*4 + 2)*3 + 2 ] = m_fDepth;

			m_pVertexArray[ (count*4 + 3)*3 + 0 ] = x4;
			m_pVertexArray[ (count*4 + 3)*3 + 1 ] = y4;
			m_pVertexArray[ (count*4 + 3)*3 + 2 ] = m_fDepth;

			// uv
			if ( m_pImage ) 
			{
				m_pUVArray[ (count*4 + 0)*2 + 0 ] = m_pImage->GetU1();
				m_pUVArray[ (count*4 + 0)*2 + 1 ] = m_pImage->GetV1();

				m_pUVArray[ (count*4 + 1)*2 + 0 ] = m_pImage->GetU1();
				m_pUVArray[ (count*4 + 1)*2 + 1 ] = m_pImage->GetV2();

				m_pUVArray[ (count*4 + 2)*2 + 0 ] = m_pImage->GetU2();
				m_pUVArray[ (count*4 + 2)*2 + 1 ] = m_pImage->GetV1();

				m_pUVArray[ (count*4 + 3)*2 + 0 ] = m_pImage->GetU2();
				m_pUVArray[ (count*4 + 3)*2 + 1 ] = m_pImage->GetV2();
			}
			else
			{
				m_pUVArray[ (count*4 + 0)*2 + 0 ] = 0.0f;
				m_pUVArray[ (count*4 + 0)*2 + 1 ] = 0.0f;

				m_pUVArray[ (count*4 + 1)*2 + 0 ] = 0.0f;
				m_pUVArray[ (count*4 + 1)*2 + 1 ] = 1.0f;

				m_pUVArray[ (count*4 + 2)*2 + 0 ] = 1.0f;
				m_pUVArray[ (count*4 + 2)*2 + 1 ] = 0.0f;

				m_pUVArray[ (count*4 + 3)*2 + 0 ] = 1.0f;
				m_pUVArray[ (count*4 + 3)*2 + 1 ] = 1.0f;
			}

			// colors
			m_pColorArray[ count*4 + 0 ] = m_pParticles[ i ]->GetColor();
			m_pColorArray[ count*4 + 1 ] = m_pParticles[ i ]->GetColor();
			m_pColorArray[ count*4 + 2 ] = m_pParticles[ i ]->GetColor();
			m_pColorArray[ count*4 + 3 ] = m_pParticles[ i ]->GetColor();
			
			count++;
		}
	}

	// wrap around the array back to oldest particle
	for ( int i = (int)m_iNumParticles-1; i >= (int)m_iCurrParticle ; i-- )
	{
		if ( !m_pParticles[ i ]->m_bAlive ) continue;

		float x = m_pParticles[ i ]->m_fX;
		float y = m_pParticles[ i ]->m_fY;
		float size = m_pParticles[ i ]->m_fScale*fNewSize;

		if ( !m_bFixedToScreen )
		{
			x = agk::WorldToScreenX( x );
			y = agk::WorldToScreenY( y );
		}

		if ( x < agk::GetScreenBoundsLeft() - size*1.42 
		  || y < agk::GetScreenBoundsTop() - size*1.42*agk::m_fStretchValue 
		  || x > agk::GetScreenBoundsRight() + size*1.42 
		  || y > agk::GetScreenBoundsBottom() + size*1.42*agk::m_fStretchValue ) continue;

		// vertices
		float x1,x2,x3,x4;
		float y1,y2,y3,y4;

		if ( m_pParticles[ i ]->m_fAngle != 0 )
		{
			float stretch = agk::m_fStretchValue;
			float fSinA = agk::SinRad(m_pParticles[ i ]->m_fAngle);
			float fCosA = agk::CosRad(m_pParticles[ i ]->m_fAngle);
			float fSinA1 = fSinA/stretch;
			float fSinA2 = fSinA*stretch;
			
			float fTempX = -size;
			float fTempY = -size*stretch;
			x1 = x + (fTempX*fCosA - fTempY*fSinA1);
			y1 = y + (fTempY*fCosA + fTempX*fSinA2);
			
			fTempX = -size;
			fTempY = size*stretch;
			x2 = x + (fTempX*fCosA - fTempY*fSinA1);
			y2 = y + (fTempY*fCosA + fTempX*fSinA2);
			
			fTempX = size;
			fTempY = -size*stretch;
			x3 = x + (fTempX*fCosA - fTempY*fSinA1);
			y3 = y + (fTempY*fCosA + fTempX*fSinA2);
			
			fTempX = size;
			fTempY = size*stretch;
			x4 = x + (fTempX*fCosA - fTempY*fSinA1);
			y4 = y + (fTempY*fCosA + fTempX*fSinA2);
		}
		else
		{
			x1 = x - size;
			y1 = y - size*agk::m_fStretchValue;
			
			x2 = x - size;
			y2 = y + size*agk::m_fStretchValue;
			
			x3 = x + size;
			y3 = y - size*agk::m_fStretchValue;
			
			x4 = x + size;
			y4 = y + size*agk::m_fStretchValue;
		}
		
		m_pVertexArray[ (count*4 + 0)*3 + 0 ] = x1;
		m_pVertexArray[ (count*4 + 0)*3 + 1 ] = y1;
		m_pVertexArray[ (count*4 + 0)*3 + 2 ] = m_fDepth;

		m_pVertexArray[ (count*4 + 1)*3 + 0 ] = x2;
		m_pVertexArray[ (count*4 + 1)*3 + 1 ] = y2;
		m_pVertexArray[ (count*4 + 1)*3 + 2 ] = m_fDepth;

		m_pVertexArray[ (count*4 + 2)*3 + 0 ] = x3;
		m_pVertexArray[ (count*4 + 2)*3 + 1 ] = y3;
		m_pVertexArray[ (count*4 + 2)*3 + 2 ] = m_fDepth;

		m_pVertexArray[ (count*4 + 3)*3 + 0 ] = x4;
		m_pVertexArray[ (count*4 + 3)*3 + 1 ] = y4;
		m_pVertexArray[ (count*4 + 3)*3 + 2 ] = m_fDepth;

		// uv
		if ( m_pImage ) 
		{
			m_pUVArray[ (count*4 + 0)*2 + 0 ] = m_pImage->GetU1();
			m_pUVArray[ (count*4 + 0)*2 + 1 ] = m_pImage->GetV1();

			m_pUVArray[ (count*4 + 1)*2 + 0 ] = m_pImage->GetU1();
			m_pUVArray[ (count*4 + 1)*2 + 1 ] = m_pImage->GetV2();

			m_pUVArray[ (count*4 + 2)*2 + 0 ] = m_pImage->GetU2();
			m_pUVArray[ (count*4 + 2)*2 + 1 ] = m_pImage->GetV1();

			m_pUVArray[ (count*4 + 3)*2 + 0 ] = m_pImage->GetU2();
			m_pUVArray[ (count*4 + 3)*2 + 1 ] = m_pImage->GetV2();
		}
		else
		{
			m_pUVArray[ (count*4 + 0)*2 + 0 ] = 0.0f;
			m_pUVArray[ (count*4 + 0)*2 + 1 ] = 0.0f;

			m_pUVArray[ (count*4 + 1)*2 + 0 ] = 0.0f;
			m_pUVArray[ (count*4 + 1)*2 + 1 ] = 1.0f;

			m_pUVArray[ (count*4 + 2)*2 + 0 ] = 1.0f;
			m_pUVArray[ (count*4 + 2)*2 + 1 ] = 0.0f;

			m_pUVArray[ (count*4 + 3)*2 + 0 ] = 1.0f;
			m_pUVArray[ (count*4 + 3)*2 + 1 ] = 1.0f;
		}

		// colors
		m_pColorArray[ count*4 + 0 ] = m_pParticles[ i ]->GetColor();
		m_pColorArray[ count*4 + 1 ] = m_pParticles[ i ]->GetColor();
		m_pColorArray[ count*4 + 2 ] = m_pParticles[ i ]->GetColor();
		m_pColorArray[ count*4 + 3 ] = m_pParticles[ i ]->GetColor();

		count++;
	}

	if ( count == 0 ) return;

	for( UINT i = 0; i < count; i++ )
	{
		m_pIndices[ i*6 + 0 ] = 0 + i*4;
		m_pIndices[ i*6 + 1 ] = 1 + i*4;
		m_pIndices[ i*6 + 2 ] = 2 + i*4;

		m_pIndices[ i*6 + 3 ] = 2 + i*4;
		m_pIndices[ i*6 + 4 ] = 1 + i*4;
		m_pIndices[ i*6 + 5 ] = 3 + i*4;
	}
	
	m_iQuadParticlesDrawn += count;

	PlatformDrawQuadParticles( count, m_pIndices, m_pVertexArray, m_pUVArray, (unsigned char*) m_pColorArray );
}

float cParticleEmitter::GetMaxPointSize()
{
	return 0; // no longer supported
}

void cParticleEmitter::PlatformDrawQuadParticles( UINT count, unsigned short *pIndices, float *pVertices, float *pUV, unsigned char *pColors )
{
	// set drawing parameters for quad rendering
	agk::PlatformSetBlendMode( m_iTransparencyMode );
	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );
	agk::PlatformSetCullMode( 0 );
	agk::PlatformSetDepthRange( 0, 1 );
	agk::PlatformSetDepthTest( 0 );

	agk::ResetScissor();

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