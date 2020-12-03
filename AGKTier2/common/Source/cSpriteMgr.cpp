#include "agk.h"
#include "DebugDraw.h"

using namespace AGK;

cSpriteMgrEx::cSpriteMgrEx()
{
	m_pSprites = 0;
	m_pLastSprite = 0;

	iCurrentCount = 0;
	iCurrentCountAll = 0;
	pVertices = UNDEF;
	pUV = UNDEF;
	pColor = UNDEF;
	pIndices = UNDEF;
	
	m_iLastSorted = 0; 
	m_iLastDrawn = 0; 
	m_iLastTotal = 0;
	m_iLastDrawCalls = 0;

	m_pCurrentShader = 0;
	iCurrentTexture = 0;

	m_pDrawList = 0;
}

cSpriteMgrEx::~cSpriteMgrEx()
{
	ClearAll();
}

void cSpriteMgrEx::ClearSprites()
{
	cSpriteContainer *pCont = m_pSprites;
	cSpriteContainer *pLast = 0;
	
	while ( pCont )
	{
		if ( pCont->GetType() == 1 )
		{
			cSpriteContainer *pNext = pCont->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pCont ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pCont;
			pCont = pNext;
		}
		else
		{
			pLast = pCont;
			pCont = pCont->m_pNext;
		}
	}
	m_pLastSprite = pLast;

	if ( m_pDrawList ) delete [] m_pDrawList;
	if ( pVertices ) delete [] pVertices;
	if ( pUV ) delete [] pUV;
	if ( pColor ) delete [] pColor;
	if ( pIndices ) delete [] pIndices;

	m_pDrawList = 0;
	pVertices = UNDEF;
	pUV = UNDEF;
	pColor = UNDEF;
	pIndices = UNDEF;

	m_pCurrentShader = 0;
	iCurrentTexture = 0;

	iCurrentCount = 0;
	iCurrentCountAll = 0;
}

void cSpriteMgrEx::ClearAll()
{
	cSpriteContainer *pCont;
	
	while ( m_pSprites )
	{
		pCont = m_pSprites;
		m_pSprites = m_pSprites->m_pNext;
		delete pCont;
	}
	m_pSprites = 0;
	m_pLastSprite = 0;

	if ( m_pDrawList ) delete [] m_pDrawList;
	if ( pVertices ) delete [] pVertices;
	if ( pUV ) delete [] pUV;
	if ( pColor ) delete [] pColor;
	if ( pIndices ) delete [] pIndices;

	m_pDrawList = 0;
	pVertices = UNDEF;
	pUV = UNDEF;
	pColor = UNDEF;
	pIndices = UNDEF;

	m_pCurrentShader = 0;
	iCurrentTexture = 0;

	iCurrentCount = 0;
	iCurrentCountAll = 0;
}

void cSpriteMgrEx::AddSprite( cSprite* sprite )
{
	if ( !sprite ) return;

	cSpriteContainer *pNewMember = new cSpriteContainer();
	pNewMember->SetSprite( sprite );
	sprite->m_bManaged = true;

	if ( !AddContainer( pNewMember ) ) delete pNewMember;
}

void cSpriteMgrEx::AddParticles( cParticleEmitter* particles )
{
	if ( !particles ) return;

	cSpriteContainer *pNewMember = new cSpriteContainer();
	pNewMember->SetParticles( particles );

	if ( !AddContainer( pNewMember ) ) delete pNewMember;
}

void cSpriteMgrEx::AddText( cText* text )
{
	if ( !text ) return;

	cSpriteContainer *pNewMember = new cSpriteContainer();
	pNewMember->SetText( text );

	if ( !AddContainer( pNewMember ) ) delete pNewMember;
}

void cSpriteMgrEx::AddEditBox( cEditBox* editbox )
{
	if ( !editbox ) return;

	cSpriteContainer *pNewMember = new cSpriteContainer();
	pNewMember->SetEditBox( editbox );

	if ( !AddContainer( pNewMember ) ) delete pNewMember;
}

void cSpriteMgrEx::AddSkeleton2D( Skeleton2D* skeleton )
{
	if ( !skeleton ) return;

	cSpriteContainer *pNewMember = new cSpriteContainer();
	pNewMember->SetSkeleton2D( skeleton );

	if ( !AddContainer( pNewMember ) ) delete pNewMember;
}

bool cSpriteMgrEx::AddContainer( cSpriteContainer* pNewMember )
{
	if ( !pNewMember ) return false;
	if ( pNewMember->GetType() == 0 ) return false;

	pNewMember->m_pNext = UNDEF;
	
	if ( pNewMember->GetType() != 1 )
	{
		// no sorting
		/*
		if ( m_pLastSprite )
		{
			pNewMember->m_pPrev = m_pLastSprite;
			m_pLastSprite->m_pNext = pNewMember;
			m_pLastSprite = pNewMember;
		}
		else
		{
			pNewMember->m_pPrev = 0;
			m_pSprites = pNewMember;
			m_pLastSprite = pNewMember;
		}
		*/

		pNewMember->m_pPrev = 0;
		pNewMember->m_pNext = m_pSprites;
		if ( m_pSprites ) m_pSprites->m_pPrev = pNewMember;
		m_pSprites = pNewMember;
		if ( !m_pLastSprite ) m_pLastSprite = pNewMember;
	}
	else
	{
		// sort by created
		cSpriteContainer *pCont = m_pSprites;
		while ( pCont )
		{
			if ( pCont->GetCreated() < pNewMember->GetCreated() )
			{
				pNewMember->m_pPrev = pCont->m_pPrev;
				pNewMember->m_pNext = pCont;
				if ( !pCont->m_pPrev ) m_pSprites = pNewMember;
				else pCont->m_pPrev->m_pNext = pNewMember;
				pCont->m_pPrev = pNewMember;
				break;
			}

			pCont = pCont->m_pNext;
		}

		if ( !pCont )
		{
			if ( m_pLastSprite )
			{
				pNewMember->m_pPrev = m_pLastSprite;
				pNewMember->m_pNext = 0;
				m_pLastSprite->m_pNext = pNewMember;
				m_pLastSprite = pNewMember;
			}
			else
			{
				pNewMember->m_pPrev = 0;
				pNewMember->m_pNext = 0;
				m_pSprites = pNewMember;
				m_pLastSprite = pNewMember;
			}
		}
	}

	return true;
}

void cSpriteMgrEx::RemoveSprite( cSprite* sprite )
{
	if ( !sprite ) return;
	sprite->m_bManaged = false;
	if ( !m_pLastSprite ) return;

	// if it is the last in the list we can do a quick remove
	if ( m_pLastSprite->GetType() == 1 && m_pLastSprite->GetSprite() == sprite )
	{
		cSpriteContainer *pCont = m_pLastSprite;
		m_pLastSprite = pCont->m_pPrev;

		if ( !pCont->m_pPrev )
		{
			m_pSprites = 0;
		}
		else
		{
			pCont->m_pPrev->m_pNext = 0;
		}

		delete pCont;
		return;
	}

	cSpriteContainer *pMember = m_pSprites;
	cSpriteContainer *pLast = UNDEF;
	while ( pMember )
	{
		if ( pMember->GetType() == 1 && pMember->GetSprite() == sprite )
		{
			//found, remove it
			cSpriteContainer *pNext = pMember->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pMember ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pMember;
			pMember = pNext;
			continue;
		}

		pLast = pMember;
		pMember = pMember->m_pNext;
	}
}

void cSpriteMgrEx::RemoveParticles( cParticleEmitter* particles )
{
	if ( !particles ) return;
	if ( !m_pLastSprite ) return;

	// if it is the last in the list we can do a quick remove
	if ( m_pLastSprite->GetType() == 2 && m_pLastSprite->GetParticles() == particles )
	{
		cSpriteContainer *pCont = m_pLastSprite;
		m_pLastSprite = pCont->m_pPrev;

		if ( !pCont->m_pPrev )
		{
			m_pSprites = 0;
		}
		else
		{
			pCont->m_pPrev->m_pNext = 0;
		}

		delete pCont;
		return;
	}

	cSpriteContainer *pMember = m_pSprites;
	cSpriteContainer *pLast = UNDEF;
	while ( pMember )
	{
		if ( pMember->GetType() == 2 && pMember->GetParticles() == particles )
		{
			//found, remove it
			cSpriteContainer *pNext = pMember->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pMember ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pMember;
			pMember = pNext;
			continue;
		}

		pLast = pMember;
		pMember = pMember->m_pNext;
	}
}

void cSpriteMgrEx::RemoveText( cText* text )
{
	if ( !text ) return;
	if ( !m_pLastSprite ) return;

	// if it is the last in the list we can do a quick remove
	if ( m_pLastSprite->GetType() == 3 && m_pLastSprite->GetText() == text )
	{
		cSpriteContainer *pCont = m_pLastSprite;
		m_pLastSprite = pCont->m_pPrev;

		if ( !pCont->m_pPrev )
		{
			m_pSprites = 0;
		}
		else
		{
			pCont->m_pPrev->m_pNext = 0;
		}

		delete pCont;
		return;
	}

	cSpriteContainer *pMember = m_pSprites;
	cSpriteContainer *pLast = UNDEF;
	while ( pMember )
	{
		if ( pMember->GetType() == 3 && pMember->GetText() == text )
		{
			//found, remove it
			cSpriteContainer *pNext = pMember->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pMember ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pMember;
			pMember = pNext;
			continue;
		}

		pLast = pMember;
		pMember = pMember->m_pNext;
	}
}

void cSpriteMgrEx::RemoveEditBox( cEditBox* editbox )
{
	if ( !editbox ) return;
	if ( !m_pLastSprite ) return;

	// if it is the last in the list we can do a quick remove
	if ( m_pLastSprite->GetType() == 4 && m_pLastSprite->GetEditBox() == editbox )
	{
		cSpriteContainer *pCont = m_pLastSprite;
		m_pLastSprite = pCont->m_pPrev;

		if ( !pCont->m_pPrev )
		{
			m_pSprites = 0;
		}
		else
		{
			pCont->m_pPrev->m_pNext = 0;
		}

		delete pCont;
		return;
	}

	cSpriteContainer *pMember = m_pSprites;
	cSpriteContainer *pLast = UNDEF;
	while ( pMember )
	{
		if ( pMember->GetType() == 4 && pMember->GetEditBox() == editbox )
		{
			//found, remove it
			cSpriteContainer *pNext = pMember->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pMember ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pMember;
			pMember = pNext;
			continue;
		}

		pLast = pMember;
		pMember = pMember->m_pNext;
	}
}

void cSpriteMgrEx::RemoveSkeleton2D( Skeleton2D* skeleton )
{
	if ( !skeleton ) return;
	if ( !m_pLastSprite ) return;

	// if it is the last in the list we can do a quick remove
	if ( m_pLastSprite->GetType() == 5 && m_pLastSprite->GetSkeleton2D() == skeleton )
	{
		cSpriteContainer *pCont = m_pLastSprite;
		m_pLastSprite = pCont->m_pPrev;

		if ( !pCont->m_pPrev )
		{
			m_pSprites = 0;
		}
		else
		{
			pCont->m_pPrev->m_pNext = 0;
		}

		delete pCont;
		return;
	}

	cSpriteContainer *pMember = m_pSprites;
	cSpriteContainer *pLast = UNDEF;
	while ( pMember )
	{
		if ( pMember->GetType() == 5 && pMember->GetSkeleton2D() == skeleton )
		{
			//found, remove it
			cSpriteContainer *pNext = pMember->m_pNext;
			if ( pNext ) pNext->m_pPrev = pLast;

			if ( m_pLastSprite == pMember ) m_pLastSprite = pLast;
			
			if ( pLast ) pLast->m_pNext = pNext;
			else m_pSprites = pNext;

			delete pMember;
			pMember = pNext;
			continue;
		}

		pLast = pMember;
		pMember = pMember->m_pNext;
	}
}

// Each sprite has an internal check to update or not, set using SetActive()
void cSpriteMgrEx::UpdateAll( float time )
{
	m_iLastTotal = 0;

	cSpriteContainer *pMember = m_pSprites;
	while ( pMember )
	{
		if ( pMember->GetType() == 1 ) 
		{
			m_iLastTotal++;
			pMember->GetSprite()->Update( time );
		}

		pMember = pMember->m_pNext;
	}
}

// Draw only sprites that match side = 0: all, side = 1: less or equal than depth, side = 2: greater than depth
void cSpriteMgrEx::DrawSplit( int depth, int side )
{
	if ( !m_pSprites ) return;

	int iCount = 0;
	int iCountAll = 0;
	cSpriteContainer *pMember = m_pSprites;
	while ( pMember )
	{
		if ( pMember->GetType() == 1 )
		{
			if ( pMember->GetSprite()->GetVisible() && pMember->GetSprite()->GetInScreen() ) 
			{
				iCount++;
				iCountAll++;
			}
		}
		else iCountAll++;
		pMember = pMember->m_pNext;
	}

	if ( iCountAll == 0 ) return;

	static int iCount2 = 0;
	iCount2++;

	// resize arrays if we have more sprites than before
	if ( iCountAll > iCurrentCountAll || iCount2 == 150 )
	{
		if ( m_pDrawList ) delete [] m_pDrawList;
		m_pDrawList = new AGKSortValue[ iCountAll ];

		iCurrentCountAll = iCountAll;
	}

	if ( iCount > iCurrentCount || iCount2 == 300 )
	{
		if ( pVertices ) delete [] pVertices;
		if ( pUV ) delete [] pUV;
		if ( pColor ) delete [] pColor;
		if ( pIndices ) delete [] pIndices;

		// assign arrays
		pVertices = new float[ iCount*12 ];
		pUV = new float[ iCount*8 ];
		pColor = new unsigned char[ iCount*16 ];
		pIndices = new unsigned short[ iCount*6 ];
		
		for ( int i = 0; i < iCount; i++ )
		{
			pIndices[ i*6 + 0 ] = i*4 + 0;
			pIndices[ i*6 + 1 ] = i*4 + 1;
			pIndices[ i*6 + 2 ] = i*4 + 2;
			
			pIndices[ i*6 + 3 ] = i*4 + 2;
			pIndices[ i*6 + 4 ] = i*4 + 1;
			pIndices[ i*6 + 5 ] = i*4 + 3;
		}
		
		iCurrentCount = iCount;
	}

	// always resize after 300 frames to allow it to shrink
	if ( iCount2 > 300 ) iCount2 = 0;

	if ( side != 1 )
	{
		m_iLastDrawn = 0;
		m_iLastDrawCalls = 0;
	}

	iCount = 0;
	pMember = m_pSprites;
	while ( pMember )
	{
		// 0 = all, 1 = front, 2 = back
		if ( side == 1 )
		{
			if ( pMember->GetDepthInt() > depth )
			{
				pMember = pMember->m_pNext;
				continue;
			}
		}
		else if ( side == 2 )
		{
			if ( pMember->GetDepthInt() <= depth )
			{
				pMember = pMember->m_pNext;
				continue;
			}
		}

		if ( pMember->GetType() == 1 )
		{
			if ( pMember->GetSprite()->GetVisible() && pMember->GetSprite()->GetInScreen() ) 
			{
				m_pDrawList[ iCount ].ptr = (void*)pMember;
				m_pDrawList[ iCount ].iValue = agk::SortIntToUINT( pMember->GetDepthInt() );
				iCount++;
			}
		}
		else
		{
			m_pDrawList[ iCount ].ptr = (void*)pMember;
			m_pDrawList[ iCount ].iValue = agk::SortIntToUINT( pMember->GetDepthInt() );
			iCount++;
		}

		pMember = pMember->m_pNext;
	}

	if ( iCount == 0 ) return;

	int iTotalSprites = iCount;

	agk::SortArray( m_pDrawList, iCount );
	
	agk::PlatformBindBuffer( 0 );
	agk::PlatformBindIndexBuffer( 0 );
	
	iCount = 0;
	pMember = (cSpriteContainer*) m_pDrawList[ iTotalSprites-1 ].ptr;
	iCurrentTexture = pMember->GetTextureID();
	int iLastScissorX = 0;
	int iLastScissorY = 0;
	int iLastScissorWidth = 0;
	int iLastScissorHeight = 0;
	int lastType = 0;
	int lastTransparency = pMember->GetDrawMode();
	m_pCurrentShader = pMember->GetShader();
	if ( m_pCurrentShader ) 
	{
		m_pCurrentShader->MakeActive();

		int locPos = m_pCurrentShader->GetAttribPosition();
		int locColor = m_pCurrentShader->GetAttribColor();
		int locTex = m_pCurrentShader->GetAttribTexCoord();

		if ( locPos >= 0 ) m_pCurrentShader->SetAttribFloat( locPos, 3, 0, pVertices );
		if ( locColor >= 0 ) m_pCurrentShader->SetAttribUByte( locColor, 4, 0, true, pColor );
		if ( locTex >= 0 ) m_pCurrentShader->SetAttribFloat( locTex, 2, 0, pUV );
	}

	agk::PlatformSetBlendMode( lastTransparency );
	agk::PlatformSetCullMode( 0 );
	agk::ResetScissor();
	agk::PlatformSetDepthTest( 0 );
	agk::PlatformSetDepthWrite( 0 );
	agk::PlatformSetDepthRange( 0, 1 );

	for ( int i = iTotalSprites-1; i >= 0; i-- )
	{
		pMember = (cSpriteContainer*) m_pDrawList[ i ].ptr;

		switch( pMember->GetType() )
		{
			case 5: // skeleton 2D
			{
				if ( iCount > 0 ) 
				{
					m_iLastDrawn += iCount;
					m_iLastDrawCalls++;
					if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );
					iCount = 0;
				}
				pMember->GetSkeleton2D()->Draw();
				//pMember->GetSkeleton2D()->DrawBones();
				iLastScissorX = 0;
				iLastScissorY = 0;
				iLastScissorWidth = 0;
				iLastScissorHeight = 0;
				agk::ResetScissor();
				if ( m_pCurrentShader ) m_pCurrentShader->MakeActive();
				break;
			}

			case 4: // editbox
			{
				if ( iCount > 0 ) 
				{
					m_iLastDrawn += iCount;
					m_iLastDrawCalls++;
					if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );
					iCount = 0;
				}
				pMember->GetEditBox()->Draw();
				iLastScissorX = 0;
				iLastScissorY = 0;
				iLastScissorWidth = 0;
				iLastScissorHeight = 0;
				agk::ResetScissor();
				if ( m_pCurrentShader ) m_pCurrentShader->MakeActive();
				break;
			}

			case 3: // text
			{
				if ( iCount > 0 ) 
				{
					m_iLastDrawn += iCount;
					m_iLastDrawCalls++;
					if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );
					iCount = 0;
				}
				pMember->GetText()->Draw();
				iLastScissorX = 0;
				iLastScissorY = 0;
				iLastScissorWidth = 0;
				iLastScissorHeight = 0;
				agk::ResetScissor();
				if ( m_pCurrentShader ) m_pCurrentShader->MakeActive();
				break;
			}
			
			case 2: // particles
			{
				if ( iCount > 0 ) 
				{
					m_iLastDrawn += iCount;
					m_iLastDrawCalls++;
					if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );
					iCount = 0;
				}
				pMember->GetParticles()->DrawAll();
				iLastScissorX = 0;
				iLastScissorY = 0;
				iLastScissorWidth = 0;
				iLastScissorHeight = 0;
				agk::ResetScissor();
				if ( m_pCurrentShader ) m_pCurrentShader->MakeActive();
				break;
			}

			case 1: // sprite
			{
				// multi-image sprites must be drawn separately
				if ( pMember->GetSprite()->HasAdditionalImages() )
				{
					if ( iCount > 0 ) 
					{
						m_iLastDrawn += iCount;
						m_iLastDrawCalls++;
						if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );
						iCount = 0;
					}
					pMember->GetSprite()->Draw();
					iLastScissorX = 0;
					iLastScissorY = 0;
					iLastScissorWidth = 0;
					iLastScissorHeight = 0;
					agk::ResetScissor();
					if ( m_pCurrentShader ) m_pCurrentShader->MakeActive();
					lastType = -1;
					continue;
				}

				// set state
				if ( lastType != 1 )
				{
					agk::PlatformSetBlendMode( lastTransparency );
					agk::PlatformSetCullMode( 0 );
					agk::PlatformSetDepthTest( 0 );
					agk::PlatformSetDepthWrite( 0 );
					agk::PlatformSetDepthRange( 0, 1 );
					
					if ( m_pCurrentShader )
					{
						// other members change the attribute pointers, need to reset them here
						int locPos = m_pCurrentShader->GetAttribPosition();
						int locColor = m_pCurrentShader->GetAttribColor();
						int locTex = m_pCurrentShader->GetAttribTexCoord();

						if ( locPos >= 0 ) m_pCurrentShader->SetAttribFloat( locPos, 3, 0, pVertices );
						if ( locColor >= 0 ) m_pCurrentShader->SetAttribUByte( locColor, 4, 0, true, pColor );
						if ( locTex >= 0 ) m_pCurrentShader->SetAttribFloat( locTex, 2, 0, pUV );
					}
						
					cImage::BindTexture( iCurrentTexture );
				}
				
				UINT iNextTexture = pMember->GetTextureID();
				AGKShader *pNextShader = pMember->GetShader();
				cSprite *pSprite = pMember->GetSprite();

				int iScissorX, iScissorY, iScissorWidth, iScissorHeight;
				pMember->GetSprite()->GetClipValues( iScissorX, iScissorY, iScissorWidth, iScissorHeight );

				// don't try and draw more than about 15000 sprites in one draw call as indices can't exceed 65535 (4 vertices per sprite)
				if ( (pNextShader != m_pCurrentShader) 
				  || (iNextTexture != iCurrentTexture)
				  || (iScissorX != iLastScissorX || iScissorY != iLastScissorY || iScissorWidth != iLastScissorWidth || iScissorHeight != iLastScissorHeight ) 
				  || iCount > 12000
				  || lastTransparency != pMember->GetDrawMode()
				  || pNextShader->HasPerSpriteUniforms() )
				{
					m_iLastDrawn += iCount;
					m_iLastDrawCalls++;
					if ( iCount > 0 && m_pCurrentShader ) 
					{
						m_pCurrentShader->DrawIndices( iCount*6, pIndices );
					}
					iCount = 0;
					
					// texture
					cImage::BindTexture( iNextTexture );
					iCurrentTexture = iNextTexture;
					
					// shader
					if ( m_pCurrentShader != pNextShader && pNextShader ) 
					{
						pNextShader->MakeActive();

						int locPos = pNextShader->GetAttribPosition();
						int locColor = pNextShader->GetAttribColor();
						int locTex = pNextShader->GetAttribTexCoord();

						if ( locPos >= 0 ) pNextShader->SetAttribFloat( locPos, 3, 0, pVertices );
						if ( locColor >= 0 ) pNextShader->SetAttribUByte( locColor, 4, 0, true, pColor );
						if ( locTex >= 0 ) pNextShader->SetAttribFloat( locTex, 2, 0, pUV );
					}
					
					m_pCurrentShader = pNextShader;

					if ( m_pCurrentShader->HasPerSpriteUniforms() )
					{
						m_pCurrentShader->SetTempConstantByName( "agk_spritepos", pSprite->GetXByOffset(), pSprite->GetYByOffset(), 0, 0 );
						m_pCurrentShader->SetTempConstantByName( "agk_spritesize", pSprite->GetWidth(), pSprite->GetHeight(), 0, 0 );
					}

					// transparency
					agk::PlatformSetBlendMode( pMember->GetDrawMode() );
					lastTransparency = pMember->GetDrawMode();

					// scissor
					iLastScissorX = iScissorX;
					iLastScissorY = iScissorY;
					iLastScissorWidth = iScissorWidth;
					iLastScissorHeight = iScissorHeight;
				}
				
				if ( iScissorX != 0 || iScissorY != 0 || iScissorWidth != 0 || iScissorHeight != 0 )
				{
					agk::PlatformScissor( iScissorX, iScissorY, iScissorWidth, iScissorHeight );
				}
				else
				{
					agk::ResetScissor();
				}
				pMember->GetSprite()->BatchDrawQuad( pVertices + iCount*12, pUV + iCount*8, pColor + iCount*16 );
				iCount++;
				break;
			}

			default: break;
		}

		lastType = pMember->GetType();
	}

	m_iLastDrawn += iCount;
	m_iLastDrawCalls++;
	//if ( iCount > 0 ) PlatformDrawTriangles( iCount );
	if ( iCount > 0 && m_pCurrentShader ) m_pCurrentShader->DrawIndices( iCount*6, pIndices );

	agk::ResetScissor();
}

void cSpriteMgrEx::DrawDebug( )
{
	cSpriteContainer *pMember = m_pSprites;
	while ( pMember )
	{
		if ( pMember->GetType() == 1 ) 
		{
			// if shape set but not physics enabled
			if ( pMember->GetSprite()->GetVisible() && pMember->GetSprite()->m_phyShape && !pMember->GetSprite()->m_phyBody )
			{
				switch( pMember->GetSprite()->m_phyShape->m_type )
				{
					case b2Shape::e_circle: 
					{
						b2CircleShape *pShape = (b2CircleShape*)(pMember->GetSprite()->m_phyShape);
						b2Vec2 pos( agk::WorldToPhyX(pMember->GetSprite()->m_fX), agk::WorldToPhyY(pMember->GetSprite()->m_fY) );

						b2Transform transform;
						transform.Set( pos, pMember->GetSprite()->GetAngleRad() );

						b2Vec2 center = b2Mul( transform, pShape->m_p );

						g_DebugDraw.DrawCircle( center, pShape->m_radius, b2Color(0.4f,0.77f,1) );
						break;
					} 

					case b2Shape::e_polygon: 
					{
						b2PolygonShape *pShape = (b2PolygonShape*)(pMember->GetSprite()->m_phyShape);
						b2Vec2 pos( agk::WorldToPhyX(pMember->GetSprite()->m_fX), agk::WorldToPhyY(pMember->GetSprite()->m_fY) );

						b2Transform transform;
						transform.Set( pos, pMember->GetSprite()->GetAngleRad() );

						b2Vec2 newVertices[ b2_maxPolygonVertices ];
						for ( int i = 0; i < pShape->m_vertexCount; i++ ) 
						{
							newVertices[ i ] = b2Mul( transform, pShape->m_vertices[ i ] );
						}
						
						g_DebugDraw.DrawPolygon( newVertices, pShape->m_vertexCount, b2Color(0.4f,0.77f,1) );
						break;
					}
                    default: agk::Error( "Unsupported Box2D shape" );
				}
			}
		}
		pMember = pMember->m_pNext;
	}
}

