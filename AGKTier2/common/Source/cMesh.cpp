#include "agk.h"
#include <include/assimp/cimport.h>
#include <include/assimp/Importer.hpp>
#include <include/assimp/postprocess.h>
#include <include/assimp/scene.h>
#include <include/assimp/cfileio.h>

using namespace AGK;


cMesh::cMesh( cObject3D *pParent )
{
	for (int i = 0; i < AGK_MAX_TEXTURES; i++ ) 
	{
		m_pImages[ i ] = 0;
		m_fUVOffsetU[ i ] = 0;
		m_fUVOffsetV[ i ] = 0;
		m_fUVScaleU[ i ] = 1;
		m_fUVScaleV[ i ] = 1;
	}
	m_fNormalScaleU = 1;
	m_fNormalScaleV = 1;

	m_pMaterial = 0;
	m_pObject = pParent;
	m_pShader = 0;
	m_pOrigShader = 0;

	m_iNumVSLights = 0;
	m_iNumPSLights = 0;

	m_pSharedVertices = 0;

	m_iFlags = AGK_MESH_VISIBLE | AGK_MESH_COLLISION; //PE: default visible with collision.

	m_iNumArrays = 0;
	m_iVertexStride = 0;
	m_iNumVertices = 0;
	m_ppVBOVertexData = 0;
	m_iNumIndices = 0;
	m_ppIndices = 0;
	m_iVBOVertices = 0;
	m_iVBOIndices = 0;

	m_iPosAttrib = -1;
	m_iNormAttrib = -1;
	m_iUVAttrib = -1;
	m_iUV1Attrib = -1;
	m_iTangentAttrib = -1;
	m_iBiNormAttrib = -1;
	m_iColorAttrib = -1;
	m_pVertexAttribs = 0;
	m_iNumAttribs = 0;
	m_pDummyAttribs = 0;
	m_iNumAttribVertices = 0;

	m_pRawIndices = 0;
	m_iNumRawIndices = 0;

	m_fScaledBy = 1.0f;
	m_iPrimitiveType = AGK_TRIANGLES;
}

cMesh::cMesh( cObject3D *pParent, cMesh *pCopyFrom, int share )
{
	m_BoundingBox.copy( &(pCopyFrom->m_BoundingBox) );

	for (int i = 0; i < AGK_MAX_TEXTURES; i++ ) 
	{
		m_pImages[ i ] = pCopyFrom->m_pImages[ i ];
		m_fUVOffsetU[ i ] = pCopyFrom->m_fUVOffsetU[ i ];
		m_fUVOffsetV[ i ] = pCopyFrom->m_fUVOffsetV[ i ];
		m_fUVScaleU[ i ] = pCopyFrom->m_fUVScaleU[ i ];
		m_fUVScaleV[ i ] = pCopyFrom->m_fUVScaleV[ i ];
	}
	m_fNormalScaleU = pCopyFrom->m_fNormalScaleU;
	m_fNormalScaleV = pCopyFrom->m_fNormalScaleV;

	m_pMaterial = pCopyFrom->m_pMaterial;
	m_pObject = pParent;
	m_pShader = 0;
	m_pOrigShader = 0;
	SetShader( pCopyFrom->m_pOrigShader );

	m_iNumVSLights = 0;
	m_iNumPSLights = 0;
		
	m_iFlags = pCopyFrom->m_iFlags;

	if ( share == 1 && !pCopyFrom->m_pSharedVertices ) m_pSharedVertices = pCopyFrom;
	else m_pSharedVertices = pCopyFrom->m_pSharedVertices;

	m_iNumArrays = share == 1 ? 0 : pCopyFrom->m_iNumArrays;
	m_iVertexStride = share == 1 ? 0 : pCopyFrom->m_iVertexStride;
	m_iNumVertices = 0;
	m_iNumIndices = 0;
	m_ppIndices = 0;
	m_ppVBOVertexData = 0;
	m_iVBOVertices = 0;
	m_iVBOIndices = 0;

	/*
	// call ProcessVertexData instead
	if ( m_iNumArrays > 0 )
	{
		m_iNumVertices = new UINT[ m_iNumArrays ];
		m_iNumIndices = pCopyFrom->m_iNumIndices ? (new UINT[ m_iNumArrays ]) : 0;
		m_ppIndices = pCopyFrom->m_ppIndices ? (new unsigned short*[ m_iNumArrays ]) : 0;
		m_ppVBOVertexData = new float*[ m_iNumArrays ];
		
		for ( UINT i = 0; i < m_iNumArrays; i++ )
		{
			m_iNumVertices[ i ] = pCopyFrom->m_iNumVertices[ i ];
			if ( pCopyFrom->m_iNumIndices ) 
			{
				m_iNumIndices[ i ] = pCopyFrom->m_iNumIndices[ i ];

				if ( m_iFlags & AGK_MESH_UINT_INDICES )
				{
					m_ppIndices[ i ] = (unsigned short*) (new UINT[ m_iNumIndices[ i ] ]);
					for ( UINT j = 0; j < m_iNumIndices[ i ]; j++ ) ((UINT**)m_ppIndices)[ i ][ j ] = ((UINT**)pCopyFrom->m_ppIndices)[ i ][ j ];
				}
				else
				{
					m_ppIndices[ i ] = new unsigned short[ m_iNumIndices[ i ] ];
					for ( UINT j = 0; j < m_iNumIndices[ i ]; j++ ) m_ppIndices[ i ][ j ] = pCopyFrom->m_ppIndices[ i ][ j ];
				}
			}

			m_ppVBOVertexData[ i ] = new float[ m_iNumVertices[ i ]*(m_iVertexStride/4) ];
			for ( UINT j = 0; j < m_iNumVertices[ i ]*(m_iVertexStride/4); j++ ) m_ppVBOVertexData[ i ][ j ] = pCopyFrom->m_ppVBOVertexData[ i ][ j ];
		}

		PlatformGenBuffers();
	}
	*/

	m_iNumAttribs = share == 1 ? 0 : pCopyFrom->m_iNumAttribs;
	m_iPosAttrib = share == 1 ? -1 : pCopyFrom->m_iPosAttrib;
	m_iNormAttrib = share == 1 ? -1 : pCopyFrom->m_iNormAttrib;
	m_iUVAttrib = share == 1 ? -1 : pCopyFrom->m_iUVAttrib;
	m_iUV1Attrib = share == 1 ? -1 : pCopyFrom->m_iUV1Attrib;
	m_iTangentAttrib = share == 1 ? -1 : pCopyFrom->m_iTangentAttrib;
	m_iBiNormAttrib = share == 1 ? -1 : pCopyFrom->m_iBiNormAttrib;
	m_iColorAttrib = share == 1 ? -1 : pCopyFrom->m_iColorAttrib;
	m_pVertexAttribs = 0;
	m_pDummyAttribs = 0;
	m_iNumAttribVertices = share == 1 ? 0 : pCopyFrom->m_iNumAttribVertices;

	m_iNumRawIndices = share == 1 ? 0 : pCopyFrom->m_iNumRawIndices;
	m_pRawIndices = 0;
	if ( m_iNumRawIndices > 0 )
	{
		m_pRawIndices = new UINT[ m_iNumRawIndices ];
		for ( UINT i = 0; i < m_iNumRawIndices; i++ )
		{
			m_pRawIndices[ i ] = pCopyFrom->m_pRawIndices[ i ];
		}
	}

	m_fScaledBy = pCopyFrom->m_fScaledBy;
	m_iPrimitiveType = pCopyFrom->m_iPrimitiveType;

	if ( m_iNumAttribs > 0 )
	{
		m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];
		for ( int i = 0; i < m_iNumAttribs; i++ )
		{
			m_pVertexAttribs[ i ] = new cVertexAttrib( pCopyFrom->m_pVertexAttribs[ i ], m_iNumAttribVertices );
		}

		ProcessVertexData();
	}
}

cMesh::~cMesh()
{
	if ( m_iNumVertices ) delete [] m_iNumVertices;
	if ( m_iNumIndices ) delete [] m_iNumIndices;
	for ( UINT i = 0; i < m_iNumArrays; i++ ) 
	{
		if ( m_ppIndices && m_ppIndices[ i ] ) 
		{
			if ( m_iFlags & AGK_MESH_UINT_INDICES ) delete [] ((UINT*)m_ppIndices[ i ]);
			else delete [] m_ppIndices[ i ];
		}
		if ( m_ppVBOVertexData && m_ppVBOVertexData[ i ] ) delete [] m_ppVBOVertexData[ i ];
		if ( m_iVBOVertices && m_iVBOVertices[ i ] ) agk::PlatformDeleteBuffer( m_iVBOVertices[ i ] ); 
		if ( m_iVBOIndices && m_iVBOIndices[ i ] ) agk::PlatformDeleteBuffer( m_iVBOIndices[ i ] ); 
	}
	if ( m_ppIndices ) delete [] m_ppIndices;
	if ( m_ppVBOVertexData ) delete [] m_ppVBOVertexData;
	if ( m_iVBOVertices ) delete [] m_iVBOVertices;
	if ( m_iVBOIndices ) delete [] m_iVBOIndices;

	ClearAttribs();
	ClearRawVertexData();
}

void cMesh::ClearAttribs()
{
	if ( m_pVertexAttribs )
	{
		for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
		{
			if ( m_pVertexAttribs[ i ] ) delete m_pVertexAttribs[ i ];
		}

		delete [] m_pVertexAttribs;
	}

	while ( m_pDummyAttribs )
	{
		cDummyAttrib *pAttrib = m_pDummyAttribs;
		m_pDummyAttribs = m_pDummyAttribs->m_pNextAttrib;
		delete pAttrib;
	}

	m_pVertexAttribs = 0;
	m_iPosAttrib = -1;
	m_iNormAttrib = -1;
	m_iUVAttrib = -1;
	m_iUV1Attrib = -1;
	m_iTangentAttrib = -1;
	m_iBiNormAttrib = -1;
	m_iColorAttrib = -1;
	m_iNumAttribs = 0;
	m_iFlags &= ~AGK_MESH_HAS_BONES;
	m_iFlags |= AGK_MESH_VISIBLE; //PE: Default visible.
	m_iFlags |= AGK_MESH_COLLISION; //PE: Default collision.

	
}

void cMesh::ClearRawVertexData()
{
	for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
	{
		if ( m_pVertexAttribs[ i ] ) m_pVertexAttribs[ i ]->ClearData();
	}

	if ( m_pRawIndices ) delete [] m_pRawIndices;
	m_pRawIndices = 0;
}

void cMesh::CreateDummyAttributesForShader( AGKShader *pShader )
{
	if ( !pShader ) return;
	if ( m_pSharedVertices ) return;

	for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
	{
		int loc = pShader->GetAttribByName( m_pVertexAttribs[ i ]->m_sName );
		m_pVertexAttribs[ i ]->m_iShaderLoc = loc;
	}

	while ( m_pDummyAttribs )
	{
		cDummyAttrib *pAttrib = m_pDummyAttribs;
		m_pDummyAttribs = m_pDummyAttribs->m_pNextAttrib;
		delete pAttrib;
	}

	bool bWarned = false;
	cShaderAttrib *pAttrib = pShader->GetFirstAttribute();
	while ( pAttrib )
	{
		if ( pAttrib->m_iLocation >= 0 )
		{
			bool bFound = false;
			for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
			{
				if ( m_pVertexAttribs[ i ]->m_iShaderLoc == pAttrib->m_iLocation ) 
				{
					bFound = true;
					break;
				}
			}

			if ( !bFound )
			{
				if ( !bWarned )
				{
					uString info;
					info.Format( "Shader \"%s\" requires vertex attributes that object %d does not provide, this shader may fail to display the object", pShader->GetVSFilename(), m_pObject->m_iID );
					agk::Warning( info );
				}

				// create a dummy vertex array just so the object will display something
				cDummyAttrib *pNewAttrib = new cDummyAttrib();
				pNewAttrib->m_pNextAttrib = m_pDummyAttribs;
				m_pDummyAttribs = pNewAttrib;

				UINT size = m_iNumAttribVertices;
				if ( size > 65536 && (m_iFlags & AGK_MESH_UINT_INDICES) == 0 ) size = 65536;
				pNewAttrib->m_iShaderLoc = pAttrib->m_iLocation;
				pNewAttrib->m_pData = new unsigned char[ size*4 ];
				pNewAttrib->m_sName.SetStr( pAttrib->m_sName );
				for( UINT i = 0; i < size; i++ )
				{
					pNewAttrib->m_pData[ i*4 + 0 ] = 0;
					pNewAttrib->m_pData[ i*4 + 1 ] = 1;
					pNewAttrib->m_pData[ i*4 + 2 ] = 0;
					pNewAttrib->m_pData[ i*4 + 3 ] = 1;
				}
			}
		}

		pAttrib = pShader->GetNextAttribute();
	}
}

void cMesh::DeleteGLData()
{
	// delete
	if ( m_iVBOVertices )
	{
		for ( UINT i = 0; i < m_iNumArrays; i++ )
		{
			agk::PlatformDeleteBuffer( m_iVBOVertices[ i ] );
		}
		delete [] m_iVBOVertices;
		m_iVBOVertices = 0;
	}

	if ( m_iVBOIndices )
	{
		for ( UINT i = 0; i < m_iNumArrays; i++ )
		{
			agk::PlatformDeleteBuffer( m_iVBOIndices[ i ] );
		}
		delete [] m_iVBOIndices;
		m_iVBOIndices = 0;
	}
}

void cMesh::ReloadGLData()
{
	if ( m_pSharedVertices ) return;
	if ( !m_ppVBOVertexData ) ProcessVertexData();
	else PlatformGenBuffers();
}

void cMesh::DeleteLocalVBOData()
{
	for ( UINT i = 0; i < m_iNumArrays; i++ ) 
	{
		if ( m_ppIndices && m_ppIndices[ i ] ) 
		{
			if ( m_iFlags & AGK_MESH_UINT_INDICES ) delete [] ((UINT*)m_ppIndices[ i ]);
			else delete [] m_ppIndices[ i ];
		}
		if ( m_ppVBOVertexData && m_ppVBOVertexData[ i ] ) delete [] m_ppVBOVertexData[ i ];
	}
	if ( m_ppIndices ) delete [] m_ppIndices;
	if ( m_ppVBOVertexData ) delete [] m_ppVBOVertexData;
	
	m_ppIndices = 0;
	m_ppVBOVertexData = 0;
}

// This function receives a single list of vertices and an optional list of indices.
// It must split the vertex list into batches of no more than 65536 vertices and 
// create new index lists (if any) to match.
// When this is finished the data passed in will have been copied and should be deleted
// vertexStride is the number of bytes per vertex, must be a multiple of 4.
void cMesh::CreateVBOLists( float *pVertices, UINT numVertices, UINT vertexStride, UINT *pIndices, UINT numIndices, int keepBuffers )
{
	if ( vertexStride % 4 != 0 )
	{
		agk::Error( "vertex stride must be a multiple of 4" );
		return;
	}

	// triangle lists only
	if ( numIndices % 3 != 0 && m_iPrimitiveType == AGK_TRIANGLES )
	{
		agk::Error( "number of indices must be a multiple of 3" );
		return;
	}

	if ( numVertices < 3 )
	{
		agk::Error( "vertex array must contain at least 3 vertices" );
		return;
	}
	
	if ( !pIndices ) numIndices = 0;
	m_iVertexStride = vertexStride;

	// convert bytes to floats
	vertexStride = vertexStride / 4;

	// clear out any old data
	if ( keepBuffers == 0 )
	{
		if ( m_iNumVertices ) delete [] m_iNumVertices;
		if ( m_iNumIndices ) delete [] m_iNumIndices;
		for ( UINT i = 0; i < m_iNumArrays; i++ ) 
		{
			if ( m_ppIndices && m_ppIndices[ i ] )
			{
				if ( m_iFlags & AGK_MESH_UINT_INDICES ) delete [] (UINT*)(m_ppIndices[ i ]);
				else delete [] m_ppIndices[ i ];
			}
			if ( m_ppVBOVertexData && m_ppVBOVertexData[ i ] ) delete [] m_ppVBOVertexData[ i ];
			if ( m_iVBOVertices && m_iVBOVertices[ i ] ) agk::PlatformDeleteBuffer( m_iVBOVertices[ i ] ); 
			if ( m_iVBOIndices && m_iVBOIndices[ i ] ) agk::PlatformDeleteBuffer( m_iVBOIndices[ i ] ); 
		}
		if ( m_ppIndices ) delete [] m_ppIndices;
		if ( m_ppVBOVertexData ) delete [] m_ppVBOVertexData;
		if ( m_iVBOVertices ) delete [] m_iVBOVertices;
		if ( m_iVBOIndices ) delete [] m_iVBOIndices;

		m_iNumArrays = 0;
		m_iNumVertices = 0;
		m_iNumIndices = 0;
		m_ppIndices = 0;
		m_ppVBOVertexData = 0;
		m_iVBOVertices = 0;
		m_iVBOIndices = 0;
	}

	m_iFlags &= ~AGK_MESH_UINT_INDICES;

	// if no indices then just split the vertices
	if ( numIndices == 0 )
	{
		if ( keepBuffers == 0 )
		{
			m_iNumArrays = 1;
			m_iNumVertices = new UINT[ m_iNumArrays ]; 
			m_iNumVertices[ 0 ] = numVertices;
			m_iNumIndices = 0;
			m_ppIndices = 0;
			m_ppVBOVertexData = new float*[ m_iNumArrays ];
			m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
		}
		else 
		{
			// this might have been deleted
			if ( !m_ppVBOVertexData )
			{
				m_ppVBOVertexData = new float*[ m_iNumArrays ];
				m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
			}
		}

		for ( UINT i = 0; i < numVertices*vertexStride; i++ ) m_ppVBOVertexData[ 0 ][ i ] = pVertices[ i ];
	}
	else
	{
		// if there are less than 65536 vertices or device supports large buffers then don't split
		if ( numVertices <= 65536 )
		{
			if ( keepBuffers == 0 )
			{
				m_iNumArrays = 1;
				m_iNumVertices = new UINT[ 1 ]; m_iNumVertices[ 0 ] = numVertices;
				m_iNumIndices = new UINT[ 1 ]; m_iNumIndices[ 0 ] = numIndices;
				m_ppIndices = new unsigned short*[ 1 ];
				m_ppVBOVertexData = new float*[ 1 ];

				m_ppIndices[ 0 ] = new unsigned short[ numIndices ];
				m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
			}
			else 
			{
				// this might have been deleted
				if ( !m_ppVBOVertexData )
				{
					m_ppVBOVertexData = new float*[ 1 ];
					m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
				}

				if ( !m_ppIndices )
				{
					m_ppIndices = new unsigned short*[ 1 ];
					m_ppIndices[ 0 ] = new unsigned short[ numIndices ];
				}
			}

			for ( UINT i = 0; i < numIndices; i++ ) m_ppIndices[ 0 ][ i ] = pIndices[ i ];
			for ( UINT i = 0; i < numVertices*vertexStride; i++ ) m_ppVBOVertexData[ 0 ][ i ] = pVertices[ i ];
		}
		else if ( agk::CanUseIntIndices() )
		{
			if ( keepBuffers == 0 )
			{
				m_iNumArrays = 1;
				m_iNumVertices = new UINT[ 1 ]; m_iNumVertices[ 0 ] = numVertices;
				m_iNumIndices = new UINT[ 1 ]; m_iNumIndices[ 0 ] = numIndices;
				m_ppIndices = new unsigned short*[ 1 ];
				m_ppVBOVertexData = new float*[ 1 ];

				m_ppIndices[ 0 ] = (unsigned short*) (new UINT[ numIndices ]);
				m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
			}
			else 
			{
				// this might have been deleted
				if ( !m_ppVBOVertexData )
				{
					m_ppVBOVertexData = new float*[ 1 ];
					m_ppVBOVertexData[ 0 ] = new float[ numVertices*vertexStride ];
				}

				if ( !m_ppIndices )
				{
					m_ppIndices = new unsigned short*[ 1 ];
					m_ppIndices[ 0 ] = (unsigned short*) (new UINT[ numIndices ]);
				}
			}

			for ( UINT i = 0; i < numIndices; i++ ) ((UINT**)(m_ppIndices))[ 0 ][ i ] = pIndices[ i ];
			for ( UINT i = 0; i < numVertices*vertexStride; i++ ) m_ppVBOVertexData[ 0 ][ i ] = pVertices[ i ];

			m_iFlags |= AGK_MESH_UINT_INDICES;
		}
		else
		{
			if ( m_iPrimitiveType != AGK_TRIANGLES && m_iPrimitiveType != AGK_TRIANGLE_STRIP )
			{
				agk::Error( "AGK needs to split the vertex buffer into chunks of 65535, but the the mesh uses an unsupported primitive type. The mesh will not draw on this device" );
				return;
			}

			if ( keepBuffers == 0 )
			{
				// start with 1 array
				m_iNumArrays = 1;
				m_iNumVertices = new UINT[ 1 ]; m_iNumVertices[ 0 ] = 0;
				m_iNumIndices = new UINT[ 1 ]; m_iNumIndices[ 0 ] = 0;
				UINT *iIndicesArraySize = new UINT[ 1 ]; iIndicesArraySize[ 0 ] = 16384;
				m_ppIndices = new unsigned short*[ 1 ];
				m_ppVBOVertexData = new float*[ 1 ];

				m_ppIndices[ 0 ] = new unsigned short[ 16384 ];
				m_ppVBOVertexData[ 0 ] = new float[ 65536*vertexStride ];

				int *pReassignedArray = new int[ numVertices ];
				for ( UINT i = 0; i < numVertices; i++ ) pReassignedArray[ i ] = -1;
				int lastVertex = -1;
				int lastVertex2 = -1;

				for ( UINT i = 0; i < numIndices; i++ )
				{
					UINT v1 = pIndices[ i ];
					UINT v2 = 0;
					UINT v3 = 0;
					
					int add = 0;
					if ( pReassignedArray[ v1 ] == -1 ) add++;

					if ( m_iPrimitiveType == AGK_TRIANGLES )
					{
						v2 = pIndices[ i+1 ];
						v3 = pIndices[ i+2 ];
						i += 2;

						if ( pReassignedArray[ v2 ] == -1 ) add++;
						if ( pReassignedArray[ v3 ] == -1 ) add++;
					}
					else if ( i % 2 == 0 )
					{
						// triangle strip
						// if index is even then we need to make sure there is enough room for 2 triangles to avoid next buffer having an inverted cull mode
						add++;
					}

					if ( add > 0 )
					{
						// add vertex to current vertex buffer
						if ( m_iNumVertices[ m_iNumArrays-1 ] + add > 65536 )
						{
							// vertex array full, start a new one
							m_iNumArrays++;
							UINT *newNumVertices = new UINT[ m_iNumArrays ]; newNumVertices[ m_iNumArrays-1 ] = 0;
							UINT *newNumIndices = new UINT[ m_iNumArrays ]; newNumIndices[ m_iNumArrays-1 ] = 0;
							UINT *newIndicesArraySize = new UINT[ m_iNumArrays ]; newIndicesArraySize[ m_iNumArrays-1 ] = 16384;
							unsigned short **newIndices = new unsigned short*[ m_iNumArrays ];
							float **newVBOVertexData = new float*[ m_iNumArrays ];
							
							newIndices[ m_iNumArrays-1 ] = new unsigned short[ 16384 ]; // increase this in chunks of 16384
							newVBOVertexData[ m_iNumArrays-1 ] = new float[ 65536*vertexStride ]; // max 65536 vertices per buffer
							
							for ( int n = 0; n < m_iNumArrays-1; n++ )
							{
								newNumVertices[ n ] = m_iNumVertices[ n ];
								newNumIndices[ n ] = m_iNumIndices[ n ];
								newIndicesArraySize[ n ] = iIndicesArraySize[ n ];
								newIndices[ n ] = m_ppIndices[ n ];
								newVBOVertexData[ n ] = m_ppVBOVertexData[ n ];
							}
							
							delete [] m_iNumVertices;
							delete [] m_iNumIndices;
							delete [] iIndicesArraySize;
							delete [] m_ppIndices;
							delete [] m_ppVBOVertexData;
							
							m_iNumVertices = newNumVertices;
							m_iNumIndices = newNumIndices;
							iIndicesArraySize = newIndicesArraySize;
							m_ppIndices = newIndices;
							m_ppVBOVertexData = newVBOVertexData;

							// no vertex currently assigned to the new array
							for ( UINT j = 0; j < numVertices; j++ ) pReassignedArray[ j ] = -1;
							
							if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP )
							{
								// assign the last two vertices to the new array, for triangle strip continuity
								m_iNumVertices[ m_iNumArrays-1 ] = 2;
								for ( UINT v = 0; v < vertexStride; v++ ) 
								{
									m_ppVBOVertexData[ m_iNumArrays-1 ][ v ] = pVertices[ lastVertex2*vertexStride + v ];
									m_ppVBOVertexData[ m_iNumArrays-1 ][ vertexStride + v ] = pVertices[ lastVertex*vertexStride + v ];
								}
								pReassignedArray[ lastVertex2 ] = 0;
								pReassignedArray[ lastVertex ] = 1;

								m_ppIndices[ m_iNumArrays-1 ][ 0 ] = 0;
								m_ppIndices[ m_iNumArrays-1 ][ 1 ] = 1;
								m_iNumIndices[ m_iNumArrays-1 ] = 2;
							}
						}

						// copy vertex into current buffer
						UINT indexV = m_iNumVertices[ m_iNumArrays-1 ];
						if ( pReassignedArray[ v1 ] == -1 )
						{
							for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v1*vertexStride + v ];
							pReassignedArray[ v1 ] = indexV;
							indexV++;
						}

						if ( m_iPrimitiveType == AGK_TRIANGLES )
						{
							if ( pReassignedArray[ v2 ] == -1 )
							{
								for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v2*vertexStride + v ];
								pReassignedArray[ v2 ] = indexV;
								indexV++;
							}

							if ( pReassignedArray[ v3 ] == -1 )
							{
								for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v3*vertexStride + v ];
								pReassignedArray[ v3 ] = indexV;
								indexV++;
							}
						}

						m_iNumVertices[ m_iNumArrays-1 ] = indexV;
					}

					lastVertex2 = lastVertex;
					lastVertex = v1;

					add = 1;
					if ( m_iPrimitiveType == AGK_TRIANGLES ) add = 3;
					if ( m_iNumIndices[ m_iNumArrays-1 ] + add > iIndicesArraySize[ m_iNumArrays-1 ] )
					{
						// increase the size of the indices array
						unsigned short *newArray = new unsigned short[ iIndicesArraySize[ m_iNumArrays-1 ] + 16384 ];
						for ( UINT n = 0; n < iIndicesArraySize[ m_iNumArrays-1 ]; n++ ) newArray[ n ] = m_ppIndices[ m_iNumArrays-1 ][ n ];
						delete [] m_ppIndices[ m_iNumArrays-1 ];
						m_ppIndices[ m_iNumArrays-1 ] = newArray;
						iIndicesArraySize[ m_iNumArrays-1 ] += 16384;
					}

					UINT index2 = m_iNumIndices[ m_iNumArrays-1 ];
					m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v1 ];
					if ( m_iPrimitiveType == AGK_TRIANGLES )
					{
						m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v2 ];
						m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v3 ];
					}
					m_iNumIndices[ m_iNumArrays-1 ] = index2;
				}

				delete [] iIndicesArraySize;
				delete [] pReassignedArray;

				// tidy up oversized arrays
				for ( UINT i = 0; i < m_iNumArrays; i++ ) 
				{
					unsigned short *newIndices = new unsigned short[ m_iNumIndices[ i ] ];
					for ( UINT j = 0; j < m_iNumIndices[ i ]; j++ ) newIndices[ j ] = m_ppIndices[ i ][ j ];
					delete [] m_ppIndices[ i ];
					m_ppIndices[ i ] = newIndices;

					// if it is pretty close to full then no need to spend time reducing it
					if ( m_iNumVertices[ i ] < 65000 )
					{
						float *newVertices = new float[ m_iNumVertices[ i ]*vertexStride ];
						for ( UINT j = 0; j < m_iNumVertices[ i ]*vertexStride; j++ ) newVertices[ j ] = m_ppVBOVertexData[ i ][ j ];
						delete [] m_ppVBOVertexData[ i ];
						m_ppVBOVertexData[ i ] = newVertices;
					}
				}
			}
			else
			{
				// keeping buffers, check if vertex data was deleted
				if ( !m_ppVBOVertexData )
				{
					m_ppVBOVertexData = new float*[ m_iNumArrays ];
					for ( int i = 0; i < m_iNumArrays; i++ ) 
					{
						m_ppVBOVertexData[ i ] = new float[ m_iNumVertices[i]*vertexStride ];
					}
				}

				if ( !m_ppIndices )
				{
					m_ppIndices = new unsigned short*[ m_iNumArrays ];
					for ( int i = 0; i < m_iNumArrays; i++ ) 
					{
						m_ppIndices[ i ] = new unsigned short[ m_iNumIndices[i] ];
					}
				}

				// start fresh with teh same arrays
				m_iNumVertices[ 0 ] = 0;
				m_iNumIndices[ 0 ] = 0;
				m_iNumArrays = 1;

				int *pReassignedArray = new int[ numVertices ];
				for ( UINT i = 0; i < numVertices; i++ ) pReassignedArray[ i ] = -1;
				int lastVertex = -1;
				int lastVertex2 = -1;

				for ( UINT i = 0; i < numIndices; i++ )
				{
					UINT v1 = pIndices[ i ];
					UINT v2 = 0;
					UINT v3 = 0;
					
					int add = 0;
					if ( pReassignedArray[ v1 ] == -1 ) add++;

					if ( m_iPrimitiveType == AGK_TRIANGLES )
					{
						v2 = pIndices[ i+1 ];
						v3 = pIndices[ i+2 ];
						i += 2;

						if ( pReassignedArray[ v2 ] == -1 ) add++;
						if ( pReassignedArray[ v3 ] == -1 ) add++;
					}
					else if ( i % 2 == 0 )
					{
						// triangle strip
						// if index is even then we need to make sure there is enough room for 2 triangles to avoid next buffer having an inverted cull mode
						add++;
					}

					if ( add > 0 )
					{
						// add vertex to current vertex buffer
						if ( m_iNumVertices[ m_iNumArrays-1 ] + add > 65536 )
						{
							// vertex array full, start a new one
							m_iNumArrays++;
							m_iNumVertices[ m_iNumArrays-1 ] = 0;
							m_iNumIndices[ m_iNumArrays-1 ] = 0;

							// no vertex currently assigned to the new array
							for ( UINT j = 0; j < numVertices; j++ ) pReassignedArray[ j ] = -1;
							
							if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP )
							{
								// assign the last two vertices to the new array, for triangle strip continuity
								m_iNumVertices[ m_iNumArrays-1 ] = 2;
								UINT lastSize = m_iNumVertices[ m_iNumArrays-2 ];
								for ( UINT v = 0; v < vertexStride; v++ ) 
								{
									m_ppVBOVertexData[ m_iNumArrays-1 ][ v ] = pVertices[ lastVertex2*vertexStride + v ];
									m_ppVBOVertexData[ m_iNumArrays-1 ][ vertexStride + v ] = pVertices[ lastVertex*vertexStride + v ];
								}
								pReassignedArray[ lastVertex2 ] = 0;
								pReassignedArray[ lastVertex ] = 1;

								m_ppIndices[ m_iNumArrays-1 ][ 0 ] = lastVertex2;
								m_ppIndices[ m_iNumArrays-1 ][ 1 ] = lastVertex;
								m_iNumIndices[ m_iNumArrays-1 ] = 2;
							}
						}

						// copy vertex into current buffer
						UINT indexV = m_iNumVertices[ m_iNumArrays-1 ];
						if ( pReassignedArray[ v1 ] == -1 )
						{
							for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v1*vertexStride + v ];
							pReassignedArray[ v1 ] = indexV;
							indexV++;
						}

						if ( m_iPrimitiveType == AGK_TRIANGLES )
						{
							if ( pReassignedArray[ v2 ] == -1 )
							{
								for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v2*vertexStride + v ];
								pReassignedArray[ v2 ] = indexV;
								indexV++;
							}

							if ( pReassignedArray[ v3 ] == -1 )
							{
								for ( UINT v = 0; v < vertexStride; v++ ) m_ppVBOVertexData[ m_iNumArrays-1 ][ indexV*vertexStride + v ] = pVertices[ v3*vertexStride + v ];
								pReassignedArray[ v3 ] = indexV;
								indexV++;
							}
						}

						m_iNumVertices[ m_iNumArrays-1 ] = indexV;
					}

					lastVertex2 = lastVertex;
					lastVertex = v1;

					UINT index2 = m_iNumIndices[ m_iNumArrays-1 ];
					m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v1 ];
					if ( m_iPrimitiveType == AGK_TRIANGLES )
					{
						m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v2 ];
						m_ppIndices[ m_iNumArrays-1 ][ index2++ ] = pReassignedArray[ v3 ];
					}
					m_iNumIndices[ m_iNumArrays-1 ] = index2;
				}

				delete [] pReassignedArray;
			}
		}
	}
}

void cMesh::ProcessVertexData( int keepBuffers )
{
	UINT stride = 0;
	for ( int i = 0; i < m_iNumAttribs; i++ ) 
	{
		if ( m_pVertexAttribs[ i ]->m_iType == 0 ) stride += m_pVertexAttribs[ i ]->m_iComponents;
		else if ( m_pVertexAttribs[ i ]->m_iType == 1 ) stride += 1;

		if ( m_iPosAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 ) m_iPosAttrib = i;
		if ( m_iNormAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "normal" ) == 0 ) m_iNormAttrib = i;
		if ( m_iUVAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "uv" ) == 0 ) m_iUVAttrib = i;
		if ( m_iUV1Attrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "uv1" ) == 0 ) m_iUV1Attrib = i;
		if ( m_iTangentAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "tangent" ) == 0 ) m_iTangentAttrib = i;
		if ( m_iBiNormAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "binormal" ) == 0 ) m_iBiNormAttrib = i;
		if ( m_iColorAttrib < 0 && m_pVertexAttribs[ i ]->m_sName.CompareTo( "color" ) == 0 ) m_iColorAttrib = i;
	}

	float minX=1000000000, minY=1000000000, minZ=1000000000;
	float maxX=-1000000000, maxY=-1000000000, maxZ=-1000000000;
	m_fRadius = 0;

	float* pData = new float[ m_iNumAttribVertices*stride ];
	float* pVertexDataF = 0;
	unsigned char* pVertexDataUB = 0;
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		UINT offset = 0;
		for ( int a = 0; a < m_iNumAttribs; a++ ) 
		{
			m_pVertexAttribs[ a ]->m_iOffset = offset*4;
			UINT stride2 = m_pVertexAttribs[ a ]->m_iComponents;
			if ( m_pVertexAttribs[ a ]->m_iType == 0 ) 
			{
				pVertexDataF = (float*) (m_pVertexAttribs[ a ]->m_pData);
				for ( UINT c = 0; c < stride2; c++ ) 
				{
					pData[ i*stride + (offset++) ] = pVertexDataF[ i*stride2 + c ];
				}

				// calculate bounding box
				if ( a == m_iPosAttrib )
				{
					float x = pVertexDataF[ i*stride2 + 0 ];
					float y = pVertexDataF[ i*stride2 + 1 ];
					float z = pVertexDataF[ i*stride2 + 2 ];

					if ( x < minX ) minX = x;
					if ( x > maxX ) maxX = x;
					
					if ( y < minY ) minY = y;
					if ( y > maxY ) maxY = y;

					if ( z < minZ ) minZ = z;
					if ( z > maxZ ) maxZ = z;

					float length = x*x + y*y + z*z;
					if ( length > m_fRadius ) m_fRadius = length;
				}
			}
			else if ( m_pVertexAttribs[ a ]->m_iType == 1 ) 
			{
				pVertexDataUB = (unsigned char*) (m_pVertexAttribs[ a ]->m_pData);
				UINT value = pVertexDataUB[ i*stride2 ];
				if ( stride2 > 1 ) value |= (pVertexDataUB[ i*stride2 + 1 ] << 8);
				if ( stride2 > 2 ) value |= (pVertexDataUB[ i*stride2 + 2 ] << 16);
				if ( stride2 > 3 ) value |= (pVertexDataUB[ i*stride2 + 3 ] << 24);

				((UINT*)pData)[ i*stride + (offset++) ] = value;
			}
		}
	}

	m_BoundingBox.set( minX, minY, minZ, maxX, maxY, maxZ );
	m_fRadius = agk::Sqrt( m_fRadius );

	CreateVBOLists( pData, m_iNumAttribVertices, stride*4, m_pRawIndices, m_iNumRawIndices, keepBuffers );
	delete [] pData;

	PlatformGenBuffers();
	DeleteLocalVBOData();
}

bool cMesh::HasValidBones() const 
{ 
	return HasBones() && (m_pObject != 0) && (m_pObject->GetSkeleton() != 0); 
}

int cMesh::GetNumBones() const 
{ 
	return m_pObject->GetSkeleton()->GetBoneCount(); 
}

bool cMesh::HasNormals() const
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iNormAttrib >= 0; 
	else return m_iNormAttrib >= 0; 
}

bool cMesh::HasTangents() const
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iTangentAttrib >= 0; 
	else return m_iTangentAttrib >= 0; 
}

bool cMesh::HasBiNormals() const
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iBiNormAttrib >= 0; 
	else return m_iBiNormAttrib >= 0; 
}

bool cMesh::HasVertColors() const
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iColorAttrib >= 0; 
	else return m_iColorAttrib >= 0; 
}

bool cMesh::HasUVs() const
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iUVAttrib >= 0; 
	else return m_iUVAttrib >= 0; 
}

bool cMesh::HasUV1s() const 
{ 
	if ( m_pSharedVertices ) return m_pSharedVertices->m_iUV1Attrib >= 0; 
	else return m_iUV1Attrib >= 0; 
}

bool cMesh::HasUVStage( int stage ) const
{
	if ( stage == 0 && HasUVs() ) return true;
	if ( stage == 1 && HasUV1s() ) return true;
	return false;
}

int cMesh::HasTextureStage(int stage) const 
{
	if ( stage < 0 || stage >= AGK_MAX_TEXTURES ) return 0;
	return m_pImages[stage] ? 1 : 0; 
}

int cMesh::WantsLighting() const 
{ 
	return m_pObject->GetLightMode() ? 1 : 0; 
}

int cMesh::WantsFog() const 
{ 
	return m_pObject->GetFogMode() ? 1 : 0; 
}

int cMesh::WantsShadows() const 
{ 
	if ( m_pObject->GetLightMode() == 0 ) return 0;
	if ( m_pObject->GetShadowReceiveMode() == 0 ) return 0;

	return 1;
}

//PE: 25-10-2018 , to be used to quickly do LOD by disabling enabled a mesh quickly.
void cMesh::SetVisible(UINT mode)
{
	if (mode > 0) m_iFlags |= AGK_MESH_VISIBLE;
	else m_iFlags &= ~AGK_MESH_VISIBLE;
}

void cMesh::SetCollision(UINT mode)
{
	if (mode > 0) m_iFlags |= AGK_MESH_COLLISION;
	else m_iFlags &= ~AGK_MESH_COLLISION;
}

UINT cMesh::GetInScreen() 
{ 
	if ( !m_pObject ) return 0;

	cCamera *pCamera = agk::GetCurrentCamera();
	if ( !pCamera ) return 0;

	if ( HasValidBones() ) 
	{
		if ( !m_pObject->GetSkeleton() ) return 1; // if the mesh has bones but the object doesn't then assume it is visible

		AGKVector normal[6];
		AGKVector origin[6];
		for ( int i = 0; i < 6; i++ )
		{
			float dist;
			pCamera->GetFrustumPlane( i, normal[i], dist );
			origin[i] = normal[i];
			origin[i].Mult( -dist );
		}

		for ( int b = 0; b < m_pObject->GetSkeleton()->GetBoneCount(); b++ )
		{
			Bone3D *pBone = m_pObject->GetSkeleton()->GetBone(b);
			AGKQuaternion q = pBone->rotFinal();
			q.Invert();

			int inside = 1;
			for ( int i = 0; i < 6; i++ )
			{
				AGKVector n2 = q * normal[i];
				n2 = n2 * pBone->scaleFinal();

				AGKVector origin2 = origin[i] - pBone->posFinal();
				origin2 = q * origin2;
				origin2 = origin2 / pBone->scaleFinal();

				float dist = -origin2.Dot( n2 );

				if ( !pBone->m_BoundingBox.inFrustumPlane( &n2, dist ) )
				{
					inside = 0;
					break;
				}
			}

			if ( inside == 1 )
			{
				return 1;
			}
		}
	}
	else
	{
		// not boned
		AGKVector n;
		float d;
		int inside = 1;
		for ( int i = 0; i < 6; i++ )
		{
			pCamera->GetFrustumPlane( i, n, d );

			AGKVector origin = n;
			origin.Mult( -d );

			AGKQuaternion q = m_pObject->rotFinal();
			q.Invert();
			n = q * n;
			n = n * m_pObject->scaleFinal();

			origin = origin - m_pObject->posFinal();
			origin = q * origin;
			origin = origin / m_pObject->scaleFinal();

			d = -origin.Dot( n );

			if ( !InFrustumPlane( &n, d ) ) 
			{
				inside = 0;
				break;
			}
		}

		if ( inside == 1 )
		{
			return 1;
		}
	}
	
	return 0;
}

UINT cMesh::GetInShadowFrustum() 
{ 
	if ( !m_pObject ) return 0;
	if ( m_iNumAttribVertices < 20 ) return 1; // probably quicker to draw it even if it isn't visible

	AGKMatrix4 *pShadowMat = AGKShader::GetShadowMatrix();

	if ( HasValidBones() ) 
	{
		if ( !m_pObject->GetSkeleton() ) return 1; // if the mesh has bones but the object doesn't then assume it is visible

		for ( int b = 0; b < m_pObject->GetSkeleton()->GetBoneCount(); b++ )
		{
			Bone3D *pBone = m_pObject->GetSkeleton()->GetBone(b);
			AGKMatrix4 boneMat;
			boneMat.MakeWorld( pBone->rotFinal(), pBone->posFinal(), pBone->scaleFinal() );
			boneMat.Mult( *pShadowMat );
			
			if ( pBone->m_BoundingBox.inFrustum( &boneMat ) ) return 1;
		}
	}
	else
	{
		// not boned
		AGKMatrix4 objMat;
		objMat.MakeWorld( m_pObject->rotFinal(), m_pObject->posFinal(), m_pObject->scaleFinal() );
		objMat.Mult( *pShadowMat );

		if ( m_BoundingBox.inFrustum( &objMat ) ) return 1;
	}

	return 0;
}

void cMesh::CreateBox( float width, float height, float length )
{
	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
		
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	float *pRawVertices = new float[ 24 * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float *pRawNormals = new float[ 24 * 3 ];	pAttribNorm->m_pData = (void*) pRawNormals;
	float *pRawUV = new float[ 24 * 2 ];		pAttribTex->m_pData = (void*) pRawUV;
	m_pRawIndices = new UINT[ 36 ];

	m_iNumAttribVertices = 24;
	m_iNumRawIndices = 36;

	cVertex *pVertices = (cVertex*) pRawVertices;

	width /= 2;
	height /= 2;
	length /= 2;

	int vertexIndex = 0;
	for ( int i = 0; i < 6; i++ )
	{
		m_pRawIndices[ i*6 ] = vertexIndex;
		m_pRawIndices[ i*6 + 1 ] = ++vertexIndex;
		m_pRawIndices[ i*6 + 2 ] = ++vertexIndex;
		m_pRawIndices[ i*6 + 3 ] = vertexIndex;
		m_pRawIndices[ i*6 + 4 ] = vertexIndex-1;
		m_pRawIndices[ i*6 + 5 ] = ++vertexIndex;
		vertexIndex++;
	}

	// Face 1
	pVertices[ 0 ].x = -width;
	pVertices[ 0 ].y = height;
	pVertices[ 0 ].z = -length;

	pVertices[ 1 ].x = -width;
	pVertices[ 1 ].y = -height;
	pVertices[ 1 ].z = -length;

	pVertices[ 2 ].x = width;
	pVertices[ 2 ].y = height;
	pVertices[ 2 ].z = -length;

	pVertices[ 3 ].x = width;
	pVertices[ 3 ].y = -height;
	pVertices[ 3 ].z = -length;

	// Face 2
	pVertices[ 4 ].x = width;
	pVertices[ 4 ].y = height;
	pVertices[ 4 ].z = -length;

	pVertices[ 5 ].x = width;
	pVertices[ 5 ].y = -height;
	pVertices[ 5 ].z = -length;

	pVertices[ 6 ].x = width;
	pVertices[ 6 ].y = height;
	pVertices[ 6 ].z = length;

	pVertices[ 7 ].x = width;
	pVertices[ 7 ].y = -height;
	pVertices[ 7 ].z = length;

	// Face 3
	pVertices[ 8 ].x = -width;
	pVertices[ 8 ].y = height;
	pVertices[ 8 ].z = length;

	pVertices[ 9 ].x = -width;
	pVertices[ 9 ].y = height;
	pVertices[ 9 ].z = -length;

	pVertices[ 10 ].x = width;
	pVertices[ 10 ].y = height;
	pVertices[ 10 ].z = length;

	pVertices[ 11 ].x = width;
	pVertices[ 11 ].y = height;
	pVertices[ 11 ].z = -length;

	// Faces 4-5
	for ( int i = 0; i < 8; i++ )
	{
		pVertices[ i+12 ].x = -pVertices[ i ].x;
		pVertices[ i+12 ].y = pVertices[ i ].y;
		pVertices[ i+12 ].z = -pVertices[ i ].z;
	}

	// Face 6
	for ( int i = 8; i < 12; i++ )
	{
		pVertices[ i+12 ].x = pVertices[ i ].x;
		pVertices[ i+12 ].y = -pVertices[ i ].y;
		pVertices[ i+12 ].z = -pVertices[ i ].z;
	}

	// Normal 1
	for ( int i = 0; i < 4; i++ )
	{
		pRawNormals[ i*3 ] = 0;
		pRawNormals[ i*3 + 1 ] = 0;
		pRawNormals[ i*3 + 2 ] = -1;
	}

	// Normal 2
	for ( int i = 4; i < 8; i++ )
	{
		pRawNormals[ i*3 ] = 1;
		pRawNormals[ i*3 + 1 ] = 0;
		pRawNormals[ i*3 + 2 ] = 0;
	}

	// Normal 3
	for ( int i = 8; i < 12; i++ )
	{
		pRawNormals[ i*3 ] = 0;
		pRawNormals[ i*3 + 1 ] = 1;
		pRawNormals[ i*3 + 2 ] = 0;
	}

	// Normals 4-6
	for ( int i = 12; i < 24; i++ )
	{
		pRawNormals[ i*3 ] = -pRawNormals[ (i-12)*3 ];
		pRawNormals[ i*3 + 1 ] = -pRawNormals[ (i-12)*3 + 1 ];
		pRawNormals[ i*3 + 2 ] = -pRawNormals[ (i-12)*3 + 2 ];
	}

	// UVs 1-6
	for ( int i = 0; i < 6; i++ )
	{
		pRawUV[ i*8 + 0 ] = 0.0f;
		pRawUV[ i*8 + 1 ] = 0.0f;

		pRawUV[ i*8 + 2 ] = 0.0f;
		pRawUV[ i*8 + 3 ] = 1.0f;

		pRawUV[ i*8 + 4 ] = 1.0f;
		pRawUV[ i*8 + 5 ] = 0.0f;

		pRawUV[ i*8 + 6 ] = 1.0f;
		pRawUV[ i*8 + 7 ] = 1.0f;
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, m_pRawIndices, m_iNumRawIndices );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

// indexed sphere
void cMesh::CreateSphere( float diameter, int rows, int columns )
{
	if ( rows < 2 ) rows = 2;
	if ( columns < 3 ) columns = 3;

	float radius = diameter / 2.0f;

	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	int faces = (rows-1) * columns * 2;
	m_iNumAttribVertices = (rows+1) * (columns+1); 
	m_iNumRawIndices = faces*3;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;
	m_pRawIndices = new UINT[ m_iNumRawIndices ];

	cVertex *pVertices = (cVertex*) pRawVertices;
//	cVertex *pNormals = (cVertex*) pRawNormals;
	
	float fSegY = PI / rows;
	float fSegX = 2*PI / columns;

	float fSegU = 1.0f / columns;
	float fSegV = 1.0f / rows;

	// vertices
	UINT count = 0;
	for ( int j = 0; j <= rows; j++ )
	{
		float fSY = agk::SinRad( fSegY*j );
		float fCY = agk::CosRad( fSegY*j );

		for ( int i = 0; i <= columns; i++ )
		{
			pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY * radius;
			pVertices[ count ].y = fCY * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY * radius;
			if ( j == 0 || j == rows ) pRawUV[ count*2 ] = fSegU*i + fSegU/2.0f;
			else pRawUV[ count*2 ] = fSegU*i;
			pRawUV[ count*2 + 1 ] = fSegV*j;
			count++;
		}
	}

	// normals
	for ( UINT i = 0 ; i < m_iNumAttribVertices*3; i++ )
	{
		pRawNormals[ i ] = pRawVertices[ i ] / radius;
	}

	// indices
	UINT countI = 0;
	// top row
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;

		m_pRawIndices[ countI*3 + 0 ] = i;
		m_pRawIndices[ countI*3 + 1 ] = columns+1 + i;
		m_pRawIndices[ countI*3 + 2 ] = columns+1 + next;
		countI++;
	}

	// middle rows
	for ( int j = 1; j < rows-1; j++ )
	{
		for ( int i = 0; i < columns; i++ )
		{
			int next = i+1;

			m_pRawIndices[ countI*3 + 0 ] = (columns+1)*j + i;
			m_pRawIndices[ countI*3 + 1 ] = (columns+1)*(j+1) + i;
			m_pRawIndices[ countI*3 + 2 ] = (columns+1)*j + next;
			countI++;

			m_pRawIndices[ countI*3 + 0 ] = (columns+1)*j + next;
			m_pRawIndices[ countI*3 + 1 ] = (columns+1)*(j+1) + i;
			m_pRawIndices[ countI*3 + 2 ] = (columns+1)*(j+1) + next;
			countI++;
		}
	}

	// bottom row
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;

		m_pRawIndices[ countI*3 + 0 ] = (columns+1)*(rows-1) + i;
		m_pRawIndices[ countI*3 + 1 ] = (columns+1)*rows + i;
		m_pRawIndices[ countI*3 + 2 ] = (columns+1)*(rows-1) + next;
		countI++;
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, m_pRawIndices, m_iNumRawIndices );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

// perspective adjusted UV sphere
/*
void cMesh::CreateSphere2( float diameter, int rows, int columns )
{
	if ( rows < 2 ) rows = 2;
	if ( columns < 3 ) columns = 3;
	//if ( diameter < 0 ) diameter = 0;

	float radius = diameter / 2.0f;

	int faces = (rows-1) * columns * 2;
	m_iNumVertices = faces * 3; // could use indices and shared vertices to save space here
	m_iNumIndices = 0;
	
	if ( m_pVertices ) delete [] m_pVertices;
	if ( m_pNormals ) delete [] m_pNormals;
	if ( m_pUV ) delete [] m_pUV;
	if ( m_pIndices ) delete [] m_pIndices;

	m_pVertices = new float[ m_iNumVertices * 3 ];
	m_pNormals = new float[ m_iNumVertices * 3 ];
	m_pUV = new float[ m_iNumVertices * 3 ];
	m_pIndices = 0;

	cVertex *pVertices = (cVertex*) m_pVertices;
	cVertex *pNormals = (cVertex*) m_pNormals;
	
	float fSegY = PI / rows;
	float fSegX = 2*PI / columns;

	float fSegU = 1.0f / columns;
	float fSegV = 1.0f / rows;

	UINT count = 0;
	float fSY = agk::SinRad( fSegY );
	float fCY = agk::CosRad( fSegY );

	// top row, one polygon per segment
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;
		if ( next >= columns ) next = 0;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = radius;
		pVertices[ count ].z = 0;
		m_pUV[ count*3 ] = fSegU*i + fSegU/2.0f;
		m_pUV[ count*3 + 1 ] = 0;
		m_pUV[ count*3 + 2 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY * radius;
		pVertices[ count ].y = fCY * radius;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY * radius;
		m_pUV[ count*3 ] = fSegU*i;
		m_pUV[ count*3 + 1 ] = fSegV;
		m_pUV[ count*3 + 2 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * fSY * radius;
		pVertices[ count ].y = fCY * radius;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * fSY * radius;
		m_pUV[ count*3 ] = fSegU*(i+1);
		m_pUV[ count*3 + 1 ] = fSegV;
		m_pUV[ count*3 + 2 ] = 1;
		count++;
	}

	// middle rows, teo polygons per segment
	for ( int j = 1; j < rows-1; j++ )
	{
		fSY = agk::SinRad( fSegY*j );
		fCY = agk::CosRad( fSegY*j );

		float fSY2 = agk::SinRad( fSegY*(j+1) );
		float fCY2 = agk::CosRad( fSegY*(j+1) );

		for ( int i = 0; i < columns; i++ )
		{
			int next = i+1;
			if ( next >= columns ) next = 0;

			float x = fSY * radius * (agk::SinRad( -fSegX*i ) - agk::SinRad( -fSegX*next ));
			float z = fSY * radius * (agk::CosRad( -fSegX*i ) - agk::CosRad( -fSegX*next ));
			float distTop = agk::Sqrt(x*x + z*z);

			x = fSY2 * radius * (agk::SinRad( -fSegX*i ) - agk::SinRad( -fSegX*next ));
			z = fSY2 * radius * (agk::CosRad( -fSegX*i ) - agk::CosRad( -fSegX*next ));
			float distBottom = agk::Sqrt(x*x + z*z);

			float ratioTop = distTop/distBottom + 1;
			float ratioBottom = distBottom/distTop + 1;

			// polygon 1
			pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY * radius;
			pVertices[ count ].y = fCY * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY * radius;
			m_pUV[ count*3 ] = fSegU*i * ratioTop;
			m_pUV[ count*3 + 1 ] = fSegV*j * ratioTop;
			m_pUV[ count*3 + 2 ] = ratioTop;
			count++;

			pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY2 * radius;
			pVertices[ count ].y = fCY2 * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY2 * radius;
			m_pUV[ count*3 ] = fSegU*i * ratioBottom;
			m_pUV[ count*3 + 1 ] = fSegV*(j+1) * ratioBottom;
			m_pUV[ count*3 + 2 ] = ratioBottom;
			count++;

			pVertices[ count ].x = agk::SinRad( -fSegX*next ) * fSY * radius;
			pVertices[ count ].y = fCY * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*next ) * fSY * radius;
			m_pUV[ count*3 ] = fSegU*(i+1) * ratioTop;
			m_pUV[ count*3 + 1 ] = fSegV*j * ratioTop;
			m_pUV[ count*3 + 2 ] = ratioTop;
			count++;

			// polygon 2
			pVertices[ count ].x = agk::SinRad( -fSegX*next ) * fSY * radius;
			pVertices[ count ].y = fCY * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*next ) * fSY * radius;
			m_pUV[ count*3 ] = fSegU*(i+1) * ratioTop;
			m_pUV[ count*3 + 1 ] = fSegV*j * ratioTop;
			m_pUV[ count*3 + 2 ] = ratioTop;
			count++;

			pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY2 * radius;
			pVertices[ count ].y = fCY2 * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY2 * radius;
			m_pUV[ count*3 ] = fSegU*i * ratioBottom;
			m_pUV[ count*3 + 1 ] = fSegV*(j+1) * ratioBottom;
			m_pUV[ count*3 + 2 ] = ratioBottom;
			count++;

			pVertices[ count ].x = agk::SinRad( -fSegX*next ) * fSY2 * radius;
			pVertices[ count ].y = fCY2 * radius;
			pVertices[ count ].z = agk::CosRad( -fSegX*next ) * fSY2 * radius;
			m_pUV[ count*3 ] = fSegU*(i+1) * ratioBottom;
			m_pUV[ count*3 + 1 ] = fSegV*(j+1) * ratioBottom;
			m_pUV[ count*3 + 2 ] = ratioBottom;
			count++;
		}
	}

	fSY = agk::SinRad( PI-fSegY );
	fCY = agk::CosRad( PI-fSegY );

	// bottom row, one polygon per segment
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;
		if ( next >= columns ) next = 0;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY * radius;
		pVertices[ count ].y = fCY * radius;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY * radius;
		m_pUV[ count*3 ] = fSegU*i;
		m_pUV[ count*3 + 1 ] = 1-fSegV;
		m_pUV[ count*3 + 2 ] = 1;
		count++;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = -radius;
		pVertices[ count ].z = 0;
		m_pUV[ count*3 ] = fSegU*i + fSegU/2.0f;
		m_pUV[ count*3 + 1 ] = 1;
		m_pUV[ count*3 + 2 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * fSY * radius;
		pVertices[ count ].y = fCY * radius;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * fSY * radius;
		m_pUV[ count*3 ] = fSegU*(i+1);
		m_pUV[ count*3 + 1 ] = 1-fSegV;
		m_pUV[ count*3 + 2 ] = 1;
		count++;		
	}

	for ( UINT i = 0 ; i < m_iNumVertices*3; i++ )
	{
		m_pNormals[ i ] = m_pVertices[ i ] / radius;
	}

	PlatformGenBuffers();
}
*/

void cMesh::CreateCone( float height, float diameter, int segments )
{
	if ( segments < 3 ) segments = 3;
	if ( diameter < 0 ) diameter = -diameter;

	float radius = diameter / 2.0f;

	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	int faces = segments*2;
	m_iNumAttribVertices = faces * 3; 
	m_iNumRawIndices = 0;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;
	
	cVertex *pVertices = (cVertex*) pRawVertices;
	cVertex *pNormals = (cVertex*) pRawNormals;
	
	float fSegX = 2*PI / segments;
//	float fSegU = 1.0f / segments;

	UINT count = 0;
	
	// cone part
	for ( int i = 0; i < segments; i++ )
	{
		int next = i+1;
		if ( next >= segments ) next = 0;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = 0;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = 1;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = 0.5f;
		pRawUV[ count*2 + 1 ] = 0.5f;
		//m_pUV[ count*2 ] = fSegU*i + fSegU/2.0f;
		//m_pUV[ count*2 + 1 ] = 0;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*i );
		pNormals[ count ].y = agk::Abs(radius)/height;
		pNormals[ count ].z = agk::CosRad( -fSegX*i );
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*i )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*i )/2.0f + 0.5f;
		//pRawUV[ count*2 ] = fSegU*i;
		//pRawUV[ count*2 + 1 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*next );
		pNormals[ count ].y = agk::Abs(radius)/height;
		pNormals[ count ].z = agk::CosRad( -fSegX*next );
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*(i+1) )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*(i+1) )/2.0f + 0.5f;
		//pRawUV[ count*2 ] = fSegU*(i+1);
		//pRawUV[ count*2 + 1 ] = 1;
		count++;
	}

	// base, just make a flat cone
	for ( int i = 0; i < segments; i++ )
	{
		int next = i+1;
		if ( next >= segments ) next = 0;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -height;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*i )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*i )/2.0f + 0.5f;
		count++;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = 0;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -height;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = 0.5f;
		pRawUV[ count*2 + 1 ] = 0.5f;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -height;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*(i+1) )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*(i+1) )/2.0f + 0.5f;
		count++;		
	}

	// normalise normals
	for ( UINT i = 0 ; i < m_iNumAttribVertices; i++ )
	{
		float length = agk::Sqrt( pNormals[ i ].x*pNormals[ i ].x + pNormals[ i ].y*pNormals[ i ].y + pNormals[ i ].z*pNormals[ i ].z );
		pNormals[ i ].x = pNormals[ i ].x / length;
		pNormals[ i ].y = pNormals[ i ].y / length;
		pNormals[ i ].z = pNormals[ i ].z / length;
	}

	// if inside out, flip normals
	if ( height < 0 )
	{
		for ( UINT i = 0 ; i < m_iNumAttribVertices*3; i++ )
		{
			pRawNormals[ i ] = -pRawNormals[ i ];
		}
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, 0, 0 );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

void cMesh::CreateCylinder( float height, float diameter, int segments )
{
	if ( segments < 3 ) segments = 3;
	if ( diameter < 0 ) diameter = -diameter;

	float radius = diameter / 2.0f;

	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	int faces = segments*4;
	m_iNumAttribVertices = faces * 3; 
	m_iNumRawIndices = 0;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;

	cVertex *pVertices = (cVertex*) pRawVertices;
	cVertex *pNormals = (cVertex*) pRawNormals;
	
	float fSegX = 2*PI / segments;
	float fSegU = 1.0f / segments;

	UINT count = 0;
	float NormY = height>0 ? 1.0f : -1.0f;

	// top, just make a flat cone
	for ( int i = 0; i < segments; i++ )
	{
		int next = i+1;
		if ( next >= segments ) next = 0;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = 0;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = 0.5f;
		pRawUV[ count*2 + 1 ] = 0.5f;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*i )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*i )/2.0f + 0.5f;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*(i+1) )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*(i+1) )/2.0f + 0.5f;
		count++;		
	}
	
	// middle part, two polygons per segment
	for ( int i = 0; i < segments; i++ )
	{
		int next = i+1;
		if ( next >= segments ) next = 0;

		// polygon 1
		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*i );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*i );
		pRawUV[ count*2 ] = fSegU*i;
		pRawUV[ count*2 + 1 ] = 0;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*i );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*i );
		pRawUV[ count*2 ] = fSegU*i;
		pRawUV[ count*2 + 1 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*next );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*next );
		pRawUV[ count*2 ] = fSegU*(i+1);
		pRawUV[ count*2 + 1 ] = 1;
		count++;

		// polygon 2
		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*i );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*i );
		pRawUV[ count*2 ] = fSegU*i;
		pRawUV[ count*2 + 1 ] = 0;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*next );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*next );
		pRawUV[ count*2 ] = fSegU*(i+1);
		pRawUV[ count*2 + 1 ] = 1;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = agk::SinRad( -fSegX*next );
		pNormals[ count ].y = 0;
		pNormals[ count ].z = agk::CosRad( -fSegX*next );
		pRawUV[ count*2 ] = fSegU*(i+1);
		pRawUV[ count*2 + 1 ] = 0;
		count++;
	}

	// base, just make a flat cone
	for ( int i = 0; i < segments; i++ )
	{
		int next = i+1;
		if ( next >= segments ) next = 0;

		pVertices[ count ].x = agk::SinRad( -fSegX*i ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*i ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*i )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*i )/2.0f + 0.5f;
		count++;

		pVertices[ count ].x = 0;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = 0;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = 0.5f;
		pRawUV[ count*2 + 1 ] = 0.5f;
		count++;

		pVertices[ count ].x = agk::SinRad( -fSegX*next ) * radius;
		pVertices[ count ].y = -height/2.0f;
		pVertices[ count ].z = agk::CosRad( -fSegX*next ) * radius;
		pNormals[ count ].x = 0;
		pNormals[ count ].y = -NormY;
		pNormals[ count ].z = 0;
		pRawUV[ count*2 ] = agk::SinRad( -fSegX*(i+1) )/2.0f + 0.5f;
		pRawUV[ count*2 + 1 ] = agk::CosRad( -fSegX*(i+1) )/2.0f + 0.5f;
		count++;		
	}

	// normals already normalised

	// if inside out, flip normals
	if ( height < 0 )
	{
		for ( UINT i = 0 ; i < m_iNumAttribVertices*3; i++ )
		{
			pRawNormals[ i ] = -pRawNormals[ i ];
		}
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, 0, 0 );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

void cMesh::CreatePlane( float width, float height )
{
	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	m_iNumAttribVertices = 12; 
	m_iNumRawIndices = 0;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;

	cVertex *pVertices = (cVertex*) pRawVertices;

	width /= 2;
	height /= 2;

	// Face 1
	pVertices[ 0 ].x = -width;
	pVertices[ 0 ].y = height;
	pVertices[ 0 ].z = 0;

	pVertices[ 1 ].x = -width;
	pVertices[ 1 ].y = -height;
	pVertices[ 1 ].z = 0;

	pVertices[ 2 ].x = width;
	pVertices[ 2 ].y = height;
	pVertices[ 2 ].z = 0;

	pVertices[ 3 ].x = width;
	pVertices[ 3 ].y = height;
	pVertices[ 3 ].z = 0;

	pVertices[ 4 ].x = -width;
	pVertices[ 4 ].y = -height;
	pVertices[ 4 ].z = 0;

	pVertices[ 5 ].x = width;
	pVertices[ 5 ].y = -height;
	pVertices[ 5 ].z = 0;

	// Face 2
	pVertices[ 6 ].x = width;
	pVertices[ 6 ].y = height;
	pVertices[ 6 ].z = 0;

	pVertices[ 7 ].x = width;
	pVertices[ 7 ].y = -height;
	pVertices[ 7 ].z = 0;

	pVertices[ 8 ].x = -width;
	pVertices[ 8 ].y = height;
	pVertices[ 8 ].z = 0;

	pVertices[ 9 ].x = -width;
	pVertices[ 9 ].y = height;
	pVertices[ 9 ].z = 0;

	pVertices[ 10 ].x = width;
	pVertices[ 10 ].y = -height;
	pVertices[ 10 ].z = 0;

	pVertices[ 11 ].x = -width;
	pVertices[ 11 ].y = -height;
	pVertices[ 11 ].z = 0;

	// Normal 1
	for ( int i = 0; i < 6; i++ )
	{
		pRawNormals[ i*3 ] = 0;
		pRawNormals[ i*3 + 1 ] = 0;
		pRawNormals[ i*3 + 2 ] = -1;
	}

	// Normal 2
	for ( int i = 6; i < 12; i++ )
	{
		pRawNormals[ i*3 ] = 0;
		pRawNormals[ i*3 + 1 ] = 0;
		pRawNormals[ i*3 + 2 ] = 1;
	}

	// UVs 1-6
	for ( int i = 0; i < 2; i++ )
	{
		pRawUV[ i*12 + 0 ] = 0.0f;
		pRawUV[ i*12 + 1 ] = 0.0f;

		pRawUV[ i*12 + 2 ] = 0.0f;
		pRawUV[ i*12 + 3 ] = 1.0f;

		pRawUV[ i*12 + 4 ] = 1.0f;
		pRawUV[ i*12 + 5 ] = 0.0f;

		pRawUV[ i*12 + 6 ] = 1.0f;
		pRawUV[ i*12 + 7 ] = 0.0f;

		pRawUV[ i*12 + 8 ] = 0.0f;
		pRawUV[ i*12 + 9 ] = 1.0f;
		
		pRawUV[ i*12 + 10 ] = 1.0f;
		pRawUV[ i*12 + 11 ] = 1.0f;
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;
	
	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, 0, 0 );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

void cMesh::CreateQuad()
{
	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;
	m_iNormAttrib = 1;
	m_iUVAttrib = 2;

	m_iNumAttribVertices = 6; 
	m_iNumRawIndices = 0;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;

	cVertex *pVertices = (cVertex*) pRawVertices;

	float width = 1;
	float height = 1;

	// Face 1
	pVertices[ 0 ].x = -width;
	pVertices[ 0 ].y = height;
	pVertices[ 0 ].z = 0;

	pVertices[ 1 ].x = -width;
	pVertices[ 1 ].y = -height;
	pVertices[ 1 ].z = 0;

	pVertices[ 2 ].x = width;
	pVertices[ 2 ].y = height;
	pVertices[ 2 ].z = 0;

	pVertices[ 3 ].x = width;
	pVertices[ 3 ].y = height;
	pVertices[ 3 ].z = 0;

	pVertices[ 4 ].x = -width;
	pVertices[ 4 ].y = -height;
	pVertices[ 4 ].z = 0;

	pVertices[ 5 ].x = width;
	pVertices[ 5 ].y = -height;
	pVertices[ 5 ].z = 0;

	// Normal 1
	for ( int i = 0; i < 6; i++ )
	{
		pRawNormals[ i*3 ] = 0;
		pRawNormals[ i*3 + 1 ] = 0;
		pRawNormals[ i*3 + 2 ] = -1;
	}

	// UVs 1-6
	pRawUV[ 0 ] = 0.0f;
	pRawUV[ 1 ] = 0.0f;

	pRawUV[ 2 ] = 0.0f;
	pRawUV[ 3 ] = 1.0f;

	pRawUV[ 4 ] = 1.0f;
	pRawUV[ 5 ] = 0.0f;

	pRawUV[ 6 ] = 1.0f;
	pRawUV[ 7 ] = 0.0f;

	pRawUV[ 8 ] = 0.0f;
	pRawUV[ 9 ] = 1.0f;
	
	pRawUV[ 10 ] = 1.0f;
	pRawUV[ 11 ] = 1.0f;

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	ProcessVertexData();

	/*
	float* pData = new float[ m_iNumAttribVertices*8 ];
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		pData[ i*8 + 0 ] = pRawVertices[ i*3 + 0 ];
		pData[ i*8 + 1 ] = pRawVertices[ i*3 + 1 ];
		pData[ i*8 + 2 ] = pRawVertices[ i*3 + 2 ];
		
		pData[ i*8 + 3 ] = pRawNormals[ i*3 + 0 ];
		pData[ i*8 + 4 ] = pRawNormals[ i*3 + 1 ];
		pData[ i*8 + 5 ] = pRawNormals[ i*3 + 2 ];
		
		pData[ i*8 + 6 ] = pRawUV[ i*2 + 0 ];
		pData[ i*8 + 7 ] = pRawUV[ i*2 + 1 ];
	}

	CreateVBOLists( pData, m_iNumAttribVertices, 8*4, 0, 0 );
	delete [] pData;

	PlatformGenBuffers();
	*/
}

void cMesh::CreateCapsule( float diameter, int rows, int columns, float height, int axis )
{
	if ( rows < 2 ) rows = 2;
	if ( columns < 3 ) columns = 3;

	float radius = diameter / 2.0f;

	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribTex = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribTex->m_iComponents = 2;
	pAttribTex->m_iType = 0;
	pAttribTex->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribTex;
	m_iPosAttrib = 0;

	int faces = (rows-1) * columns * 2;
	m_iNumAttribVertices = (rows+1) * (columns+1); 
	m_iNumRawIndices = faces*3;

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribTex->m_pData = (void*) pRawUV;
	m_pRawIndices = new UINT[ m_iNumRawIndices ];

	cVertex *pVertices = (cVertex*) pRawVertices;
//	cVertex *pNormals = (cVertex*) pRawNormals;
	
	float fSegY = PI / rows;
	float fSegX = 2*PI / columns;

	float fSegU = 1.0f / columns;
	float fSegV = 1.0f / rows;

	height = (height - diameter)/2.0f;

	// vertices
	UINT count = 0;
	for ( int j = 0; j <= rows; j++ )
	{
		float fSY = agk::SinRad( fSegY*j );
		float fCY = agk::CosRad( fSegY*j );

		for ( int i = 0; i <= columns; i++ )
		{
			pVertices[ count ].x = agk::SinRad( -fSegX*i ) * fSY * radius;
			if ( fCY * radius > 0 ) pVertices[ count ].y = fCY * radius + height;
			else pVertices[ count ].y = fCY * radius - height;
			pVertices[ count ].z = agk::CosRad( -fSegX*i ) * fSY * radius;

			if ( j == 0 || j == rows ) pRawUV[ count*2 ] = fSegU*i + fSegU/2.0f;
			else pRawUV[ count*2 ] = fSegU*i;
			pRawUV[ count*2 + 1 ] = fSegV*j;
			count++;
		}
	}

	// normals
	for ( UINT i = 0 ; i < m_iNumAttribVertices*3; i++ )
	{
		pRawNormals[ i ] = pRawVertices[ i ] / radius;
	}

	// indices
	UINT countI = 0;
	// top row
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;

		m_pRawIndices[ countI*3 + 0 ] = i;
		m_pRawIndices[ countI*3 + 1 ] = columns+1 + i;
		m_pRawIndices[ countI*3 + 2 ] = columns+1 + next;
		countI++;
	}

	// middle rows
	for ( int j = 1; j < rows-1; j++ )
	{
		for ( int i = 0; i < columns; i++ )
		{
			int next = i+1;

			m_pRawIndices[ countI*3 + 0 ] = (columns+1)*j + i;
			m_pRawIndices[ countI*3 + 1 ] = (columns+1)*(j+1) + i;
			m_pRawIndices[ countI*3 + 2 ] = (columns+1)*j + next;
			countI++;

			m_pRawIndices[ countI*3 + 0 ] = (columns+1)*j + next;
			m_pRawIndices[ countI*3 + 1 ] = (columns+1)*(j+1) + i;
			m_pRawIndices[ countI*3 + 2 ] = (columns+1)*(j+1) + next;
			countI++;
		}
	}

	// bottom row
	for ( int i = 0; i < columns; i++ )
	{
		int next = i+1;

		m_pRawIndices[ countI*3 + 0 ] = (columns+1)*(rows-1) + i;
		m_pRawIndices[ countI*3 + 1 ] = (columns+1)*rows + i;
		m_pRawIndices[ countI*3 + 2 ] = (columns+1)*(rows-1) + next;
		countI++;
	}

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;
	pAttribTex->m_iOffset = 6*4;

	AGKQuaternion rotationQ;
	if( axis == Z_AXIS ) rotationQ.MakeFromEulerYXZ(90.0,0.0,0.0);
	if( axis == X_AXIS ) rotationQ.MakeFromEulerYXZ(0.0,0.0,90.0);

	RotateMesh( rotationQ.w, rotationQ.x, rotationQ.y, rotationQ.z, 0 );
	ProcessVertexData();
}

void cMesh::CreateFromHeightMap( unsigned short *pValues, int totalSegsX, int totalSegsZ, int startX, int endX, int startZ, int endZ, float width, float height, float length )
{
	ClearAttribs();
	ClearRawVertexData();
	m_fScaledBy = 1;

	m_iNumAttribs = 3;
	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pAttribPos = new cVertexAttrib();
	cVertexAttrib *pAttribNorm = new cVertexAttrib();
	cVertexAttrib *pAttribUV = new cVertexAttrib();

	pAttribPos->m_iComponents = 3;
	pAttribPos->m_iType = 0;
	pAttribPos->m_sName.SetStr( "position" );
	
	pAttribNorm->m_iComponents = 3;
	pAttribNorm->m_iType = 0;
	pAttribNorm->m_sName.SetStr( "normal" );

	pAttribUV->m_iComponents = 2;
	pAttribUV->m_iType = 0;
	pAttribUV->m_sName.SetStr( "uv" );

	m_pVertexAttribs[ 0 ] = pAttribPos;
	m_pVertexAttribs[ 1 ] = pAttribNorm;
	m_pVertexAttribs[ 2 ] = pAttribUV;
	m_iPosAttrib = 0;

	// total number of vertices in the main array
	int totalVertsX = totalSegsX + 1;
	int totalVertsZ = totalSegsZ + 1;

	// number of segments to use in this sub array
	int segX = endX - startX;
	int segZ = endZ - startZ;

	// number of vertices in this sub array
	int vertX = segX + 1;
	int vertZ = segZ + 1;

	// segment size, same for both main and sub arrays
	float fSegSizeX = width / totalSegsX;
	float fSegSizeZ = length / totalSegsZ;

	m_iNumAttribVertices = vertX*vertZ; 
	m_iNumRawIndices = (segX*2 + 4)*segZ - 2; // using triangle strip

	float* pRawVertices = new float[ m_iNumAttribVertices * 3 ];	pAttribPos->m_pData = (void*) pRawVertices;
	float* pRawNormals = new float[ m_iNumAttribVertices * 3 ];		pAttribNorm->m_pData = (void*) pRawNormals;
	float* pRawUV = new float[ m_iNumAttribVertices * 2 ];			pAttribUV->m_pData = (void*) pRawUV;
	m_pRawIndices = new UINT[ m_iNumRawIndices ];

	cVertex *pVertices = (cVertex*) pRawVertices;
	cVertex *pNormals = (cVertex*) pRawNormals;

	// vertices
	for ( int z = startZ; z <= endZ; z++ )
	{
		for ( int x = startX; x <= endX; x++ )
		{
			UINT index = (z * totalVertsX) + x;
			UINT subIndex = ((z-startZ)*vertX) + (x-startX);

			pVertices[ subIndex ].x = x*fSegSizeX;
			pVertices[ subIndex ].y = pValues[index]/65535.0f * height;
			pVertices[ subIndex ].z = length - (z*fSegSizeZ);

			pRawUV[ subIndex*2 + 0 ] = (x*fSegSizeX) / width;
			pRawUV[ subIndex*2 + 1 ] = (z*fSegSizeZ) / length;
		}
	}

	// normals
	for ( int z = startZ; z <= endZ; z++ )
	{
		for ( int x = startX; x <= endX; x++ )
		{
			UINT subIndex = ((z-startZ)*vertX) + (x-startX);
			UINT index = (z * totalVertsX) + x;
			UINT indexX1 = index;
			UINT indexX2 = index;
			int numSegsX = 0;
			if ( x > 0 ) { --indexX1; numSegsX++; }
			if ( x < totalVertsX-1 ) { ++indexX2; numSegsX++; }

			float diffX = (float) (pValues[indexX2] - pValues[indexX1]);
			diffX = diffX/65535.0f * height;

			UINT indexZ1 = index;
			UINT indexZ2 = index;
			int numSegsZ = 0;
			if ( z > 0 ) { indexZ1 -= totalVertsX; numSegsZ++; }
			if ( z < totalVertsZ-1 ) { indexZ2 += totalVertsX; numSegsZ++; }

			float diffZ = (float) (pValues[indexZ2] - pValues[indexZ1]);
			diffZ = diffZ/65535.0f * height;

			float nx = -diffX;
			float ny = (numSegsX*fSegSizeX + numSegsZ*fSegSizeZ) / 2.0f;
			float nz = diffZ;

			float length = nx*nx + ny*ny + nz*nz;
			length = 1.0f / agk::Sqrt(length);

			pNormals[ subIndex ].x = nx * length;
			pNormals[ subIndex ].y = ny * length;
			pNormals[ subIndex ].z = nz * length;
		}
	}

	// indices
	UINT countI = 0;
	for ( int z = 0; z < segZ; z++ )
	{
		m_pRawIndices[ countI++ ] = z*vertX;
		m_pRawIndices[ countI++ ] = (z+1)*vertX;

		for ( int x = 0; x < segX; x++ )
		{
			m_pRawIndices[ countI++ ] = z*vertX + x+1;
			m_pRawIndices[ countI++ ] = (z+1)*vertX + x+1;
		}

		if ( z < segZ-1 ) 
		{
			m_pRawIndices[ countI++ ] = (z+1)*vertX + segX;
			m_pRawIndices[ countI++ ] = (z+1)*vertX;
		}
	}

	if ( countI != m_iNumRawIndices ) 
	{
		uString info;
		info.Format( "Num Indices: %d does not match index count: %d", m_iNumRawIndices, countI );
		agk::Warning( info );
	}

	m_iPrimitiveType = AGK_TRIANGLE_STRIP;

	pAttribPos->m_iOffset = 0;
	pAttribNorm->m_iOffset = 3*4;

	ProcessVertexData();
}

void cMesh::CreateMesh( aiMesh *pMesh, float height, int scaleMode )
{
	ClearAttribs();
	ClearRawVertexData();

	float *pV = 0;
	float *pVT0 = 0;
	float *pVT1 = 0;
	float *pVN = 0;
	float *pVTan = 0;
	float *pVBin = 0;
	unsigned char *pVCol = 0;
	float *pVWeights = 0;
	unsigned char *pVIndices = 0;
	unsigned char *pWeightCount = 0;

	m_iNumAttribs = 1;
	m_iNumAttribVertices = pMesh->mNumVertices;
	m_sName.SetStr( pMesh->mName.C_Str() );

	pV = new float[ m_iNumAttribVertices*3 ];

	if ( pMesh->mTextureCoords[0] ) 
	{
		pVT0 = new float[ m_iNumAttribVertices*2 ];
		m_iNumAttribs++;
	}
	if ( pMesh->mTextureCoords[1] ) 
	{
		pVT1 = new float[ m_iNumAttribVertices*2 ];
		m_iNumAttribs++;
	}
	if ( pMesh->mNormals ) 
	{
		pVN = new float[ m_iNumAttribVertices*3 ];
		m_iNumAttribs++;
	}
	if ( pMesh->mTangents ) 
	{
		pVTan = new float[ m_iNumAttribVertices*3 ];
		m_iNumAttribs++;
	}
	if ( pMesh->mBitangents ) 
	{
		pVBin = new float[ m_iNumAttribVertices*3 ];
		m_iNumAttribs++;
	}
	if ( pMesh->mColors[0] ) 
	{
		pVCol = new unsigned char[ m_iNumAttribVertices*4 ];
		m_iNumAttribs++;
	}
	if ( pMesh->HasBones() && m_pObject->m_pSkeleton )
	{
		m_iFlags |= AGK_MESH_HAS_BONES;

		pVWeights = new float[ m_iNumAttribVertices*4 ];
		memset( pVWeights, 0, m_iNumAttribVertices*4*sizeof(float) );
		m_iNumAttribs++;

		pVIndices = new unsigned char[ m_iNumAttribVertices*4 ];
		memset( pVIndices, 0, m_iNumAttribVertices*4 );
		m_iNumAttribs++;
	}

	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pPositions = new cVertexAttrib();
	pPositions->m_iComponents = 3;
	pPositions->m_iType = 0;
	pPositions->m_sName.SetStr( "position" );
	pPositions->m_pData = (void*) pV;
	pPositions->m_iOffset = 0;
	m_pVertexAttribs[ 0 ] = pPositions;
	m_iPosAttrib = 0;
	
	UINT currAttrib = 1;
	UINT currOffset = 3*4;

	if ( pVN )
	{
		m_iNormAttrib = currAttrib;
		cVertexAttrib *pNormals = new cVertexAttrib();
		pNormals->m_iComponents = 3;
		pNormals->m_iType = 0;
		pNormals->m_sName.SetStr( "normal" );
		pNormals->m_pData = (void*) pVN;
		pNormals->m_iOffset = currOffset;
		currOffset += 3*4;
		m_pVertexAttribs[ currAttrib++ ] = pNormals;
	}
	if ( pVTan )
	{
		m_iTangentAttrib = currAttrib;
		cVertexAttrib *pTangents = new cVertexAttrib();
		pTangents->m_iComponents = 3;
		pTangents->m_iType = 0;
		pTangents->m_sName.SetStr( "tangent" );
		pTangents->m_pData = (void*) pVTan;
		pTangents->m_iOffset = currOffset;
		currOffset += 3*4;
		m_pVertexAttribs[ currAttrib++ ] = pTangents;
	}
	if ( pVBin )
	{
		m_iBiNormAttrib = currAttrib;
		cVertexAttrib *pBiNormals = new cVertexAttrib();
		pBiNormals->m_iComponents = 3;
		pBiNormals->m_iType = 0;
		pBiNormals->m_sName.SetStr( "binormal" );
		pBiNormals->m_pData = (void*) pVBin;
		pBiNormals->m_iOffset = currOffset;
		currOffset += 3*4;
		m_pVertexAttribs[ currAttrib++ ] = pBiNormals;
	}

	if ( pVT0 )
	{
		m_iUVAttrib = currAttrib;
		cVertexAttrib *pTexCoords = new cVertexAttrib();
		pTexCoords->m_iComponents = 2;
		pTexCoords->m_iType = 0;
		pTexCoords->m_sName.SetStr( "uv" );
		pTexCoords->m_pData = (void*) pVT0;
		pTexCoords->m_iOffset = currOffset;
		currOffset += 2*4;
		m_pVertexAttribs[ currAttrib++ ] = pTexCoords;
	}
	if ( pVT1 )
	{
		m_iUV1Attrib = currAttrib;
		cVertexAttrib *pTexCoords = new cVertexAttrib();
		pTexCoords->m_iComponents = 2;
		pTexCoords->m_iType = 0;
		pTexCoords->m_sName.SetStr( "uv1" );
		pTexCoords->m_pData = (void*) pVT1;
		pTexCoords->m_iOffset = currOffset;
		currOffset += 2*4;
		m_pVertexAttribs[ currAttrib++ ] = pTexCoords;
	}

	if ( pVCol )
	{
		m_iColorAttrib = currAttrib;
		cVertexAttrib *pColors = new cVertexAttrib();
		pColors->m_iComponents = 4;
		pColors->m_iType = 1;
		pColors->m_sName.SetStr( "color" );
		pColors->m_pData = (void*) pVCol;
		pColors->m_iOffset = currOffset;
		pColors->m_bNormalize = true;
		currOffset += 4;
		m_pVertexAttribs[ currAttrib++ ] = pColors;
	}

	if ( HasBones() )
	{
		cVertexAttrib *pBoneWeights = new cVertexAttrib();
		pBoneWeights->m_iComponents = 4;
		pBoneWeights->m_iType = 0;
		pBoneWeights->m_sName.SetStr( "boneweights" );
		pBoneWeights->m_pData = (void*) pVWeights;
		pBoneWeights->m_iOffset = currOffset;
		currOffset += 4*4;
		m_pVertexAttribs[ currAttrib++ ] = pBoneWeights;

		cVertexAttrib *pBoneIndices = new cVertexAttrib();
		pBoneIndices->m_iComponents = 4;
		pBoneIndices->m_iType = 1;
		pBoneIndices->m_sName.SetStr( "boneindices" );
		pBoneIndices->m_pData = (void*) pVIndices;
		pBoneIndices->m_iOffset = currOffset;
		pBoneIndices->m_bNormalize = false;
		currOffset += 4;
		m_pVertexAttribs[ currAttrib++ ] = pBoneIndices;
	}

	float maxHeight = -1000000000;
	float minHeight = 1000000000;

	// load data into AGK arrays
	for ( UINT i = 0; i < pMesh->mNumVertices; i++ )
	{
		if ( pMesh->mVertices[ i ].y > maxHeight ) maxHeight = pMesh->mVertices[ i ].y;
		if ( pMesh->mVertices[ i ].y < minHeight ) minHeight = pMesh->mVertices[ i ].y;

		pV[ i*3 + 0 ] = pMesh->mVertices[ i ].x;
		pV[ i*3 + 1 ] = pMesh->mVertices[ i ].y;
		pV[ i*3 + 2 ] = pMesh->mVertices[ i ].z;

		if ( pVN )
		{
			float nx = pMesh->mNormals[ i ].x;
			float ny = pMesh->mNormals[ i ].y;
			float nz = pMesh->mNormals[ i ].z;

			float length = sqrt(nx*nx + ny*ny + nz*nz);
			if ( length > 0.00001f ) length = 1 / length;
			else 
			{
				ny = 1;
				length = 1;
			}

			pVN[ i*3 + 0 ] = nx * length;
			pVN[ i*3 + 1 ] = ny * length;
			pVN[ i*3 + 2 ] = nz * length;
		}

		if ( pVTan )
		{
			pVTan[ i*3 + 0 ] = pMesh->mTangents[ i ].x;
			pVTan[ i*3 + 1 ] = pMesh->mTangents[ i ].y;
			pVTan[ i*3 + 2 ] = pMesh->mTangents[ i ].z;
		}

		if ( pVBin )
		{
			pVBin[ i*3 + 0 ] = pMesh->mBitangents[ i ].x;
			pVBin[ i*3 + 1 ] = pMesh->mBitangents[ i ].y;
			pVBin[ i*3 + 2 ] = pMesh->mBitangents[ i ].z;
		}

		if ( pVCol )
		{
			pVCol[ i*4 + 0 ] = (unsigned char)(pMesh->mColors[ 0 ][ i ].r*255);
			pVCol[ i*4 + 1 ] = (unsigned char)(pMesh->mColors[ 0 ][ i ].g*255);
			pVCol[ i*4 + 2 ] = (unsigned char)(pMesh->mColors[ 0 ][ i ].b*255);
			pVCol[ i*4 + 3 ] = (unsigned char)(pMesh->mColors[ 0 ][ i ].a*255);
		}

		if ( pVT0 )
		{
			pVT0[ i*2 + 0 ] = pMesh->mTextureCoords[ 0 ][ i ].x;
			pVT0[ i*2 + 1 ] = pMesh->mTextureCoords[ 0 ][ i ].y;
		}

		if ( pVT1 )
		{
			pVT1[ i*2 + 0 ] = pMesh->mTextureCoords[ 1 ][ i ].x;
			pVT1[ i*2 + 1 ] = pMesh->mTextureCoords[ 1 ][ i ].y;
		}
	}

	if ( HasBones() )
	{
		pWeightCount = new unsigned char[ m_iNumAttribVertices ]; // just to count the weights as we find them
		memset( pWeightCount, 0, m_iNumAttribVertices );

		for( UINT b = 0; b < pMesh->mNumBones; b++ )
		{
			UINT boneIndex = m_pObject->m_pSkeleton->GetBoneIndex( pMesh->mBones[b]->mName.C_Str() );

			for ( UINT w = 0; w < pMesh->mBones[b]->mNumWeights; w++ )
			{
				UINT vertexIndex = pMesh->mBones[b]->mWeights[w].mVertexId;
				if ( pWeightCount[vertexIndex] >= 4 ) 
				{
					static bool warned = false;
					if ( !warned )
					{
						uString err;
						err.Format( "Object %d has more than 4 bone weights per vertex, AGK only supports 4 weights, the rest will be ignored", m_pObject->m_iID );
						agk::Warning( err );
						warned = true;
					}
				}
				else
				{
					pVWeights[ vertexIndex*4 + pWeightCount[vertexIndex] ] = pMesh->mBones[b]->mWeights[w].mWeight;
					pVIndices[ vertexIndex*4 + pWeightCount[vertexIndex] ] = boneIndex;
					pWeightCount[vertexIndex]++;
				}
			}
		}

		delete [] pWeightCount;
	}

	// scale mesh if necessary
	m_fScaledBy = 1;
	if ( scaleMode == 0 )
	{
		// scale to specified height
		if ( height > 0 && maxHeight > minHeight )
		{
			m_fScaledBy = height / (maxHeight-minHeight);
			for ( UINT i = 0; i < pMesh->mNumVertices*3; i++ )
			{
				pV[ i ] *= m_fScaledBy;
			}
		}
	}
	else if ( scaleMode == 1 )
	{
		// use height as a scale factor
		if ( height != 0 )
		{
			m_fScaledBy = height;
			for ( UINT i = 0; i < pMesh->mNumVertices*3; i++ )
			{
				pV[ i ] *= m_fScaledBy;
			}
		}
	}

	// load indices into AGK data
	m_iNumRawIndices = pMesh->mNumFaces*3;
	m_pRawIndices = new UINT[ m_iNumRawIndices ];

	for ( UINT i = 0; i < pMesh->mNumFaces; i++ )
	{
		m_pRawIndices[ i*3 + 0 ] = pMesh->mFaces[ i ].mIndices[ 0 ];
		m_pRawIndices[ i*3 + 1 ] = pMesh->mFaces[ i ].mIndices[ 1 ];
		m_pRawIndices[ i*3 + 2 ] = pMesh->mFaces[ i ].mIndices[ 2 ];
	}

	ProcessVertexData();
}

void cMesh::CreateFromObj( int lines, uString *sLines, float height, const char *szFilename )
{
	ClearAttribs();
	ClearRawVertexData();

	UINT numPositions = 0;
	UINT numNormals = 0;
	UINT numTexCoords = 0;
	bool bTexCoords2 = false;
	UINT numIndices = 0;

	cHashedList<int> cVertexCount;
	uString token;

	// first pass count vertex and face data
	for ( int l = 0; l < lines; l++ )
	{
		uString *pLine = sLines + l;
		pLine->Trim( "\r\n " );

		if ( strncmp( pLine->GetStr(), "v ", 2 ) == 0 )
		{
			numPositions++;
		}
		else if ( strncmp( pLine->GetStr(), "vn ", 3 ) == 0 )
		{
			numNormals++;
		}
		else if ( strncmp( pLine->GetStr(), "vt ", 3 ) == 0 )
		{
			numTexCoords++;
			if ( bTexCoords2==false )
			{
				int count = pLine->CountTokens( " " );
				if ( count>=5 ) bTexCoords2 = true;
			}
		}
		else if ( strncmp( pLine->GetStr(), "f ", 2 ) == 0 )
		{
			int count = pLine->CountTokens( " " );
			if ( count < 4 ) continue;

			// faces can have more than 3 vertices per face, need to split these later so account for the extra
			numIndices += 3;
			if ( count-1 > 3 ) 
			{
				int extra = (count-1) - 3;
				numIndices += extra*3;
			}

			// count unique vertex pairs
			for ( int i = 2; i <= count; i++ )
			{
				pLine->GetToken( " ", i, token );
				if ( token.GetLength() > 0 )
				{
					int* value = cVertexCount.GetItem( token.GetStr() );
					if ( !value ) 
					{
						cVertexCount.AddItem( (int*)1, token.GetStr() );
					}
				}
			}
		}
	}

	// vertices must have position
	if ( numPositions == 0 ) 
	{
		uString info;
		info.Format( "Failed to load object \"%s\", no vertex position data found", szFilename );
		agk::Warning( info );
		delete [] sLines;
		return;
	}

	if ( cVertexCount.GetCount() == 0 ) 
	{
		uString info;
		info.Format( "Failed to load object \"%s\", no polygon data found", szFilename );
		agk::Warning( info );
		delete [] sLines;
		return;
	}

	float *pV = 0;		float *pV2 = 0;
	float *pVT0 = 0;	float *pVT02 = 0;
	float *pVT1 = 0;	float *pVT12 = 0;
	float *pVN = 0;		float *pVN2 = 0;

	m_iNumAttribs = 1;
	m_iNumAttribVertices = cVertexCount.GetCount();

	pV = new float[ numPositions*3 ];
	pV2 = new float[ m_iNumAttribVertices*3 ];

	if ( numTexCoords ) 
	{
		pVT0 = new float[ numTexCoords*2 ];
		pVT02 = new float[ m_iNumAttribVertices*2 ];
		m_iNumAttribs++;
	}
	if ( bTexCoords2==true ) 
	{
		pVT1 = new float[ numTexCoords*2 ];
		pVT12 = new float[ m_iNumAttribVertices*2 ];
		m_iNumAttribs++;
	}
	if ( numNormals ) 
	{
		pVN = new float[ numNormals*3 ];
		pVN2 = new float[ m_iNumAttribVertices*3 ];
		m_iNumAttribs++;
	}

	m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

	cVertexAttrib *pPositions = new cVertexAttrib();
	pPositions->m_iComponents = 3;
	pPositions->m_iType = 0;
	pPositions->m_sName.SetStr( "position" );
	pPositions->m_pData = (void*) pV2;
	pPositions->m_iOffset = 0;
	m_pVertexAttribs[ 0 ] = pPositions;
	m_iPosAttrib = 0;
	
	UINT currAttrib = 1;

	UINT currOffset = 3*4;

	if ( pVN )
	{
		cVertexAttrib *pNormals = new cVertexAttrib();
		pNormals->m_iComponents = 3;
		pNormals->m_iType = 0;
		pNormals->m_sName.SetStr( "normal" );
		pNormals->m_pData = (void*) pVN2;
		pNormals->m_iOffset = currOffset;
		currOffset += 3*4;
		m_pVertexAttribs[ currAttrib++ ] = pNormals;
	}

	if ( pVT0 )
	{
		cVertexAttrib *pTexCoords = new cVertexAttrib();
		pTexCoords->m_iComponents = 2;
		pTexCoords->m_iType = 0;
		pTexCoords->m_sName.SetStr( "uv" );
		pTexCoords->m_pData = (void*) pVT02;
		//pTexCoords->m_iOffset = pVN ? 6*4 : 3*4;
		pTexCoords->m_iOffset = currOffset;
		currOffset += 2*4;
		m_pVertexAttribs[ currAttrib++ ] = pTexCoords;
	}
	if ( pVT1 )
	{
		cVertexAttrib *pTexCoords = new cVertexAttrib();
		pTexCoords->m_iComponents = 2;
		pTexCoords->m_iType = 0;
		pTexCoords->m_sName.SetStr( "uv1" );
		pTexCoords->m_pData = (void*) pVT12;
		pTexCoords->m_iOffset = currOffset;
		currOffset += 2*4;
		m_pVertexAttribs[ currAttrib++ ] = pTexCoords;
	}

	UINT iVCount = 0;
	UINT iVTCount = 0;
	UINT iVNCount = 0;

	float maxHeight = -1000000000;
	float minHeight = 1000000000;
	
	// second pass, load the vertex data
	for ( int l = 0; l < lines; l++ )
	{
		uString *pLine = sLines + l;

		if ( strncmp( pLine->GetStr(), "v ", 2 ) == 0 )
		{
			int count = pLine->CountTokens( " " );
			if ( count > 4 ) count = 4;

			for ( int i = 2; i <= count; i++ )
			{
				pLine->GetToken( " ", i, token );
				if ( token.GetLength() > 0 )
				{
					float value = token.ToFloat();
					pV[ iVCount*3 + (i-2) ] = value;
					if ( i == 3 )
					{
						if ( value > maxHeight ) maxHeight = value;
						if ( value < minHeight ) minHeight = value;
					}
				}
			}
			iVCount++;
		}
		else if ( strncmp( pLine->GetStr(), "vn ", 3 ) == 0 )
		{
			int count = pLine->CountTokens( " " );
			if ( count > 4 ) count = 4;

			for ( int i = 2; i <= count; i++ )
			{
				pLine->GetToken( " ", i, token );
				if ( token.GetLength() > 0 )
				{
					pVN[ iVNCount*3 + (i-2) ] = token.ToFloat();
				}
			}
			iVNCount++;
		}
		else if ( strncmp( pLine->GetStr(), "vt ", 3 ) == 0 )
		{
			int count = pLine->CountTokens( " " );
			if ( count > 5 ) count = 5;
			if ( count == 4 ) count = 3; // must account for 3 uv values used by some exporters

			for ( int i = 2; i <= count; i++ )
			{
				pLine->GetToken( " ", i, token );
				if ( token.GetLength() > 0 )
				{
					if ( i>=4 )
						pVT1[ iVTCount*2 + (i-4) ] = token.ToFloat();
					else
						pVT0[ iVTCount*2 + (i-2) ] = token.ToFloat();
				}
			}
			iVTCount++;
		}
	}

	// adjust model size to match height
	if ( height > 0 && maxHeight > minHeight )
	{
		float scale = height / (maxHeight-minHeight);
		for ( UINT i = 0; i < iVCount*3; i++ )
		{
			pV[ i ] *= scale;
		}
	}

	m_iNumRawIndices = numIndices;
	m_pRawIndices = new UINT[ numIndices ];
		
	UINT numVertices = 0;
	numIndices = 0;

	cVertexCount.ClearAll();

	// third pass load polygon data
	for ( int l = 0; l < lines; l++ )
	{
		uString *pLine = sLines + l;
		
		if ( strncmp( pLine->GetStr(), "f ", 2 ) == 0 )
		{
			int count = pLine->CountTokens( " " );
			if ( count < 4 ) continue;

			int firstIndex = numIndices;

			for ( int i = 2; i <= count; i++ )
			{
				pLine->GetToken( " ", i, token );
				if ( token.GetLength() == 0 ) 
				{
					uString info;
					info.Format( "Failed to load object \"%s\", unexpected blank token in face list", szFilename );
					agk::Warning( info );
					return;
				}

				if ( i > 4 )
				{
					m_pRawIndices[ numIndices ] = m_pRawIndices[ firstIndex ];
					numIndices++;
					m_pRawIndices[ numIndices ] = m_pRawIndices[ numIndices-2  ];
					numIndices++;
				}

				// check for shared vertex that already exists
				UINT_PTR pInt = (UINT_PTR) cVertexCount.GetItem( token.GetStr() );
				if ( pInt ) m_pRawIndices[ numIndices ] = ((UINT)pInt)-1;
				else
				{
					// add new vertex
					uString token2;
					token.GetToken( "/", 1, token2 );
					UINT pos = token2.ToInt()-1;
					if ( pos > iVCount )
					{
						uString info;
						info.Format( "Failed to load object \"%s\", position index out of range", szFilename );
						agk::Warning( info );
						return;
					}

					// position
					pV2[ numVertices*3 + 0 ] = pV[ pos*3 + 0 ];
					pV2[ numVertices*3 + 1 ] = pV[ pos*3 + 1 ];
					pV2[ numVertices*3 + 2 ] = pV[ pos*3 + 2 ] * -1;

					int elements = token.CountTokens( "/" );
					if ( elements > 1 )
					{
						if ( token.FindStr( "//" ) == -1 )
						{
							// texcoord
							token.GetToken( "/", 2, token2 );
							UINT tex = token2.ToInt()-1;
							if ( tex > iVTCount )
							{
								uString info;
								info.Format( "Failed to load object \"%s\", texture coordinate index out of range", szFilename );
								agk::Warning( info );
								return;
							}
							
							pVT02[ numVertices*2 + 0 ] = pVT0[ tex*2 + 0 ];
							pVT02[ numVertices*2 + 1 ] = 1 - pVT0[ tex*2 + 1 ];
							if ( pVT1!=NULL )
							{
								pVT12[ numVertices*2 + 0 ] = pVT1[ tex*2 + 0 ];
								pVT12[ numVertices*2 + 1 ] = 1 - pVT1[ tex*2 + 1 ];
							}

							if ( elements > 2 )
							{
								// normal
								token.GetToken( "/", 3, token2 );
								UINT norm = token2.ToInt()-1;
								if ( norm > iVNCount )
								{
									uString info;
									info.Format( "Failed to load object \"%s\", normal index out of range", szFilename );
									agk::Warning( info );
									return;
								}
								
								float nx = pVN[ norm*3 + 0 ];
								float ny = pVN[ norm*3 + 1 ];
								float nz = pVN[ norm*3 + 2 ] * -1;

								float length = sqrt(nx*nx + ny*ny + nz*nz);
								if ( length > 0.00001f ) length = 1 / length;
								else 
								{
									ny = 1;
									length = 1;
								}

								pVN2[ numVertices*3 + 0 ] = nx * length;
								pVN2[ numVertices*3 + 1 ] = ny * length;
								pVN2[ numVertices*3 + 2 ] = nz * length;
							}
						}
						else
						{
							// normal
							token.GetToken( "/", 2, token2 );
							UINT norm = token2.ToInt()-1;
							if ( norm > iVNCount )
							{
								uString info;
								info.Format( "Failed to load object \"%s\", normal index out of range", szFilename );
								agk::Warning( info );
								return;
							}

							float nx = pVN[ norm*3 + 0 ];
							float ny = pVN[ norm*3 + 1 ];
							float nz = pVN[ norm*3 + 2 ] * -1;

							float length = sqrt(nx*nx + ny*ny + nz*nz);
							if ( length > 0.00001f ) length = 1 / length;
							else 
							{
								ny = 1;
								length = 1;
							}

							pVN2[ numVertices*3 + 0 ] = nx * length;
							pVN2[ numVertices*3 + 1 ] = ny * length;
							pVN2[ numVertices*3 + 2 ] = nz * length;
						}
					}

					m_pRawIndices[ numIndices ] = numVertices;
                    UINT_PTR index = numVertices+1;
					cVertexCount.AddItem( (int*)index, token.GetStr() );
					numVertices++;
				}

				numIndices++;
			} // end face token loop
		}
	}

	if ( pV ) delete [] pV;
	if ( pVN ) delete [] pVN;
	if ( pVT0 ) delete [] pVT0;
	if ( pVT1 ) delete [] pVT1;

	cVertexCount.ClearAll();

	ProcessVertexData();
}

void cMesh::GetVerticesFromMemblock( UINT &memSize, unsigned char **pMemData )
{
	if ( !pMemData ) return;

	UINT size = 6*4; // header
	UINT vertexSize = 0;

	for ( UINT i = 0; i < m_iNumAttribs; ++i )
	{
		size += 4; // four 1 byte values
		UINT length = m_pVertexAttribs[ i ]->m_sName.GetLength(); // ignore the null terminator, it cancels with a -1 in the algorithm
		length /= 4;
		length = (length+1) * 4; // string length rounded up to nearest multiple of 4
		if ( length > 252 ) length = 252;
		size += length;

		if ( m_pVertexAttribs[ i ]->m_iType == 1 ) vertexSize += 4;
		else vertexSize += 4*m_pVertexAttribs[i]->m_iComponents;
	}

	UINT vertexOffset = size;

	size += m_iNumAttribVertices * vertexSize;

	UINT indexOffset = size;
	if ( m_iNumRawIndices == 0 ) indexOffset = 0;
	size += m_iNumRawIndices * 4;

	memSize = size;
	*pMemData = new unsigned char[ memSize ];
	float *pMemDataFloat = (float*)(*pMemData);
	int *pMemDataInt = (int*)(*pMemData);
	unsigned char *pMemDataByte = (unsigned char*)(*pMemData);

	pMemDataInt[ 0 ] = m_iNumAttribVertices;
	pMemDataInt[ 1 ] = m_iNumRawIndices;
	pMemDataInt[ 2 ] = m_iNumAttribs;
	pMemDataInt[ 3 ] = vertexSize;
	pMemDataInt[ 4 ] = vertexOffset;
	pMemDataInt[ 5 ] = indexOffset;

	UINT offset = 24;
	for ( UINT i = 0; i < m_iNumAttribs; ++i )
	{
		pMemDataByte[ offset++ ] = m_pVertexAttribs[i]->m_iType;
		pMemDataByte[ offset++ ] = m_pVertexAttribs[i]->m_iComponents;
		pMemDataByte[ offset++ ] = m_pVertexAttribs[i]->m_bNormalize ? 1 : 0;
		
		UINT length = m_pVertexAttribs[ i ]->m_sName.GetLength(); // ignore the null terminator, it cancels with a -1 in the algorithm
		length /= 4;
		length = (length+1) * 4; // string length rounded up to nearest multiple of 4
		if ( length > 252 ) length = 252;
		
		pMemDataByte[ offset++ ] = length;
		memset( &(pMemDataByte[ offset ]), 0, length );
		memcpy( &(pMemDataByte[ offset ]), m_pVertexAttribs[ i ]->m_sName.GetStr(), m_pVertexAttribs[ i ]->m_sName.GetLength()+1 );
		
		offset += length;
	}

	UINT offsetFloat = offset/4;
	for ( UINT v = 0; v < m_iNumAttribVertices; ++v )
	{
		for ( UINT a = 0; a < m_iNumAttribs; ++a )
		{
			if ( m_pVertexAttribs[a]->m_iType == 1 )
			{
				pMemDataInt[ offsetFloat++ ] = ((int*)(m_pVertexAttribs[a]->m_pData))[ v ];
			}
			else
			{
				for ( UINT c = 0; c < m_pVertexAttribs[a]->m_iComponents; c++ )
				{
					pMemDataFloat[ offsetFloat++ ] = ((float*)(m_pVertexAttribs[a]->m_pData))[ v*m_pVertexAttribs[a]->m_iComponents + c ];
				}
			}
		}
	}

	for ( UINT i = 0; i < m_iNumRawIndices; ++i ) 
	{
		pMemDataInt[ offsetFloat++ ] = ((int*)m_pRawIndices)[ i ];
	}
}

void cMesh::SetVerticesFromMemblock( UINT memSize, unsigned char *pMemData )
{
	if ( !pMemData ) return;
	float *pMemDataFloat = (float*) pMemData;
	int *pMemDataInt = (int*) pMemData;

	// if the structure and num vertices hasn't changed then shortcuts can be made
	bool bChangedStructure = false;

	if ( pMemDataInt[ 2 ] != m_iNumAttribs ) 
	{
		bChangedStructure = true;
	}

	m_iFlags &= ~AGK_MESH_HAS_BONES;

	// further structure checks and vertex offset calculation
	UINT offset = 24;
	UINT vertexSize = 0;
	for ( int i = 0; i < pMemDataInt[2]; ++i )
	{
		if ( offset+8 > memSize ) 
		{
			agk::Error("Failed to set mesh from memblock data, memblock is too small for the data it claims to contain");
			return;
		}

		if ( pMemData[offset + 0] > 1 ) 
		{
			agk::Error("Failed to set mesh from memblock data, vertex attribute data type must be 0 or 1");
			return;
		}

		if ( pMemData[offset + 1] < 1 || pMemData[offset + 1] > 4 ) 
		{
			agk::Error("Failed to set mesh from memblock data, vertex attribute component count must be between 1 and 4");
			return;
		}

		if ( !bChangedStructure )
		{
			if ( pMemData[ offset + 0 ] != m_pVertexAttribs[i]->m_iType
			  || pMemData[ offset + 1 ] != m_pVertexAttribs[i]->m_iComponents ) 
			{
				bChangedStructure = true;
			}
		}

		if ( pMemData[offset + 0] == 1 ) vertexSize += 4;
		else vertexSize += pMemData[ offset + 1 ] * 4;

		UINT length = pMemData[ offset + 3 ];
		if ( length % 4 != 0 )
		{
			agk::Error("Failed to set mesh from memblock data, vertex attribute name length must be a multiple of 4");
			return;
		}

		int length2 = length;
		if ( length2 > 11 ) length2 = 11;
		if ( strncmp( ((char*)pMemData)+offset+4, "boneindices", length2 ) == 0 ) m_iFlags |= AGK_MESH_HAS_BONES;

		offset += 4;
		offset += length;
	}

	UINT vertexOffset = offset;
	UINT indexOffset = vertexOffset + pMemDataInt[0]*vertexSize;
	UINT correctSize = indexOffset + pMemDataInt[1]*4;

	if ( correctSize > memSize )
	{
		agk::Error("Failed to set mesh from memblock data, memblock is too small for the data it claims to contain");
		return;
	}

	if ( bChangedStructure )
	{
		// delete everything and start again
		if ( m_pVertexAttribs )
		{
			for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
			{
				if ( m_pVertexAttribs[ i ] ) delete m_pVertexAttribs[ i ];
			}

			delete [] m_pVertexAttribs;
		}

		m_iNumAttribVertices = pMemDataInt[ 0 ];
		m_iNumAttribs = pMemDataInt[ 2 ];
		m_pVertexAttribs = new cVertexAttrib*[ m_iNumAttribs ];

		offset = 24;
		UINT AttribOffset = 0;
		for ( UINT i = 0; i < m_iNumAttribs; ++i )
		{
			m_pVertexAttribs[ i ] = new cVertexAttrib();
			m_pVertexAttribs[ i ]->m_iType = pMemData[ offset + 0 ];
			m_pVertexAttribs[ i ]->m_iComponents = pMemData[ offset + 1 ];
			m_pVertexAttribs[ i ]->m_bNormalize = (pMemData[ offset + 2 ] != 0);
			UINT length = pMemData[ offset + 3 ];
			m_pVertexAttribs[ i ]->m_sName.SetStrN( ((char*)pMemData)+offset+4, length );
			m_pVertexAttribs[ i ]->m_iOffset = AttribOffset;
			if ( m_pVertexAttribs[ i ]->m_iType == 1 ) 
			{
				AttribOffset += 4;
				m_pVertexAttribs[ i ]->m_pData = new unsigned char[ m_iNumAttribVertices*4 ];
			}
			else 
			{
				AttribOffset += m_pVertexAttribs[ i ]->m_iComponents * 4;
				m_pVertexAttribs[ i ]->m_pData = new float[ m_iNumAttribVertices*m_pVertexAttribs[i]->m_iComponents ];
			}

			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("position") == 0 ) m_iPosAttrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("normal") == 0 ) m_iNormAttrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("uv") == 0 ) m_iUVAttrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("uv1") == 0 ) m_iUV1Attrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("tangent") == 0 ) m_iTangentAttrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("binormal") == 0 ) m_iBiNormAttrib = i;
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo("color") == 0 ) m_iColorAttrib = i;

			offset = offset + 4 + length;
		}
	}

	// if num vertices changed then recreate vertex arrays
	if ( pMemDataInt[ 0 ] != m_iNumAttribVertices )
	{
		bChangedStructure = true;
		m_iNumAttribVertices = pMemDataInt[ 0 ];
		for ( unsigned char i = 0; i < m_iNumAttribs; i++ )
		{
			if ( m_pVertexAttribs[ i ] ) 
			{
				if ( m_pVertexAttribs[ i ]->m_pData )
				{
					if ( m_pVertexAttribs[ i ]->m_iType == 0 ) 
					{
						delete [] (float*)(m_pVertexAttribs[ i ]->m_pData);
						m_pVertexAttribs[ i ]->m_pData = new float[ m_iNumAttribVertices*m_pVertexAttribs[i]->m_iComponents ];
					}
					else if ( m_pVertexAttribs[ i ]->m_iType == 1 ) 
					{
						delete [] (unsigned char*)(m_pVertexAttribs[ i ]->m_pData);
						m_pVertexAttribs[ i ]->m_pData = new unsigned char[ m_iNumAttribVertices*4 ];
					}
				}
			}
		}
	}

	// if number of indices has changed then recreate index array
	if ( pMemDataInt[ 1 ] != m_iNumRawIndices )
	{
		bChangedStructure = true;
		if ( m_pRawIndices ) delete [] m_pRawIndices;
		m_iNumRawIndices = pMemDataInt[ 1 ];
		m_pRawIndices = new UINT[ m_iNumRawIndices ];
	}

	// at this point the mesh arrays are guaranteed to line up with the memblock structure

	// copy the vertex data
	offset = vertexOffset / 4;
	for ( UINT v = 0; v < m_iNumAttribVertices; v++ )
	{
		for ( UINT a = 0; a < m_iNumAttribs; a++ )
		{
			if ( m_pVertexAttribs[ a ]->m_iType == 1 )
			{
				((int*)(m_pVertexAttribs[ a ]->m_pData))[ v ] = pMemDataInt[ offset ];
				offset++;
			}
			else
			{
				for ( UINT c = 0; c < m_pVertexAttribs[ a ]->m_iComponents; c++ )
				{
					((float*)(m_pVertexAttribs[ a ]->m_pData))[ v*m_pVertexAttribs[a]->m_iComponents + c ] = pMemDataFloat[ offset ];
					offset++;
				}
			}
		}
	}

	// copy the index data
	offset = indexOffset / 4;
	for ( UINT i = 0; i < m_iNumRawIndices; i++ )
	{
		m_pRawIndices[ i ] = pMemDataInt[ offset+i ];
	}

	int keepBuffers = 1;
	if ( bChangedStructure ) keepBuffers = 0;
	ProcessVertexData( keepBuffers );
}

void cMesh::TranslateMesh( float x, float y, float z, int update )
{
	bool bFound = false;
	for ( UINT i = 0; i < m_iNumAttribs; i++ )
	{
		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 )
		{
			float* pV = (float*) m_pVertexAttribs[ i ]->m_pData;

			for ( UINT j = 0; j < m_iNumAttribVertices; j++ )
			{
				pV[ j*3 + 0 ] += x;
				pV[ j*3 + 1 ] += y;
				pV[ j*3 + 2 ] += z;
			}

			bFound = true;
		}
	}

	if ( bFound && update != 0 ) ProcessVertexData();
}

void cMesh::RotateMesh( float w, float x, float y, float z, int update )
{
	AGKQuaternion q( w,x,y,z );

	bool bFound = false;
	for ( UINT i = 0; i < m_iNumAttribs; i++ )
	{
		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 || m_pVertexAttribs[ i ]->m_sName.CompareTo( "normal" ) == 0
		  || m_pVertexAttribs[ i ]->m_sName.CompareTo( "tangent" ) == 0  || m_pVertexAttribs[ i ]->m_sName.CompareTo( "binormal" ) == 0 )
		{
			float* pV = (float*) m_pVertexAttribs[ i ]->m_pData;

			for ( UINT j = 0; j < m_iNumAttribVertices; j++ )
			{
				AGKVector v( pV[j*3+0], pV[j*3+1], pV[j*3+2] );

				v = q * v;
				
				pV[ j*3 + 0 ] = v.x;
				pV[ j*3 + 1 ] = v.y;
				pV[ j*3 + 2 ] = v.z;
			}

			bFound = true;
		}
	}

	if ( bFound && update != 0 ) ProcessVertexData();
}

void cMesh::ScaleMesh( float x, float y, float z, int update )
{
	bool bFound = false;
	for ( UINT i = 0; i < m_iNumAttribs; i++ )
	{
		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 )
		{
			float* pV = (float*) m_pVertexAttribs[ i ]->m_pData;

			for ( UINT j = 0; j < m_iNumAttribVertices; j++ )
			{
				pV[ j*3 + 0 ] *= x;
				pV[ j*3 + 1 ] *= y;
				pV[ j*3 + 2 ] *= z;
			}

			bFound = true;
		}
		
		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "normal" ) == 0 || m_pVertexAttribs[ i ]->m_sName.CompareTo( "tangent" ) == 0
		  || m_pVertexAttribs[ i ]->m_sName.CompareTo( "binormal" ) == 0 )
		{
			float diff1 = agk::Abs( x - y );
			float diff2 = agk::Abs( y - z );
			if ( diff1 < 0.000001f && diff2 < 0.000001f ) continue; // only modify normals if the scale is non-uniform

			float* pV = (float*) m_pVertexAttribs[ i ]->m_pData;

			for ( UINT j = 0; j < m_iNumAttribVertices; j++ )
			{
				if ( x == 0 ) 
				{
					pV[ j*3 + 0 ] = 1;
					pV[ j*3 + 1 ] = 0;
					pV[ j*3 + 2 ] = 0;
				}
				else if ( y == 0 ) 
				{
					pV[ j*3 + 0 ] = 0;
					pV[ j*3 + 1 ] = 1;
					pV[ j*3 + 2 ] = 0;
				}
				else if ( z == 0 ) 
				{
					pV[ j*3 + 0 ] = 0;
					pV[ j*3 + 1 ] = 0;
					pV[ j*3 + 2 ] = 1;
				}
				else
				{
					pV[ j*3 + 0 ] /= x;
					pV[ j*3 + 1 ] /= y;
					pV[ j*3 + 2 ] /= z;

					float length = pV[j*3+0]*pV[j*3+0] + pV[j*3+1]*pV[j*3+1] + pV[j*3+2]*pV[j*3+2];
					if ( length > 0 ) length = 1 / agk::Sqrt( length );
					pV[ j*3 + 0 ] *= length;
					pV[ j*3 + 1 ] *= length;
					pV[ j*3 + 2 ] *= length;
				}
			}

			bFound = true;
		}
	}

	if ( bFound && update != 0 ) ProcessVertexData();
}

void cMesh::GetBoundingBoxForBone( int boneIndex, Bone3D *pBone, Box* pBounds )
{
	if ( !HasBones() ) return;

	// find bone attributes
	cVertexAttrib *pIndices = 0;
	cVertexAttrib *pWeights = 0;
	cVertexAttrib *pPos = 0;
	for ( int i = 0; i < m_iNumAttribs; i++ )
	{
		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "boneindices" ) == 0 )
		{
			pIndices = m_pVertexAttribs[ i ];
		}

		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "boneweights" ) == 0 )
		{
			pWeights = m_pVertexAttribs[ i ];
		}

		if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 )
		{
			pPos = m_pVertexAttribs[ i ];
		}
	}

	float minX=pBounds->minbx(), minY=pBounds->minby(), minZ=pBounds->minbz();
	float maxX=pBounds->maxbx(), maxY=pBounds->maxby(), maxZ=pBounds->maxbz();

	unsigned char *pIndexData = (unsigned char*) (pIndices->m_pData);
	float *pWeightData = (float*) (pWeights->m_pData);
	float *pPosData = (float*) (pPos->m_pData);

	int found = 0;
	for ( UINT i = 0; i < m_iNumAttribVertices; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			if ( pIndexData[ i*4 + j ] == boneIndex && pWeightData[ i*4 + j ] > 0.3f )
			{
				// transform vertex into bone space
				AGKVector pos;
				pos.x = pPosData[ i*3 + 0 ];
				pos.y = pPosData[ i*3 + 1 ];
				pos.z = pPosData[ i*3 + 2 ];

				pos = pBone->m_offsetRotation * pos;
				pos = pos + pBone->m_offsetPosition;

				if ( pos.x < minX ) minX = pos.x;
				if ( pos.x > maxX ) maxX = pos.x;
				
				if ( pos.y < minY ) minY = pos.y;
				if ( pos.y > maxY ) maxY = pos.y;

				if ( pos.z < minZ ) minZ = pos.z;
				if ( pos.z > maxZ ) maxZ = pos.z;
				found = 1;
			}
		}
	}

	if ( found ) pBounds->set( minX, minY, minZ, maxX, maxY, maxZ );
	else pBounds->set( 0,0,0,0,0,0 );
}

Face* cMesh::GetFaceList( Face **pLast )
{
	// find position attribute
	cVertexAttrib *pPos = 0;
	if ( m_iPosAttrib >= 0 ) pPos = m_pVertexAttribs[ m_iPosAttrib ];
	else
	{
		for ( int i = 0; i < m_iNumAttribs; i++ )
		{
			if ( m_pVertexAttribs[ i ]->m_sName.CompareTo( "position" ) == 0 )
			{
				pPos = m_pVertexAttribs[ i ];
				m_iPosAttrib = i;
				break;
			}
		}
	}

	if ( !pPos || !pPos->m_pData ) 
	{
		agk::Warning( "No vertex position attribute found to build collision data" );
		return 0;
	}

	if ( pPos->m_iComponents < 3 ) 
	{
		agk::Warning( "Not enough vertex position components to build collision data" );
		return 0;
	}

	AGKVector p1, p2, p3;
	UINT index;
	Face* pFaces = 0;

	if ( m_iNumRawIndices == 0 )
	{
		float *pData = (float*) pPos->m_pData;

		for ( UINT i = 0; i < m_iNumAttribVertices/3; i++ )
		{
			p1.Set( pData[ i*9 + 0 ], pData[ i*9 + 1 ], pData[ i*9 + 2 ] );
			p2.Set( pData[ i*9 + 3 ], pData[ i*9 + 4 ], pData[ i*9 + 5 ] );
			p3.Set( pData[ i*9 + 6 ], pData[ i*9 + 7 ], pData[ i*9 + 8 ] );

			Face* aFace = new Face( );
			if ( !aFace->MakeFace( i, &p1, &p2, &p3 ) ) 
			{ 
				delete aFace; 
				continue; 
			}
	        
			if ( pFaces == 0 && pLast ) *pLast = aFace;
			aFace->nextFace = pFaces;
			pFaces = aFace;
		}
	}
	else
	{
		float *pData = (float*) pPos->m_pData;

		if ( m_iPrimitiveType == AGK_TRIANGLES )
		{
			for ( UINT i = 0; i < m_iNumRawIndices/3; i++ )
			{
				index = m_pRawIndices[ i*3 + 0 ];
				p1.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				index = m_pRawIndices[ i*3 + 1 ];
				p2.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				index = m_pRawIndices[ i*3 + 2 ];
				p3.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				Face* aFace = new Face( );
				if ( !aFace->MakeFace( i, &p1, &p2, &p3 ) ) 
				{ 
					delete aFace; 
					continue; 
				}
		        
				if ( pFaces == 0 && pLast ) *pLast = aFace;
				aFace->nextFace = pFaces;
				pFaces = aFace;
			}
		}
		else if ( m_iPrimitiveType == AGK_TRIANGLE_STRIP )
		{
			int reverse = 0;
			for ( UINT i = 2; i < m_iNumRawIndices; i++ )
			{
				reverse = 1-reverse; // every other triangle needs its winding reversed

				index = m_pRawIndices[ i - 2 ];
				p1.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				index = m_pRawIndices[ i - reverse ];
				p2.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				index = m_pRawIndices[ i - (1-reverse) ];
				p3.Set( pData[ index*3 + 0 ], pData[ index*3 + 1 ], pData[ index*3 + 2 ] );

				Face* aFace = new Face( );
				if ( !aFace->MakeFace( i, &p1, &p2, &p3 ) ) 
				{ 
					delete aFace; 
					continue; 
				}
		        
				if ( pFaces == 0 && pLast ) *pLast = aFace;
				aFace->nextFace = pFaces;
				pFaces = aFace;
			}
		}
	}

	return pFaces;
}

cImage* cMesh::GetImage( UINT stage )
{
	if ( stage > 7 ) return 0;
	return m_pImages[ stage ];
}

void cMesh::SetImage( cImage *pImage, UINT stage )
{
	if ( stage >= AGK_MAX_TEXTURES ) return;

	if ( stage == 1 ) m_iFlags &= ~AGK_MESH_HAS_LIGHTMAP;
	if ( stage == 2 ) m_iFlags &= ~AGK_MESH_HAS_NORMALMAP;
		
	m_pImages[ stage ] = pImage;
}

void cMesh::SetUVOffset( UINT stage, float offsetU, float offsetV )
{
	if ( stage >= AGK_MAX_TEXTURES ) return;

	m_fUVOffsetU[ stage ] = offsetU;
	m_fUVOffsetV[ stage ] = offsetV;
}

void cMesh::SetUVScale( UINT stage, float scaleU, float scaleV )
{
	if ( stage >= AGK_MAX_TEXTURES ) return;

	m_fUVScaleU[ stage ] = scaleU;
	m_fUVScaleV[ stage ] = scaleV;
}

void cMesh::SetLightMap( cImage *pImage )
{
	m_iFlags |= AGK_MESH_HAS_LIGHTMAP;
	m_pImages[ 1 ] = pImage;
}

void cMesh::SetNormalMap( cImage *pImage )
{
	m_iFlags |= AGK_MESH_HAS_NORMALMAP;
	m_pImages[ 2 ] = pImage;
}

void cMesh::SetNormalMapScale( float scaleU, float scaleV )
{
	m_fNormalScaleU = scaleU;
	m_fNormalScaleV = scaleV;
}

void cMesh::SetShader( AGKShader *pShader ) 
{ 
	if ( pShader ) 
	{
		if ( m_pOrigShader != pShader )
		{
			if ( m_pOrigShader ) m_pOrigShader->RemoveRef();
			pShader->AddRef();
			m_pOrigShader = pShader;
		}
	}
	else 
	{
		// AGK will generate a shader to display this object
		m_pOrigShader = 0;
	}
}

void cMesh::SetLights( int numVSLights, AGKPointLight **pVSLights, int numPSLights, AGKPointLight **pPSLights )
{
	if ( numVSLights > AGK_MAX_VERTEX_LIGHTS ) numVSLights = AGK_MAX_VERTEX_LIGHTS;
	if ( numPSLights > AGK_MAX_PIXEL_LIGHTS ) numPSLights = AGK_MAX_PIXEL_LIGHTS;

	m_iNumVSLights = numVSLights;
	for ( int i = 0; i < numVSLights; i++ )
	{
		m_pVSLights[ i ] = pVSLights[i];
	}

	m_iNumPSLights = numPSLights;
	for ( int i = 0; i < numPSLights; i++ )
	{
		m_pPSLights[ i ] = pPSLights[i];
	}
}

void cMesh::CheckShader()
{
	if ( m_pOrigShader && m_pOrigShader->IsCustom() && !m_pOrigShader->IsValid() )
	{
		agk::Warning("Invalid shader was removed and replaced with a system generated one");
		m_pOrigShader->RemoveRef();
		m_pOrigShader = 0;
	}

	if ( !m_pOrigShader || !m_pOrigShader->IsCustom() )
	{
		UINT hash = AGKShader::GetMeshShaderHash( this );
		if ( !m_pOrigShader || m_pOrigShader->GetHash() != hash )
		{
			// something changed so we need to switch shader
			AGKShader *pNewShader = AGKShader::Make3DShader( this );
			if ( pNewShader != m_pOrigShader && pNewShader ) 
			{
				if ( m_pOrigShader ) m_pOrigShader->RemoveRef();
				pNewShader->AddRef();
				m_pOrigShader = pNewShader;
			}
		}
	}

	if ( !m_pOrigShader ) return;

	if ( !m_pOrigShader->NeedsAdditionalCode() )
	{
		// shader doesn't need additional code, so use it directly, we might be referencing the 
		// same shader twice but as long as we remove it twice later it should be fine
		if ( m_pShader != m_pOrigShader )
		{
			if ( m_pShader ) m_pShader->RemoveRef();
			m_pOrigShader->AddRef();
			m_pShader = m_pOrigShader;

			CreateDummyAttributesForShader( m_pShader );
		}
	}
	else
	{
		// shader needs additional code, check if we've already done it
		UINT hash = AGKShader::GetFinalShaderHash( agk::m_cDirectionalLight.m_active, m_iNumVSLights, m_iNumPSLights, WantsShadows() );
		if ( !m_pShader || m_pShader->GetHash() != hash || m_pShader->GetBaseShader() != m_pOrigShader )
		{
			// need to update m_pShader
			AGKShader *pNewShader = AGKShader::MakeFinalShader( m_pOrigShader, agk::m_cDirectionalLight.m_active, m_iNumVSLights, m_iNumPSLights, WantsShadows(), HasNormalMap() );
			if ( !pNewShader )
			{
				if ( m_pOrigShader->IsCustom() )
				{
					agk::Warning("Invalid shader was removed and replaced with a system generated one");
					m_pOrigShader->SetValid( false );
					m_pOrigShader->RemoveRef();
					m_pOrigShader = 0;
				}
			}
			else if ( pNewShader != m_pShader )
			{
				if ( m_pShader ) m_pShader->RemoveRef();
				pNewShader->AddRef();
				m_pShader = pNewShader;

				CreateDummyAttributesForShader( m_pShader );
			}
		}
	}
}

void cMesh::Update()
{
	
}

void cMesh::Draw()
{
	if ( !m_pObject->GetVisible() ) return;
	if ( !GetVisible() ) return;

	// set mesh textures, stage 7 is now the shadow map (if used)
	int maxTex = AGK_MAX_TEXTURES;
	AGKShader* g_pSelectedShader = AGKShader::GetCurrentShader();

	if ( WantsShadows() )
	{
		if ( agk::GetShadowMappingMode() > 0 ) 
		{
			g_pSelectedShader->SetTextureStage( agk::m_pShadowMap, 7, 1 );
			maxTex = 7;
		}
		if ( agk::GetShadowMappingMode() == 3 ) 
		{
			g_pSelectedShader->SetTextureStage( agk::m_pShadowMap2, 6, 1 );
			g_pSelectedShader->SetTextureStage( agk::m_pShadowMap3, 5, 1 );
			g_pSelectedShader->SetTextureStage( agk::m_pShadowMap4, 4, 1 );
			maxTex = 4;
		}
	}

	for ( int i = 0; i < maxTex; i++ )
	{
		g_pSelectedShader->SetTextureStage( m_pImages[i], i, 1 );
	}

	for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
	{
		g_pSelectedShader->SetUVScale( i, GetUVOffsetU(i), GetUVOffsetV(i), GetUVScaleU(i), GetUVScaleV(i) );
	}

	if ( HasNormalMap() )
	{
		g_pSelectedShader->SetTempConstantByName( "agk_NormalScale", m_fNormalScaleU, m_fNormalScaleV, 0, 0 );
	}

	if ( m_iNumVSLights > 0 || m_iNumPSLights > 0 )
	{
		const char nums[ 16 ] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G' };

		char szLightPosVar[ 32 ] = "agk_VSLight1Pos";
		char szLightColorVar[ 32 ] = "agk_VSLight1Color";

		// set up lights
		for ( int i = 0; i < m_iNumVSLights; i++ )
		{
			szLightPosVar[ 11 ] = nums[ i ];
			szLightColorVar[ 11 ] = nums[ i ];
			g_pSelectedShader->SetTempConstantByName( szLightPosVar, m_pVSLights[i]->m_position.x, m_pVSLights[i]->m_position.y, m_pVSLights[i]->m_position.z, m_pVSLights[i]->m_fRadius*m_pVSLights[i]->m_fRadius );
			g_pSelectedShader->SetTempConstantByName( szLightColorVar, m_pVSLights[i]->m_color.x, m_pVSLights[i]->m_color.y, m_pVSLights[i]->m_color.z, 1 );
		}

		szLightPosVar[ 4 ] = 'P';
		szLightColorVar[ 4 ] = 'P';

		for ( int i = 0; i < m_iNumPSLights; i++ )
		{
			szLightPosVar[ 11 ] = nums[ i ];
			szLightColorVar[ 11 ] = nums[ i ];
			g_pSelectedShader->SetTempConstantByName( szLightPosVar, m_pPSLights[i]->m_position.x, m_pPSLights[i]->m_position.y, m_pPSLights[i]->m_position.z, m_pPSLights[i]->m_fRadius*m_pPSLights[i]->m_fRadius );
			g_pSelectedShader->SetTempConstantByName( szLightColorVar, m_pPSLights[i]->m_color.x, m_pPSLights[i]->m_color.y, m_pPSLights[i]->m_color.z, 1 );
		}
	}

	agk::ResetScissor();

	PlatformDraw(0);
}

void cMesh::DrawShadow()
{
	//if ( !m_pObject->GetVisible() ) return; // draw shadow even if object is inivisible

	if ( m_pObject->HasAlphaMask() )
	{
		AGKShader::GetCurrentShader()->SetTextureStage( m_pImages[0], 0, 1 );
		AGKShader::GetCurrentShader()->SetUVScale( 0, GetUVOffsetU(0), GetUVOffsetV(0), GetUVScaleU(0), GetUVScaleV(0) );
	}
	
	PlatformDraw( 1, m_pObject->HasAlphaMask()?1:0 );
}
