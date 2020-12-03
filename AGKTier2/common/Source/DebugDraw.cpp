#include "DebugDraw.h"
#include "agk.h"

using namespace AGK;



void DebugDraw::DrawString(int x, int y, const char *string, ...)
{
	/*
	char buffer[128];

	va_list arg;
	va_start(arg, string);
	vsprintf(buffer, string, arg);
	va_end(arg);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(0.9f, 0.6f, 0.6f);
	glRasterPos2i(x, y);
	int32 length = (int32)strlen(buffer);
	for (int32 i = 0; i < length; ++i)
	{
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, buffer[i]);
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	*/
}

void DebugDraw::DrawAABB(b2AABB* aabb, const b2Color& c)
{
	
}

void DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	if ( !m_pShader ) return;

	float fMinX = 10000000;
	float fMinY = 10000000;
	float fMaxX = -10000000;
	float fMaxY = -10000000;
	for (int32 i = 0; i < vertexCount; ++i)
	{
		float x = agk::PhyToWorldX( vertices[ i ].x );
		float y = agk::PhyToWorldY( vertices[ i ].y );

		x = agk::WorldToScreenX( x );
		y = agk::WorldToScreenY( y );

		if ( x < fMinX ) fMinX = x;
		if ( y < fMinY ) fMinY = y;
		if ( x > fMaxX ) fMaxX = x;
		if ( y > fMaxY ) fMaxY = y;
	}

	if ( fMaxX < agk::GetScreenBoundsLeft() || fMaxY < agk::GetScreenBoundsTop() ) return;
	if ( fMinX > agk::GetScreenBoundsRight() || fMinY > agk::GetScreenBoundsBottom() ) return;

	float *pVertices = new float[ vertexCount*2 ];
	unsigned char *pColor = new unsigned char[ vertexCount*4 ];

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );

	for (int32 i = 0; i < vertexCount; ++i)
	{
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(vertices[i].x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(vertices[i].y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 255);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 255);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 255);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 255);
	}

	m_pShader->DrawPrimitives( AGK_LINE_LOOP, 0, vertexCount );

	delete [] pVertices;
	delete [] pColor;
}

void DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	if ( !m_pShader ) return;

	float fMinX = 10000000;
	float fMinY = 10000000;
	float fMaxX = -10000000;
	float fMaxY = -10000000;
	for (int32 i = 0; i < vertexCount; ++i)
	{
		float x = agk::PhyToWorldX( vertices[ i ].x );
		float y = agk::PhyToWorldY( vertices[ i ].y );

		x = agk::WorldToScreenX( x );
		y = agk::WorldToScreenY( y );

		if ( x < fMinX ) fMinX = x;
		if ( y < fMinY ) fMinY = y;
		if ( x > fMaxX ) fMaxX = x;
		if ( y > fMaxY ) fMaxY = y;
	}

	if ( fMaxX < agk::GetScreenBoundsLeft() || fMaxY < agk::GetScreenBoundsTop() ) return;
	if ( fMinX > agk::GetScreenBoundsRight() || fMinY > agk::GetScreenBoundsBottom() ) return;

	float *pVertices = new float[ vertexCount*2 ];
	unsigned char *pColor = new unsigned char[ vertexCount*4 ];

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );

	for (int32 i = 0; i < vertexCount; ++i)
	{
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(vertices[i].x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(vertices[i].y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 128);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 128);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 128);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 128);
	}
	
	m_pShader->DrawPrimitives( AGK_TRIANGLE_FAN, 0, vertexCount );
	
	for (int32 i = 0; i < vertexCount; ++i)
	{
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(vertices[i].x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(vertices[i].y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 255);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 255);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 255);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 255);
	}
	
	m_pShader->DrawPrimitives( AGK_LINE_LOOP, 0, vertexCount );

	delete [] pVertices;
	delete [] pColor;
}

void DebugDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	if ( !m_pShader ) return;

	float fMinX = agk::WorldToScreenX( agk::PhyToWorldX(center.x - radius) );
	float fMinY = agk::WorldToScreenY( agk::PhyToWorldY(center.y - radius) );
	float fMaxX = agk::WorldToScreenX( agk::PhyToWorldX(center.x + radius) );
	float fMaxY = agk::WorldToScreenY( agk::PhyToWorldY(center.y + radius) );

	if ( fMaxX < agk::GetScreenBoundsLeft() || fMaxY < agk::GetScreenBoundsTop() ) return;
	if ( fMinX > agk::GetScreenBoundsRight() || fMinY > agk::GetScreenBoundsBottom() ) return;

	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 theta = 0.0f;

	float *pVertices = new float[ agk::Ceil(k_segments*2) ];
	unsigned char *pColor = new unsigned char[ agk::Ceil(k_segments*4) ];

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );
	
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(v.x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(v.y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 255);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 255);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 255);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 255);;

		theta += k_increment;
	}
	
	m_pShader->DrawPrimitives( AGK_LINE_LOOP, 0, agk::Ceil(k_segments) );
	
	delete [] pVertices;
	delete [] pColor;
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	if ( !m_pShader ) return;

	float fMinX = agk::WorldToScreenX( agk::PhyToWorldX(center.x - radius) );
	float fMinY = agk::WorldToScreenY( agk::PhyToWorldY(center.y - radius) );
	float fMaxX = agk::WorldToScreenX( agk::PhyToWorldX(center.x + radius) );
	float fMaxY = agk::WorldToScreenY( agk::PhyToWorldY(center.y + radius) );

	if ( fMaxX < agk::GetScreenBoundsLeft() || fMaxY < agk::GetScreenBoundsTop() ) return;
	if ( fMinX > agk::GetScreenBoundsRight() || fMinY > agk::GetScreenBoundsBottom() ) return;

	const float32 k_segments = 16.0f;
	const float32 k_increment = 2.0f * b2_pi / k_segments;
	float32 theta = 0.0f;

	float *pVertices = new float[ agk::Ceil(k_segments*2) ];
	unsigned char *pColor = new unsigned char[ agk::Ceil(k_segments*4) ];

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );

	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(v.x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(v.y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 128);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 128);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 128);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 128);

		theta += k_increment;
	}

	m_pShader->DrawPrimitives( AGK_TRIANGLE_FAN, 0, agk::Ceil(k_segments) );

	theta = 0.0f;
	
	for (int32 i = 0; i < k_segments; ++i)
	{
		b2Vec2 v = center + radius * b2Vec2(cosf(theta), sinf(theta));
		
		pVertices[ i*2 ] = agk::WorldToScreenX( agk::PhyToWorldX(v.x) );
		pVertices[ i*2 + 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(v.y) );

		pColor[ i*4 ] = (unsigned char) (color.r * 255);
		pColor[ i*4 + 1 ] = (unsigned char) (color.g * 255);
		pColor[ i*4 + 2 ] = (unsigned char) (color.b * 255);
		pColor[ i*4 + 3 ] = (unsigned char) (color.a * 255);

		theta += k_increment;
	}

	m_pShader->DrawPrimitives( AGK_LINE_LOOP, 0, agk::Ceil(k_segments) );

	b2Vec2 p = center + radius * axis;
	pVertices[ 0 ] = agk::WorldToScreenX( agk::PhyToWorldX(center.x) ); 
	pVertices[ 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(center.y) );
	pVertices[ 2 ] = agk::WorldToScreenX( agk::PhyToWorldX(p.x) ); 
	pVertices[ 3 ] = agk::WorldToScreenY( agk::PhyToWorldY(p.y) );
	m_pShader->DrawPrimitives( AGK_LINES, 0, 2 );

	delete [] pVertices;
	delete [] pColor;
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	if ( !m_pShader ) return;

	float fMinX = p1.x;
	float fMinY = p1.y;
	float fMaxX = p1.x;
	float fMaxY = p1.y;

	if ( p2.x < fMinX ) fMinX = p2.x;
	if ( p2.y < fMinY ) fMinY = p2.y;
	if ( p2.x > fMaxX ) fMaxX = p2.x;
	if ( p2.y > fMaxY ) fMaxY = p2.y;

	fMinX = agk::WorldToScreenX( agk::PhyToWorldX(fMinX) );
	fMaxX = agk::WorldToScreenX( agk::PhyToWorldX(fMaxX) );
	fMinY = agk::WorldToScreenY( agk::PhyToWorldY(fMinY) );
	fMaxY = agk::WorldToScreenY( agk::PhyToWorldY(fMaxY) );

	if ( fMaxX < agk::GetScreenBoundsLeft() || fMaxY < agk::GetScreenBoundsTop() ) return;
	if ( fMinX > agk::GetScreenBoundsRight() || fMinY > agk::GetScreenBoundsBottom() ) return;

	float *pVertices = new float[ 4 ];
	unsigned char *pColor = new unsigned char[ 16 ];

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );

	pVertices[ 0 ] = agk::WorldToScreenX( agk::PhyToWorldX(p1.x) ); 
	pVertices[ 1 ] = agk::WorldToScreenY( agk::PhyToWorldY(p1.y) );
	pVertices[ 2 ] = agk::WorldToScreenX( agk::PhyToWorldX(p2.x) ); 
	pVertices[ 3 ] = agk::WorldToScreenY( agk::PhyToWorldY(p2.y) );

	pColor[ 0 ] = (unsigned char) (color.r * 255);
	pColor[ 1 ] = (unsigned char) (color.g * 255);
	pColor[ 2 ] = (unsigned char) (color.b * 255);
	pColor[ 3 ] = (unsigned char) (color.a * 255);

	pColor[ 4 ] = (unsigned char) (color.r * 255);
	pColor[ 5 ] = (unsigned char) (color.g * 255);
	pColor[ 6 ] = (unsigned char) (color.b * 255);
	pColor[ 7 ] = (unsigned char) (color.a * 255);
	
	m_pShader->DrawPrimitives( AGK_LINES, 0, 2 );

	delete [] pVertices;
	delete [] pColor;
}

void DebugDraw::DrawTransform(const b2Transform& xf)
{
	
}

void DebugDraw::DrawPoint(const b2Vec2& p, float32 size, const b2Color& color)
{
	if ( !m_pShader ) return;

	float x = agk::WorldToScreenX( agk::PhyToWorldX(p.x) );
	float y = agk::WorldToScreenY( agk::PhyToWorldY(p.y) );

	if ( x < agk::GetScreenBoundsLeft()  || y < agk::GetScreenBoundsTop() ) return;
	if ( x > agk::GetScreenBoundsRight() || y > agk::GetScreenBoundsBottom() ) return;

	float *pVertices = new float[ 2 ];
	unsigned char *pColor = new unsigned char[ 4 ];

	int locPos = m_pShader->GetAttribPosition();
	int locColor = m_pShader->GetAttribColor();

	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );

	if ( locPos >= 0 ) m_pShader->SetAttribFloat( locPos, 2, 0, pVertices );
	if ( locColor >= 0 ) m_pShader->SetAttribUByte( locColor, 4, 0, true, pColor );

	pVertices[ 0 ] = x; pVertices[ 1 ] = y;

	pColor[ 0 ] = (unsigned char) (color.r * 255);
	pColor[ 1 ] = (unsigned char) (color.g * 255);
	pColor[ 2 ] = (unsigned char) (color.b * 255);
	pColor[ 3 ] = (unsigned char) (color.a * 255);

	m_pShader->DrawPrimitives( AGK_POINTS, 0, 1 );
		
	delete [] pVertices;
	delete [] pColor;
}


// Destruction listener class

void MyDestructionListener::SayGoodbye( b2Joint* joint )
{
	agk::ClearJoint( joint );
}

// Ray cast callback
MyRayCastCallback::MyRayCastCallback()
{
	Reset();
}

void MyRayCastCallback::Reset()
{
	m_fixture = UNDEF;
	m_fraction = 1.0f;
	m_category = 0xffff;
	m_group = 0;
	m_sprite = UNDEF;
}

float32 MyRayCastCallback::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
{
	m_category = m_category & 0xffff;
	if ( m_category != 0 )
	{
		if ( (fixture->GetFilterData().categoryBits & m_category) == 0 ) return -1;
	}

	if ( m_group != 0 )
	{
		if ( fixture->GetFilterData().groupIndex != m_group ) return -1;
	}

	m_fixture = fixture;
	m_point = point;
	m_normal = normal;
	m_fraction = fraction;

	return fraction;
}
