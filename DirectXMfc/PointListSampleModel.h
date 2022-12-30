#pragma once

#include "D3DModelPointList.h"
#include "D3DModelPointListEnumerator.h"

class PointListSampleModel : public D3D11Graphics::D3DModelPointList
{
protected:
	virtual void OnCreateBuffer(
		D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g,
		D3D11Graphics::D3DBufferPtr* ppVB, size_t* pnVertex
	);
};

////////////////////////////////////////////////////////////////////////////////

class PointListEnumeratorSampleModel : public D3D11Graphics::D3DModelPointListEnumerator
{
private:
	typedef DirectX::XMFLOAT3 XMFLOAT3;

public:
	PointListEnumeratorSampleModel();
protected:
	virtual void OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g);
	virtual void OnPostDraw();

	virtual D3DVertexBufferEnumerator::VertexBufferData OnGetNext();

private:
	void PrepareVertices(D3D11Graphics::D3DGraphics& g);

private:
	size_t m_nVertexInUnit;
	std::vector<Vertex> m_vertices;
	size_t m_nextVertex = 0;
	size_t m_nMaxDrawnPointInFrame = 0;
	size_t m_firstVertexInFrame = 0;
	D3D11Graphics::D3DBufferPtr m_pVertexBuffer;
	D3D11Graphics::D3DGraphics* m_pGraphics = nullptr;
	bool m_isViewMoving = false;
};
