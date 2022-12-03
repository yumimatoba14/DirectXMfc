#pragma once

#include "D3DDrawingModel.h"

namespace D3D11Graphics {

class D3DModelTriangleList : public D3DDrawingModel
{
private:
	friend class D3DGraphics3D;
public:
	typedef DirectX::XMFLOAT3 XMFLOAT3;
	typedef DirectX::XMFLOAT4 XMFLOAT4;

	struct Vertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 col;
	};

	typedef UINT IndexType;

public:
	virtual ~D3DModelTriangleList();

protected:
	virtual void OnDrawTo(D3DGraphics3D& g);

	void PreDraw(D3DGraphics3D& g3D, D3DGraphics& g);
	virtual void OnCreateBuffers(
		D3DGraphics3D& g3D, D3DGraphics& g, D3DBufferPtr* ppVB, D3DBufferPtr* ppIB, size_t* pnIndex
	);

private:
	D3DBufferPtr m_pVertexBuffer;
	D3DBufferPtr m_pIndexBuffer;
	size_t m_nIndex = 0;
};

} // end of namespace D3D11Graphics
