#pragma once
#include "D3D11Graphics.h"
#include "D3DGraphics3D.h"

namespace D3D11Graphics {

class D3DDrawingModel
{
public:
	virtual ~D3DDrawingModel() = 0;

	void DrawTo(D3DGraphics3D& g);

protected:
	virtual void OnDrawTo(D3DGraphics3D& g) = 0;
};

} // namespace D3D11Graphics
