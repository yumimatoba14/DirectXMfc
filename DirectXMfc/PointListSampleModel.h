#pragma once

#include "D3DModelPointList.h"

class PointListSampleModel : public D3D11Graphics::D3DModelPointList
{
protected:
	virtual void OnCreateBuffer(
		D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g,
		D3D11Graphics::D3DBufferPtr* ppVB, size_t* pnVertex
	);
};

