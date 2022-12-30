#pragma once

#include "D3DDrawingModel.h"
#include "D3DVertexBufferEnumerator.h"
#include "D3DGraphics3D.h"

namespace D3D11Graphics {

class D3DModelPointListEnumerator : public D3DDrawingModel, protected D3DVertexBufferEnumerator
{
private:
	friend class D3DGraphics3D;

public:
	typedef D3DGraphics3D::PointListVertex Vertex;

public:
	D3DModelPointListEnumerator();
	virtual ~D3DModelPointListEnumerator();

protected:
	virtual void OnDrawTo(D3DGraphics3D& g);
	void PreDraw(D3DGraphics3D& g3D, D3DGraphics& g) { OnPreDraw(g3D, g); }
	void PostDraw() { OnPostDraw(); }

	virtual void OnPreDraw(D3DGraphics3D& g3D, D3DGraphics& g) = 0;
	virtual void OnPostDraw() = 0;

	// This function has been declared in D3DVertexBufferEnumerator.
	// It must be implemented in derived classes.
	//virtual VertexBufferData OnGetNext() = 0;
};

} // namespace D3D11Graphics
