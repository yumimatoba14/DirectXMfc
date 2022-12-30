#include "pch.h"
#include "D3DModelPointListEnumerator.h"

using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DModelPointListEnumerator::D3DModelPointListEnumerator()
{
}

D3DModelPointListEnumerator::~D3DModelPointListEnumerator()
{
}

////////////////////////////////////////////////////////////////////////////////

void D3DModelPointListEnumerator::OnDrawTo(D3DGraphics3D& g)
{
	g.DrawPointListEnumerator(this);
}
