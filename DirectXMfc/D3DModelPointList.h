#pragma once

#include "D3DDrawingModel.h"

namespace D3D11Graphics {
	
class D3DModelPointList : public D3DDrawingModel
{
private:
	friend class D3DGraphics3D;
public:
	typedef DirectX::XMFLOAT3 XMFLOAT3;
	typedef DirectX::XMFLOAT4 XMFLOAT4;
	typedef D3DGraphics3D::PointListVertex Vertex;

public:
	D3DModelPointList();
	virtual ~D3DModelPointList();

protected:
	virtual void OnDrawTo(D3DGraphics3D& g);
	void PreDraw(D3DGraphics3D& g3D, D3DGraphics& g);
	virtual void OnCreateBuffer(D3DGraphics3D& g3D, D3DGraphics& g, D3DBufferPtr* ppVB, size_t* pnVertex);

private:
	D3DBufferPtr m_pVertexBuffer;
	size_t m_nVertex;
};

} // end of namespace D3D11Graphics
