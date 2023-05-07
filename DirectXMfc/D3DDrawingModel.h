#pragma once
#include "D3D11Graphics.h"
#include "D3DGraphics3D.h"
#include "D3DVector.h"

namespace D3D11Graphics {

class D3DDrawingModel
{
public:
	virtual ~D3DDrawingModel() = 0;

	void DrawTo(D3DGraphics3D& g);

	std::vector<D3DVector3d> SelectPoints(D3DSelectionTargetId selId)
	{
		return OnSelectPoints(selId);
	}

protected:
	virtual void OnDrawTo(D3DGraphics3D& g) = 0;
	virtual std::vector<D3DVector3d> OnSelectPoints(D3DSelectionTargetId selId) { return std::vector<D3DVector3d>(); };
};

} // namespace D3D11Graphics
