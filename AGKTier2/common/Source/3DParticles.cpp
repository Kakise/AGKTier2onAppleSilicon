#include "agk.h"

using namespace AGK;

// 3DParticle

AGK3DParticle::AGK3DParticle()
{
	m_fX = 0;
	m_fY = 0;
	m_fZ = 0;
	m_fVX = 0;
	m_fVY = 0;
	m_fVZ = 0;
	//m_fAngle = 0;
	//m_fAngleDelta = 0;
	m_fScale = 1;
	m_fTime = 0;
	m_bAlive = false;
}

// Emitter

int AGK3DParticleEmitter::m_iParticlesDrawn = 0;

AGK3DParticleEmitter::AGK3DParticleEmitter()
{
	m_fX = 0;
	m_fY = 0;
	m_fZ = 0;
	m_fVX = 0;
	m_fVY = 10.0f;
	m_fVZ = 0;
	m_fRoll = 0;
	m_fAngle1 = 0.0f;
	m_fAngle2 = 0.0f;
	m_fVMin = 1.0f;
	m_fVMax = 1.0f;
	m_fSize = 0;
	m_fLife = 3.0f;
	m_fFreq = 10;
	m_fNumStart = 0;
	m_iCurrParticle = 0;
	m_iNumParticles = 12;
	m_bFlags = AGK_3DPARTICLE_ACTIVE | AGK_3DPARTICLE_VISIBLE | AGK_3DPARTICLE_INTERCOLOR;
	m_fAMin = 0;
	m_fAMax = 0;
	m_iTransparencyMode = 1;

	m_iMaxParticles = -1;
	m_iEmittedParticles = 0;
	m_bSomeAlive = false;

	m_fStartX1 = 0;
	m_fStartY1 = 0;
	m_fStartZ1 = 0;
	m_fStartX2 = 0;
	m_fStartY2 = 0;
	m_fStartZ2 = 0;

	m_iImageID = 0;
	m_pImage = 0;
	m_pForces = 0;
	m_pColors = 0;
	m_pSizes = 0;

	m_pParticles = new AGK3DParticle*[ m_iNumParticles ];
	for ( UINT i = 0; i < m_iNumParticles; i++ )
	{
		m_pParticles[ i ] = new AGK3DParticle();
	}

	m_iNumVertices = 0;
	m_pVertexArray = 0;
	m_pUVArray = 0;
	m_pColorArray = 0;
	m_pIndices = 0;
}

AGK3DParticleEmitter::~AGK3DParticleEmitter()
{
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

void AGK3DParticleEmitter::UpdateNumParticles()
{
	UINT iNewNum = agk::Ceil( m_fFreq * m_fLife ) + 2;  //final addition is a buffer to leave some room for error.
	if ( iNewNum > m_iNumParticles )
	{
		AGK3DParticle **pNewParticles = new AGK3DParticle*[ iNewNum ];
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
				pNewParticles[ i ] = new AGK3DParticle();
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
				pNewParticles[ i ] = new AGK3DParticle();
			}
		}

		if ( m_pParticles ) delete [] m_pParticles;
		m_pParticles = pNewParticles;
		m_iNumParticles = iNewNum;
	}
}

void AGK3DParticleEmitter::SetPosition( float x, float y, float z )
{
	m_fX = x;
	m_fY = y;
	m_fZ = z;
}

void AGK3DParticleEmitter::SetFrequency( float freq )
{
	//if ( freq < 0.1f ) freq = 0.1f;
	if ( freq < 0 ) freq = 0;
	if ( freq > 500.0f ) freq = 500.0f;

	m_fFreq = freq;

	// adjust number of particles based on life and frequency
	UpdateNumParticles();
}

void AGK3DParticleEmitter::SetStartZone( float x1, float y1, float z1, float x2, float y2, float z2 )
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

	if ( z1 > z2 ) 
	{
		float temp = z1;
		z1 = z2;
		z2 = temp;
	}

	m_fStartX1 = x1;
	m_fStartY1 = y1;
	m_fStartZ1 = z1;
	m_fStartX2 = x2;
	m_fStartY2 = y2;
	m_fStartZ2 = z2;
}

void AGK3DParticleEmitter::SetDirection( float vx, float vy, float vz, float roll )
{
	m_fVX = vx;
	m_fVY = vy;
	m_fVZ = vz;
	m_fRoll = roll;
}

void AGK3DParticleEmitter::SetVelocityRange( float v1, float v2 )
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

void AGK3DParticleEmitter::SetDirectionRange( float angle1, float angle2 )
{
	if ( angle1 < 0 ) angle1 = 0;
	if ( angle1 > 360 ) angle1 = 360;
	m_fAngle1 = angle1;

	if ( angle2 < 0 ) angle2 = 0;
	if ( angle2 > 180 ) angle2 = 180;
	m_fAngle2 = angle2;
}

void AGK3DParticleEmitter::SetSize( float size )
{
	if ( size < 0.0f ) size = 0.0f;
	m_fSize = size;
}

void AGK3DParticleEmitter::SetVisible( int visible )
{
	if ( visible ) m_bFlags |= AGK_3DPARTICLE_VISIBLE;
	else m_bFlags &= ~AGK_3DPARTICLE_VISIBLE;
}

void AGK3DParticleEmitter::SetActive( int active )
{
	if ( active ) m_bFlags |= AGK_3DPARTICLE_ACTIVE;
	else m_bFlags &= ~AGK_3DPARTICLE_ACTIVE;
}
/*
void AGK3DParticleEmitter::SetRotationRate( float a1, float a2 )
{
	a1 = a1 * PI / 180.0f;
	a2 = a2 * PI / 180.0f;
	SetRotationRateRad( a1, a2 );
}

void AGK3DParticleEmitter::SetRotationRateRad( float a1, float a2 )
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

void AGK3DParticleEmitter::SetFaceDirection( int flag )
{
	if ( flag ) m_bFlags |= AGK_3DPARTICLE_FACEDIR;
	else m_bFlags &= ~AGK_3DPARTICLE_FACEDIR;
}
*/
void AGK3DParticleEmitter::SetTransparency( int mode )
{
	m_iTransparencyMode = mode;
}

void AGK3DParticleEmitter::SetLife( float time )
{
	if ( time < 0.001f ) time = 0.001f;
	if ( time > 120.0f ) time = 120.0f;

	m_fLife = time;

	// adjust number of particles based on life and frequency
	UpdateNumParticles();
}

void AGK3DParticleEmitter::SetImage( cImage *pImage )
{
	if ( m_pImage == pImage ) return;

	m_pImage = pImage;
	m_iImageID = 0;
	if ( !pImage ) return;

	m_iImageID = pImage->GetID();
}

void AGK3DParticleEmitter::SetColorInterpolation( int mode )
{
	if ( mode ) m_bFlags |= AGK_3DPARTICLE_INTERCOLOR;
	else m_bFlags &= ~AGK_3DPARTICLE_INTERCOLOR;
}

void AGK3DParticleEmitter::SetMaxParticles( int max )
{
	m_iMaxParticles = max;
}

void AGK3DParticleEmitter::ResetParticleCount()
{
	m_iEmittedParticles = 0;
}

int AGK3DParticleEmitter::GetMaxParticlesReached()
{
	if ( m_iMaxParticles < 0 ) return false;
	return (!m_bSomeAlive) && (m_iEmittedParticles >= m_iMaxParticles);
}

void AGK3DParticleEmitter::AddForce( float starttime, float endtime, float vx, float vy, float vz )
{
	if ( starttime < 0 ) starttime = 0;
	if ( endtime <= starttime ) return;

	AGK3DParticleForce *pForce = new AGK3DParticleForce();
	pForce->m_fStartTime = starttime;
	pForce->m_fEndTime = endtime;
	pForce->m_fX = vx;
	pForce->m_fY = vy;
	pForce->m_fZ = vz;
	pForce->m_pNext = m_pForces;

	m_pForces = pForce;
}

void AGK3DParticleEmitter::ClearForces()
{
	AGK3DParticleForce *pForce;
	while( m_pForces )
	{
		pForce = m_pForces;
		m_pForces = m_pForces->m_pNext;
		delete pForce;
	}
}

void AGK3DParticleEmitter::AddColorKeyFrame( float time, UINT red, UINT green, UINT blue, UINT alpha )
{
	if ( time < 0 ) time = 0;
	if ( red > 255 ) red = 255;
	if ( green > 255 ) green = 255;
	if ( blue > 255 ) blue = 255;
	if ( alpha > 255 ) alpha = 255;
	
	AGK3DParticleColor *pColor = new AGK3DParticleColor();
	pColor->m_fTime = time;
	pColor->red = red;
	pColor->green = green;
	pColor->blue = blue;
	pColor->alpha = alpha;
	pColor->m_pNext = m_pColors;

	m_pColors = pColor;
}

void AGK3DParticleEmitter::ClearColors()
{
	AGK3DParticleColor *pColor;
	while( m_pColors )
	{
		pColor = m_pColors;
		m_pColors = m_pColors->m_pNext;
		delete pColor;
	}
}

void AGK3DParticleEmitter::AddScaleKeyFrame( float time, float scale )
{
	if ( time < 0 ) time = 0;
	if ( scale < 0 ) scale = 0;
	
	AGK3DParticleSize *pSize = new AGK3DParticleSize();
	pSize->m_fTime = time;
	pSize->m_fScale = scale;
	pSize->m_pNext = m_pSizes;

	m_pSizes = pSize;
}

void AGK3DParticleEmitter::ClearScales()
{
	AGK3DParticleSize *pSize;
	while( m_pSizes )
	{
		pSize = m_pSizes;
		m_pSizes = m_pSizes->m_pNext;
		delete pSize;
	}
}

void AGK3DParticleEmitter::Offset( float x, float y, float z )
{
	for ( UINT i = 0; i < m_iNumParticles; i++ )
	{
		if ( !m_pParticles[ i ]->m_bAlive ) continue;

		m_pParticles[ i ]->m_fX += x;
		m_pParticles[ i ]->m_fY += y;
		m_pParticles[ i ]->m_fZ += z;
	}
}

void AGK3DParticleEmitter::Update( float time )
{
	if ( !(m_bFlags & AGK_3DPARTICLE_ACTIVE) ) return;
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
		float z = m_fStartZ1;
		if ( m_fStartX2 > m_fStartX1 ) x = (agk::Random() / 65535.0f) * (m_fStartX2 - m_fStartX1) + m_fStartX1;
		if ( m_fStartY2 > m_fStartY1 ) y = (agk::Random() / 65535.0f) * (m_fStartY2 - m_fStartY1) + m_fStartY1;
		if ( m_fStartZ2 > m_fStartZ1 ) z = (agk::Random() / 65535.0f) * (m_fStartZ2 - m_fStartZ1) + m_fStartZ1;

		m_pParticles[ m_iCurrParticle ]->m_fX = m_fX + x;
		m_pParticles[ m_iCurrParticle ]->m_fY = m_fY + y;
		m_pParticles[ m_iCurrParticle ]->m_fZ = m_fZ + z;

		float vx = m_fVX;
		float vy = m_fVY;
		float vz = m_fVZ;

		// find the start angle for this particle, could be anywhere +/- m_fAngle/2 radians from the emitter direction
		if ( m_fAngle1 > 0 || m_fAngle2 > 0 )
		{
			AGKQuaternion dir; dir.LookAt( m_fVX, m_fVY, m_fVZ, m_fRoll );
		
			float angleY = m_fAngle1*(agk::Random()/65535.0f - 0.5f);
			float angleX = m_fAngle2*(agk::Random()/65535.0f - 0.5f);
			float newX = agk::Sin( angleY )*agk::Cos( angleX );
			float newZ = agk::Cos( angleY )*agk::Cos( angleX );
			float newY = agk::Sin( angleX );

			float length = agk::Sqrt(vx*vx + vy*vy + vz*vz);
			AGKVector v( newX, newY, newZ );
			v.Mult( length );
			v.Mult( dir );

			vx = v.x;
			vy = v.y;
			vz = v.z;
		}

		// adjust the length of the direction vector (speed)
		if ( m_fVMin != 1 || m_fVMax != 1 )
		{
			float velocity = m_fVMin + (agk::Random() / 65535.0f) * (m_fVMax - m_fVMin);
			vx *= velocity;
			vy *= velocity;
			vz *= velocity;
		}

		// adjust the rotation speed
		/*
		m_pParticles[ m_iCurrParticle ]->m_fAngle = 0;
		m_pParticles[ m_iCurrParticle ]->m_fAngleDelta = 0;
		if ( m_fAMin != 0 || m_fAMax != 0 )
		{
			m_pParticles[ m_iCurrParticle ]->m_fAngleDelta = m_fAMin + (agk::Random() / 65535.0f) * (m_fAMax - m_fAMin);
		}

		if ( m_bFlags & AGK_3DPARTICLE_FACEDIR )
		{
			//m_pParticles[ m_iCurrParticle ]->m_fAngle = agk::ATan2Rad( vy, vx );
		}
		*/

		m_pParticles[ m_iCurrParticle ]->m_fVX = vx;
		m_pParticles[ m_iCurrParticle ]->m_fVY = vy;
		m_pParticles[ m_iCurrParticle ]->m_fVZ = vz;
		m_pParticles[ m_iCurrParticle ]->m_fTime = 0;

		// look for closest color
		float first = 10000;
		AGK3DParticleColor *pFirst = 0;
		AGK3DParticleColor *pColor = m_pColors;
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
		AGK3DParticleSize *pSFirst = 0;
		AGK3DParticleSize *pSize = m_pSizes;
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
		AGK3DParticleForce *pForce = m_pForces;
		while ( pForce )
		{
			if ( pForce->m_fStartTime <= m_pParticles[ i ]->m_fTime 
			  && pForce->m_fEndTime > m_pParticles[ i ]->m_fTime )
			{
				m_pParticles[ i ]->m_fVX += pForce->m_fX*time;
				m_pParticles[ i ]->m_fVY += pForce->m_fY*time;
				m_pParticles[ i ]->m_fVZ += pForce->m_fZ*time;
			}

			pForce = pForce->m_pNext;
		}

		/*
		// modify angle
		if ( m_bFlags & AGK_3DPARTICLE_FACEDIR )
		{
			//m_pParticles[ i ]->m_fAngle = agk::ATan2Rad( m_pParticles[ i ]->m_fVY, m_pParticles[ i ]->m_fVX ) + PI/2;
		}
		//else
		{
			m_pParticles[ i ]->m_fAngle += m_pParticles[ i ]->m_fAngleDelta*time;
		}
		*/

		// check colors
		AGK3DParticleColor *pBegin = 0;
		AGK3DParticleColor *pEnd = 0;
		float min = 10000;
		float max = -10000;
		AGK3DParticleColor *pColor = m_pColors;
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

		if ( !(m_bFlags & AGK_3DPARTICLE_INTERCOLOR) )
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
		AGK3DParticleSize *pSBegin = 0;
		AGK3DParticleSize *pSEnd = 0;
		min = 10000;
		max = -10000;
		AGK3DParticleSize *pSize = m_pSizes;
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
		m_pParticles[ i ]->m_fZ += m_pParticles[ i ]->m_fVZ*time;
	}
}

void AGK3DParticleEmitter::DrawAll()
{
	
	if ( !(m_bFlags & AGK_3DPARTICLE_VISIBLE) ) return;
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

		for( UINT i = 0; i < m_iNumParticles; i++ )
		{
			m_pIndices[ i*6 + 0 ] = 0 + i*4;
			m_pIndices[ i*6 + 1 ] = 1 + i*4;
			m_pIndices[ i*6 + 2 ] = 2 + i*4;

			m_pIndices[ i*6 + 3 ] = 2 + i*4;
			m_pIndices[ i*6 + 4 ] = 1 + i*4;
			m_pIndices[ i*6 + 5 ] = 3 + i*4;
		}

		m_iNumVertices = m_iNumParticles;
	}

	float fNewSize = m_fSize / 2.0f;

	AGKVector camHorz(1, 0, 0);
	AGKVector camVert(0, 1, 0);
	const AGKQuaternion camRot = agk::GetCurrentCamera()->rotFinal();
	camHorz.Mult( camRot );
	camVert.Mult( camRot );

	AGKVector frustumN[6];
	float frustumD[6];
	for( int i = 0; i < 6; i++ )
	{
		agk::GetCurrentCamera()->GetFrustumPlane( i, frustumN[i], frustumD[i] );
	}

	UINT count = 0;
	// start with the oldest particle
	if ( m_iCurrParticle > 0 )
	{
		for( int i = (int)m_iCurrParticle-1; i >= 0; i-- )
		{
			if ( !m_pParticles[ i ]->m_bAlive ) continue;

			float x = m_pParticles[ i ]->m_fX;
			float y = m_pParticles[ i ]->m_fY;
			float z = m_pParticles[ i ]->m_fZ;
			float size = m_pParticles[ i ]->m_fScale*fNewSize;

			// check if particle is in view frustum
			int valid = 1;
			float limit = -1.74f*size; // 1.74 = rough distance from center to corner of a unit cube
			for( int j = 0; j < 6; j++ )
			{
				float dist = frustumN[j].x*x + frustumN[j].y*y + frustumN[j].z*z + frustumD[j];
				if ( dist < limit ) { valid = 0; break; }
			}
			if ( !valid ) continue;
			
			// vertices			
			m_pVertexArray[ (count*4 + 0)*3 + 0 ] = x + size*(-camHorz.x + camVert.x);
			m_pVertexArray[ (count*4 + 0)*3 + 1 ] = y + size*(-camHorz.y + camVert.y);
			m_pVertexArray[ (count*4 + 0)*3 + 2 ] = z + size*(-camHorz.z + camVert.z);

			m_pVertexArray[ (count*4 + 1)*3 + 0 ] = x + size*(-camHorz.x - camVert.x);
			m_pVertexArray[ (count*4 + 1)*3 + 1 ] = y + size*(-camHorz.y - camVert.y);
			m_pVertexArray[ (count*4 + 1)*3 + 2 ] = z + size*(-camHorz.z - camVert.z);

			m_pVertexArray[ (count*4 + 2)*3 + 0 ] = x + size*(camHorz.x + camVert.x);
			m_pVertexArray[ (count*4 + 2)*3 + 1 ] = y + size*(camHorz.y + camVert.y);
			m_pVertexArray[ (count*4 + 2)*3 + 2 ] = z + size*(camHorz.z + camVert.z);

			m_pVertexArray[ (count*4 + 3)*3 + 0 ] = x + size*(camHorz.x - camVert.x);
			m_pVertexArray[ (count*4 + 3)*3 + 1 ] = y + size*(camHorz.y - camVert.y);
			m_pVertexArray[ (count*4 + 3)*3 + 2 ] = z + size*(camHorz.z - camVert.z);

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
		float z = m_pParticles[ i ]->m_fZ;
		float size = m_pParticles[ i ]->m_fScale*fNewSize;

		// check if particle is in view frustum
		int valid = 1;
		float limit = -1.74f*size; // 1.74 = rough distance from center to corner of a unit cube
		for( int j = 0; j < 6; j++ )
		{
			float dist = frustumN[j].x*x + frustumN[j].y*y + frustumN[j].z*z + frustumD[j];
			if ( dist < limit ) { valid = 0; break; }
		}
		if ( !valid ) continue;
			
		// vertices			
		m_pVertexArray[ (count*4 + 0)*3 + 0 ] = x + size*(-camHorz.x + camVert.x);
		m_pVertexArray[ (count*4 + 0)*3 + 1 ] = y + size*(-camHorz.y + camVert.y);
		m_pVertexArray[ (count*4 + 0)*3 + 2 ] = z + size*(-camHorz.z + camVert.z);

		m_pVertexArray[ (count*4 + 1)*3 + 0 ] = x + size*(-camHorz.x - camVert.x);
		m_pVertexArray[ (count*4 + 1)*3 + 1 ] = y + size*(-camHorz.y - camVert.y);
		m_pVertexArray[ (count*4 + 1)*3 + 2 ] = z + size*(-camHorz.z - camVert.z);

		m_pVertexArray[ (count*4 + 2)*3 + 0 ] = x + size*(camHorz.x + camVert.x);
		m_pVertexArray[ (count*4 + 2)*3 + 1 ] = y + size*(camHorz.y + camVert.y);
		m_pVertexArray[ (count*4 + 2)*3 + 2 ] = z + size*(camHorz.z + camVert.z);

		m_pVertexArray[ (count*4 + 3)*3 + 0 ] = x + size*(camHorz.x - camVert.x);
		m_pVertexArray[ (count*4 + 3)*3 + 1 ] = y + size*(camHorz.y - camVert.y);
		m_pVertexArray[ (count*4 + 3)*3 + 2 ] = z + size*(camHorz.z - camVert.z);

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
		
	m_iParticlesDrawn += count;

	PlatformDrawParticles( count, m_pIndices, m_pVertexArray, m_pUVArray, (unsigned char*) m_pColorArray );
	
}

void AGK3DParticleEmitter::PlatformDrawParticles( UINT count, unsigned short *pIndices, float *pVertices, float *pUV, unsigned char *pColors )
{
	// set drawing parameters for quad rendering
	agk::PlatformSetBlendMode( m_iTransparencyMode );
	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );
	agk::PlatformSetCullMode( 0 );
	agk::PlatformSetDepthRange( 0, 1 );
	agk::PlatformSetDepthTest( 1 ); 
	agk::PlatformSetDepthBias( 0 );

	agk::ResetScissor();

	AGKShader *pShader = AGKShader::g_pShader3DParticlesTex;
	if ( !m_pImage ) pShader = AGKShader::g_pShader3DParticlesColor;
	pShader->SetTextureStage( m_pImage, 0, 0 );
	
	pShader->MakeActive();
	
	int locPos = pShader->GetAttribPosition();
	int locColor = pShader->GetAttribColor();
	int locTex = pShader->GetAttribTexCoord();

	if ( locPos >= 0 ) pShader->SetAttribFloat( locPos, 3, 0, pVertices );
	if ( locColor >= 0 ) pShader->SetAttribUByte( locColor, 4, 0, true, pColors );
	if ( locTex >= 0 ) pShader->SetAttribFloat( locTex, 2, 0, pUV );

	pShader->DrawIndices( count*6, pIndices );
}