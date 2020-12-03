#include "agk.h"

using namespace AGK;

FrameBuffer* FrameBuffer::g_pBoundFBO = 0;
FrameBuffer* FrameBuffer::g_pAllFrameBuffers = 0;

void FrameBuffer::DeleteImage( cImage *pImage )
{
	if ( !pImage ) return;

	FrameBuffer *pFBO = g_pAllFrameBuffers;
	FrameBuffer *pLast = 0;
	while ( pFBO )
	{
		FrameBuffer *pNext = pFBO->m_pNextFBO;

		if ( pFBO->m_pColor == pImage || pFBO->m_pDepth == pImage )
		{
			if ( pLast ) pLast->m_pNextFBO = pFBO->m_pNextFBO;
			else g_pAllFrameBuffers = pFBO->m_pNextFBO;

			if ( pFBO->IsBound() ) agk::BindDefaultFramebuffer();

			delete pFBO;
		}
		else pLast = pFBO;

		pFBO = pNext;
	}
}

FrameBuffer* FrameBuffer::FindFrameBuffer( cImage *pColor, cImage* pDepth, bool forceDepth )
{
	FrameBuffer *pFBO = g_pAllFrameBuffers;
	while ( pFBO )
	{
		if ( pFBO->m_pColor == pColor && pFBO->m_pDepth == pDepth ) 
		{
			if ( ((pFBO->m_bFlags & AGK_FBO_FORCE_DEPTH) != 0) == forceDepth ) return pFBO;
		}

		pFBO = pFBO->m_pNextFBO;
	}

	return 0;
}

void FrameBuffer::ClearAll()
{
	FrameBuffer::g_pBoundFBO = 0;
	agk::BindDefaultFramebuffer();

	while ( g_pAllFrameBuffers )
	{
		FrameBuffer *pNext = g_pAllFrameBuffers->m_pNextFBO;

		delete g_pAllFrameBuffers;

		g_pAllFrameBuffers = pNext;
	}
}

void FrameBuffer::ReloadAll()
{
	FrameBuffer::g_pBoundFBO = 0;
	agk::BindDefaultFramebuffer();

	FrameBuffer *pFBO = g_pAllFrameBuffers;
	while ( pFBO )
	{
		pFBO->PlatformDeleteFrameBuffer();
		pFBO = pFBO->m_pNextFBO;
	}

	pFBO = g_pAllFrameBuffers;
	while ( pFBO )
	{
		pFBO->PlatformCreateFrameBuffer( pFBO->m_pColor, pFBO->m_pDepth, (pFBO->m_bFlags & AGK_FBO_FORCE_DEPTH) != 0 );
		pFBO = pFBO->m_pNextFBO;
	}
}

FrameBuffer::FrameBuffer( cImage *pColor, bool bCShared, cImage* pDepth, bool bDShared, bool forceDepth )
{
	m_pColor = pColor;
	m_pDepth = pDepth;

	m_bFlags = 0;
	
	int width = 0;
	if ( pColor ) width = pColor->GetWidth();
	else if ( pDepth ) width = pDepth->GetWidth();
	int height = 0;
	if ( pColor ) height = pColor->GetHeight();
	else if ( pDepth ) height = pDepth->GetHeight();

	// if both width and height are a power of two then turn on mipmaps
	if ( width > 0 && (width & (width-1)) == 0 ) 
	{
		if ( height > 0 && (height & (height-1)) == 0 )
		{
			m_bFlags |= AGK_FBO_USE_MIPMAPS;
		}
	}
	
	if ( m_pColor && bCShared ) m_bFlags |= AGK_FBO_SHARED_COLOR;
	if ( m_pDepth && bDShared ) m_bFlags |= AGK_FBO_SHARED_DEPTH;
	if ( forceDepth ) m_bFlags |= AGK_FBO_FORCE_DEPTH;

	m_pNextFBO = 0;

	m_iRBODepth = 0;
	m_iFBO = 0;
    
    if ( m_pColor ) m_pColor->UnBind();
    if ( m_pDepth ) m_pDepth->UnBind();

	PlatformCreateFrameBuffer( pColor, pDepth, forceDepth );

	m_pNextFBO = g_pAllFrameBuffers;
	g_pAllFrameBuffers = this;
}

FrameBuffer::~FrameBuffer()
{
	FrameBuffer *pFBO = g_pAllFrameBuffers;
	FrameBuffer *pLast = 0;
	while ( pFBO )
	{
		if ( pFBO == this )
		{
			if ( pLast ) pLast->m_pNextFBO = pFBO->m_pNextFBO;
			else g_pAllFrameBuffers = pFBO->m_pNextFBO;

			if ( pFBO->IsBound() ) agk::BindDefaultFramebuffer();
			break;
		}
		else pLast = pFBO;

		pFBO = pFBO->m_pNextFBO;
	}

	PlatformDeleteFrameBuffer();

	// only delete the images if they are not shared with anything
	if ( m_pColor && !(m_bFlags & AGK_FBO_SHARED_COLOR) ) delete m_pColor;
	if ( m_pDepth && !(m_bFlags & AGK_FBO_SHARED_DEPTH) ) delete m_pDepth;
}

void FrameBuffer::Bind()
{
	if ( g_pBoundFBO == this ) return;

	if ( m_iFBO == 0 ) 
	{
		agk::Warning( "Tried to bind frame buffer that doesn't exist" );
		return;
	}

	if ( g_pBoundFBO ) g_pBoundFBO->GenerateMipmaps();

	PlatformBind();

	g_pBoundFBO = this;
}

void FrameBuffer::GenerateMipmaps()
{
	if ( m_pColor && UseMipmaps() ) m_pColor->GenerateMipmaps();
}