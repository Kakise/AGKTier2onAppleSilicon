#include "agk.h"

using namespace AGK;

cNode::cNode()
{
	m_pParentNode = 0;
	m_iNodeFlags = 0;
	m_scale.Set(1,1,1);
	m_scaleFinal.Set(1,1,1);
}

cNode::~cNode()
{
	RemoveFromParent();
	for( UINT i = 0; i < m_vChildren.size(); i++ )
	{
		m_vChildren[ i ]->m_pParentNode = 0;
		m_vChildren[ i ]->NeedsUpdate();
	}
	m_vChildren.clear();
}

void cNode::AddChild( cNode *pChild )
{
	if ( pChild->m_pParentNode ) pChild->RemoveFromParent();
	m_vChildren.push_back( pChild );
	pChild->m_pParentNode = this;
	pChild->NeedsUpdate();
}

void cNode::RemoveChild( cNode *pChild )
{
	for( UINT i = 0; i < m_vChildren.size(); i++ )
	{
		if ( m_vChildren[ i ] == pChild )
		{
			m_vChildren.erase( m_vChildren.begin()+i );
			pChild->m_pParentNode = 0;
			pChild->NeedsUpdate();
			return;
		}
	}
}

void cNode::RemoveFromParent()
{
	if ( !m_pParentNode ) return;
	m_pParentNode->RemoveChild( this );
	m_pParentNode = 0;
	NeedsUpdate();
}

void cNode::RemoveFromDeletingParent()
{
	m_pParentNode = 0;
	NeedsUpdate();
}

void cNode::TransformChildrenByParent()
{
	for( UINT i = 0; i < m_vChildren.size(); i++ )
	{
		m_vChildren[ i ]->m_position = (m_rotation * (m_vChildren[ i ]->m_position*m_scale)) + m_position;
		m_vChildren[ i ]->m_rotation = m_rotation * m_vChildren[ i ]->m_rotation;
		m_vChildren[ i ]->m_scale = m_scale * m_vChildren[ i ]->m_scale;
		m_vChildren[ i ]->NeedsUpdate();
	}
}

float cNode::GetLargestWorldScale()
{
	float largestScale = scaleFinal().x;
	float scale = scaleFinal().y;
	if ( scale > largestScale ) largestScale = scale;
	scale = scaleFinal().z;
	if ( scale > largestScale ) largestScale = scale;
	return largestScale;
}

void cNode::MoveLocalX( float amount )
{
	AGKVector axis(amount,0,0);
	axis.MultX( rot() );
	SetNodePosition( pos().x + axis.x, pos().y + axis.y, pos().z + axis.z );
}

void cNode::MoveLocalY( float amount )
{
	AGKVector axis(0,amount,0);
	axis.MultY( rot() );
	SetNodePosition( pos().x + axis.x, pos().y + axis.y, pos().z + axis.z );
}

void cNode::MoveLocalZ( float amount )
{
	AGKVector axis(0,0,amount);
	axis.MultZ( rot() );
	SetNodePosition( pos().x + axis.x, pos().y + axis.y, pos().z + axis.z );
}

void cNode::RotateLocalX( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddLocalRotation( 0, amount );
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::RotateLocalY( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddLocalRotation( 1, amount ); 
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::RotateLocalZ( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddLocalRotation( 2, amount ); 
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::RotateGlobalX( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddGlobalRotation( 0, amount ); 
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::RotateGlobalY( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddGlobalRotation( 1, amount ); 
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::RotateGlobalZ( float amount ) 
{ 
	AGKQuaternion q( rot().w, rot().x, rot().y, rot().z );
	q.AddGlobalRotation( 2, amount ); 
	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::LookAt( float x, float y, float z, float roll ) 
{ 
	AGKQuaternion q;
	q.LookAt( x-posFinal().x, y-posFinal().y, z-posFinal().z, roll ); 
	
	if ( m_pParentNode )
	{
		// find location rotation that points to world point
		AGKQuaternion q2(m_pParentNode->rotFinal());
		q2.Invert();
		q = q2 * q;
	}

	SetNodeRotation( q.w, q.x, q.y, q.z );
}

void cNode::OverrideWorldPosition( float x, float y, float z ) 
{ 
	m_position.Set(x,y,z); 
	m_positionFinal.Set(x,y,z); 
	m_iNodeFlags |= AGK_NODE_OVERRIDE_POSITION; 
	ChildrenNeedUpdate(); 
}

void cNode::OverrideWorldRotation( float w, float x, float y, float z ) 
{ 
	m_rotation.Set(w,x,y,z); 
	m_rotationFinal.Set(w,x,y,z); 
	m_iNodeFlags |= AGK_NODE_OVERRIDE_ROTATION; 
	ChildrenNeedUpdate(); 
}

void cNode::OverrideWorldScale( float x, float y, float z ) 
{ 
	m_scale.Set(x,y,z); 
	m_scaleFinal.Set(x,y,z); 
	m_iNodeFlags |= AGK_NODE_OVERRIDE_SCALE; 
	ChildrenNeedUpdate(); 
}

void cNode::RemoveWorldOverride()
{
	// set relative transforms so the object stays in its current position during the next update
	if ( !m_pParentNode )
	{
		m_position = m_positionFinal;
		m_rotation = m_rotationFinal;
		m_scale = m_scaleFinal;
	}
	else
	{
		// calculate relative transform from parent, transform absolute points by inverse parent transform
		m_pParentNode->UpdateNode();
		AGKQuaternion q(m_pParentNode->m_rotationFinal);
		q.Invert();
		m_rotation = q * m_rotationFinal;

		AGKVector v(m_positionFinal - m_pParentNode->m_positionFinal);
		m_position = (q * v) / m_pParentNode->m_scaleFinal;

		m_scale = m_scaleFinal / m_pParentNode->m_scaleFinal;
	}

	m_iNodeFlags &= ~(AGK_NODE_OVERRIDE_POSITION | AGK_NODE_OVERRIDE_ROTATION | AGK_NODE_OVERRIDE_SCALE);
	NeedsUpdate();
}

void cNode::NeedsUpdate()
{
	if ( (m_iNodeFlags & AGK_NODE_NEEDS_UPDATE) != 0 ) return;

	// if we are overriding both position and rotation then we don't need to update anything in this node
	if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_POSITION) == 0 || (m_iNodeFlags & AGK_NODE_OVERRIDE_ROTATION) == 0 || (m_iNodeFlags & AGK_NODE_OVERRIDE_SCALE) == 0 ) 
	{
		m_iNodeFlags |= AGK_NODE_NEEDS_UPDATE;
		ChildrenNeedUpdate();
	}
}

void cNode::ChildrenNeedUpdate()
{
	for( UINT i = 0; i < m_vChildren.size(); i++ )
	{
		m_vChildren[ i ]->NeedsUpdate();
	}
}

void cNode::UpdateNode()
{
	if ( (m_iNodeFlags & AGK_NODE_NEEDS_UPDATE) == 0 ) return;
	m_iNodeFlags &= ~AGK_NODE_NEEDS_UPDATE;
	
	if ( m_pParentNode )
	{
		m_pParentNode->UpdateNode();
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_POSITION) == 0 ) m_positionFinal = (m_pParentNode->m_rotationFinal * (m_position*m_pParentNode->m_scaleFinal)) + m_pParentNode->m_positionFinal;
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_ROTATION) == 0 ) m_rotationFinal = m_pParentNode->m_rotationFinal * m_rotation; // not affected by scale
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_SCALE) == 0 ) m_scaleFinal = m_pParentNode->m_scaleFinal * m_scale; // no shear, non-uniform scales might not propagate as expected
	}
	else
	{
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_POSITION) == 0 ) m_positionFinal = m_position;
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_ROTATION) == 0 ) m_rotationFinal = m_rotation;
		if ( (m_iNodeFlags & AGK_NODE_OVERRIDE_SCALE) == 0 ) m_scaleFinal = m_scale;
	}

	UpdatedCallback(); // notify sub objects that the world transform has changed, used by cameras to update view matrix
}