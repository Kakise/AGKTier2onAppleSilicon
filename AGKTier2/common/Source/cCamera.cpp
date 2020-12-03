#include "agk.h"

using namespace AGK;

cCamera* cCamera::g_pAllCameras = 0;

// static functions
void cCamera::ResetAllUpdated()
{
	cCamera *pCamera = g_pAllCameras;
	while( pCamera )
	{
		pCamera->m_iFlags &= ~AGK_CAMERA_UPDATED;
		pCamera = pCamera->m_pNextCamera;
	}
}

void cCamera::ResetAllProjUpdated()
{
	cCamera *pCamera = g_pAllCameras;
	while( pCamera )
	{
		pCamera->m_iFlags &= ~AGK_CAMERA_PROJ_UPDATED;
		pCamera = pCamera->m_pNextCamera;
	}
}

void cCamera::UpdateAllAspectRatio( float aspect )
{
	cCamera *pCamera = g_pAllCameras;
	while( pCamera )
	{
		pCamera->SetAspectRatio( aspect );
		pCamera = pCamera->m_pNextCamera;
	}
}

void cCamera::UpdateAllUsingFBO( int mode )
{
	cCamera *pCamera = g_pAllCameras;
	while( pCamera )
	{
		pCamera->SetUsingFBO( mode );
		pCamera = pCamera->m_pNextCamera;
	}
}

// non static functions
cCamera::cCamera()
{
	m_iFlags = AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL;
	m_fFOV = 70;
	m_fNear = 1;
	m_fFar = 1000;
	m_fAspect = agk::GetDeviceWidth()/(float)agk::GetDeviceHeight();
	m_fOrthoWidth = 40;
	m_fLeft = -40;
	m_fRight = 40;
	m_fTop = 40 / m_fAspect;
	m_fBottom = -40 / m_fAspect;

	if ( g_pAllCameras ) g_pAllCameras->m_pPrevCamera = this;
	m_pPrevCamera = 0;
	m_pNextCamera = g_pAllCameras;
	g_pAllCameras = this;
}

cCamera::~cCamera()
{
	//remove from global list
	if ( m_pPrevCamera )
	{
		m_pPrevCamera->m_pNextCamera = m_pNextCamera;
	}
	else
	{
		g_pAllCameras = m_pNextCamera;
	}

	if ( m_pNextCamera )
	{
		m_pNextCamera->m_pPrevCamera = m_pPrevCamera;
	}

	// remove from any tweens
	TweenInstance::DeleteTarget( this );
}

void cCamera::SetAspectRatio( float aspect )
{
	if ( aspect <= 0.0001f ) aspect = 0.0001f;
	if ( aspect >= 10000 ) aspect = 10000;
	
	m_fAspect = aspect;
	m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
}

void cCamera::SetFOV( float fov )
{
	if ( fov < 0 ) fov = 0;
	if ( fov > 179 ) fov = 179;

	m_fFOV = fov;
	m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
}

void cCamera::SetOrthoWidth( float width )
{
	if ( width < 0 ) width = 0;
	if ( width == 0 ) return;

	m_fOrthoWidth = width;

	// also set the bounds
	m_fLeft = -m_fOrthoWidth;
	m_fRight = m_fOrthoWidth;
	m_fTop = m_fRight/m_fAspect;
	m_fBottom = -m_fRight/m_fAspect;

	m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
}

void cCamera::SetRange( float fNear, float fFar )
{
	if ( fNear < 0 ) fNear = 0;
	if ( fFar < fNear ) fFar = fNear;

	m_fNear = fNear;
	m_fFar = fFar;
	m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
}

void cCamera::SetBounds( float fLeft, float fRight, float fTop, float fBottom )
{
	m_fLeft = fLeft;
	m_fRight = fRight;
	m_fTop = fTop;
	m_fBottom = fBottom;

	// only need to update if matrix is off center
	if ( m_iFlags & AGK_CAMERA_OFF_CENTER ) m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
}

void cCamera::SetOffCenter( int mode )
{
	int currentMode = (m_iFlags & AGK_CAMERA_OFF_CENTER) ? 1 : 0;

	if ( mode != currentMode )
	{
		m_iFlags |= (AGK_CAMERA_PROJ_NEEDS_UPDATE | AGK_CAMERA_PLANE_NEEDS_UPDATE_ALL);
		if ( mode == 0 ) m_iFlags &= ~AGK_CAMERA_OFF_CENTER;
		else m_iFlags |= AGK_CAMERA_OFF_CENTER;
	}
}

void cCamera::SetUsingFBO( int mode )
{
	int currentMode = (m_iFlags & AGK_CAMERA_USING_FBO) ? 1 : 0;
	mode = mode ? 1 : 0;

	// flip proj matrix if mode has changed
	if ( currentMode != mode ) 
	{
		m_matProj.mat[1][1] = -m_matProj.mat[1][1];
		m_matProj.mat[2][1] = -m_matProj.mat[2][1];
		m_iFlags |= AGK_CAMERA_PROJ_UPDATED;

		if ( mode == 0 ) m_iFlags &= ~AGK_CAMERA_USING_FBO;
		else m_iFlags |= AGK_CAMERA_USING_FBO;
	}
}

const AGKMatrix4* cCamera::GetProjMatrix() 
{ 
	if ( (m_iFlags & AGK_CAMERA_PROJ_NEEDS_UPDATE) == 0 ) return &m_matProj; 
	m_iFlags &= ~AGK_CAMERA_PROJ_NEEDS_UPDATE;
	m_iFlags |= AGK_CAMERA_PROJ_UPDATED;

	if ( (m_iFlags & AGK_CAMERA_OFF_CENTER) == 0 ) m_matProj.MakeProj( m_fFOV, m_fAspect, m_fNear, m_fFar, m_fOrthoWidth );
	else m_matProj.MakeProjOffCenter( (m_fFOV == 0) ? 1 : 0, m_fLeft, m_fRight, m_fTop, m_fBottom, m_fNear, m_fFar );

	if ( m_iFlags & AGK_CAMERA_USING_FBO )
	{
		m_matProj.mat[1][1] = -m_matProj.mat[1][1];
		m_matProj.mat[2][1] = -m_matProj.mat[2][1];
	}

	return &m_matProj; 
}

void cCamera::GetFrustumPlane( UINT index, AGKVector &n, float &d )
{
	if ( index > 5 ) index = 5;

	// 0 = near
	// 1 = far
	// 2 = left
	// 3 = right
	// 4 = bottom
	// 5 = top

	UpdateNode();
	int mask = AGK_CAMERA_PLANE_NEEDS_UPDATE_0 << index;
	if ( (m_iFlags & mask) != 0 )
	{
		// something changed, recalculate
		AGKVector normal, origin;
		float sign = -1;
		float fValue = m_fRight;
		float fValue2 = m_fTop;
		switch( index )
		{
			case 0: normal.Set(0, 0, 1); origin.Set(0,0,m_fNear); break;
			case 1: normal.Set(0, 0,-1); origin.Set(0,0,m_fFar); break;
			case 2: sign = 1; fValue = m_fLeft;
			case 3:
			{
				if ( m_fFOV == 0 )
				{
					origin.Set( fValue, 0, 0 );
					normal.Set( sign, 0, 0 ); 
				}
				else
				{
					if ( m_iFlags & AGK_CAMERA_OFF_CENTER ) 
					{
						normal.Set( sign*m_fNear, 0, -sign*fValue ); 
						normal.Normalize();
					}
					else
					{
						float ang = m_fFOV*PI / 360.0f;
						float sAng = sin( ang );
						float cAng = cos( ang );
						normal.Set( sign*cAng, 0, sAng ); 
					}
				}
				break;
			}
			case 4: sign = 1; fValue2 = m_fBottom;
			case 5: 
			{
				if ( m_fFOV == 0 )
				{
					origin.Set( 0, fValue2, 0 );
					normal.Set( 0, sign, 0 ); 
				}
				else
				{
					if ( m_iFlags & AGK_CAMERA_OFF_CENTER ) 
					{
						normal.Set( 0, sign*m_fNear, -sign*fValue2 ); 
						normal.Normalize();
					}
					else
					{
						float ang = atan( tan(m_fFOV*PI / 360.0f) / m_fAspect );
						float sAng = sin( ang );
						float cAng = cos( ang );
						normal.Set( 0, sign*cAng, sAng ); 
					}
				}
				break;
			}
		}

		// transform normal and origin by view matrix
		normal = rotFinal() * normal;
		origin = rotFinal() * origin;
		origin += posFinal();

		m_vFrustumN[ index ] = normal;
		m_fFrustumD[ index ] = -origin.Dot( normal );

		m_iFlags &= ~mask;
	}

	n = m_vFrustumN[ index ];
	d = m_fFrustumD[ index ];
}

void cCamera::GetFrustumPoints( float dist, AGKVector *points )
{
	points[0].z = dist;
	points[1].z = dist;
	points[2].z = dist;
	points[3].z = dist;

	if ( m_fFOV == 0 )
	{
		points[0].x = m_fLeft; points[0].y = m_fBottom;
		points[1].x = m_fLeft; points[1].y = m_fTop;
		points[2].x = m_fRight; points[2].y = m_fBottom;
		points[3].x = m_fRight; points[3].y = m_fTop;
	}
	else
	{
		if ( m_iFlags & AGK_CAMERA_OFF_CENTER ) 
		{
			points[0].x = m_fLeft; points[0].y = m_fBottom;
			points[1].x = m_fLeft; points[1].y = m_fTop;
			points[2].x = m_fRight; points[2].y = m_fBottom;
			points[3].x = m_fRight; points[3].y = m_fTop;

			float diff = dist / m_fNear;
			for( int i = 0; i < 4; i++ )
			{
				points[i].x *= diff;
				points[i].y *= diff;
			}
		}
		else
		{
			float tanX = agk::Tan( m_fFOV / 2.0f );
			float tanY = tanX / m_fAspect;
			points[0].x = -tanX; points[0].y = -tanY;
			points[1].x = -tanX; points[1].y = tanY;
			points[2].x = tanX; points[2].y = -tanY;
			points[3].x = tanX; points[3].y = tanY;

			for( int i = 0; i < 4; i++ )
			{
				points[i].x *= dist;
				points[i].y *= dist;
			}
		}
	}

	for( int i = 0; i < 4; i++ )
	{
		points[i] = rotFinal() * points[i];
		points[i] += posFinal();
	}
}