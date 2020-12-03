#include "agk.h"

cMaterial* cMaterial::g_pAllMaterials = 0;

cMaterial::cMaterial()
{
	if ( g_pAllMaterials ) g_pAllMaterials->m_pPrevMaterial = this;
	m_pNextMaterial = g_pAllMaterials;
	m_pPrevMaterial = 0;
	g_pAllMaterials = this;

	for (int i = 0; i < AGK_MAX_TEXTURES; i++ ) m_pImages[ i ] = 0;
}

cMaterial::~cMaterial()
{
	// remove from list of all materials
	if ( m_pPrevMaterial ) m_pPrevMaterial->m_pNextMaterial = m_pNextMaterial;
	else g_pAllMaterials = m_pNextMaterial;
	if ( m_pNextMaterial ) m_pNextMaterial->m_pPrevMaterial = m_pPrevMaterial;

	// clear any pointers to this material
	cMesh *pMesh = m_pMeshes.GetFirst();
	while( pMesh )
	{
		pMesh->m_pMaterial = 0;
		pMesh = m_pMeshes.GetNext();
	}
	m_pMeshes.ClearAll();
}

cMaterial* cMaterial::FindMaterial( cImage **pDesiredImages )
{
	cMaterial *pMaterial = g_pAllMaterials;
	while( pMaterial )
	{
		bool bFound = true;
		for ( int i = 0; i < AGK_MAX_TEXTURES; i++ )
		{
			if ( pMaterial->m_pImages[ i ] != pDesiredImages[ i ] ) 
			{
				bFound = false;
				break;
			}
		}

		if ( bFound ) return pMaterial;
		pMaterial = pMaterial->m_pNextMaterial;
	}

	return 0;
}

void cMaterial::AddMesh( cMesh *pMesh ) 
{ 
	m_pMeshes.AddItem( pMesh, pMesh ); 
}

void cMaterial::RemoveMesh( cMesh *pMesh )
{
	m_pMeshes.RemoveItem( pMesh );

	if ( m_pMeshes.GetCount() == 0 )
	{
		// delete or retire this material as it is no longer used
	}
}