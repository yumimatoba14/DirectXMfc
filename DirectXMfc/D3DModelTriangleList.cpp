#include "pch.h"
#include "D3DModelTriangleList.h"
//#include <vector>

using namespace std;
using namespace D3D11Graphics;

D3DModelTriangleList::~D3DModelTriangleList()
{

}

void D3DModelTriangleList::OnDrawTo(D3DGraphics3D& g)
{
	g.DrawTriangleList(this);
}

void D3DModelTriangleList::PreDraw(D3DGraphics3D& g3D, D3DGraphics& g)
{
	if (!m_pVertexBuffer || !m_pIndexBuffer)
	{
		OnCreateBuffers(g3D, g, &m_pVertexBuffer, &m_pIndexBuffer, &m_nIndex);
		P_IS_TRUE(m_pVertexBuffer);
		P_IS_TRUE(m_pIndexBuffer);
	}
	else {
		// TODO: update buffers if needed.
	}
}

void D3DModelTriangleList::OnCreateBuffers(
	D3DGraphics3D& g3D, D3DGraphics& g, D3DBufferPtr* ppVB, D3DBufferPtr* ppIB, size_t* pnIndex
)
{
	const float z0 = 0.5;
	const float z1 = 0.75;
	const float z2 = 0.25;
	vector<Vertex> vertices =
	{
		{ XMFLOAT3(0.0f, 1.0f, z0), XMFLOAT4(1,0,0,1)},
		{ XMFLOAT3(-1.0f, 0.0f, z0), XMFLOAT4(0,1,0,1)},
		{ XMFLOAT3(0.0f, -1.0f, z0), XMFLOAT4(0,0,1,1)},
		{ XMFLOAT3(1.0f, 0.0f, z0), XMFLOAT4(0,0,0,1)},
		{ XMFLOAT3(0.25f, -0.25f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.75f, -0.75f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.75f, -0.25f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.25f, 0.25f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
		{ XMFLOAT3(0.75f, 0.25f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
		{ XMFLOAT3(0.75f, 0.75f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
	};

	vector<UINT> indices = { 0,1,2,0,2,3,4,5,6,7,9,8 };

	size_t nVertex = vertices.size();
	size_t vertexSize = sizeof(Vertex);
	*pnIndex = indices.size();
	*ppVB = g.CreateVertexBuffer(vertices.data(), static_cast<UINT>(nVertex));
	*ppIB = g.CreateIndexBuffer(indices.data(), static_cast<UINT>(m_nIndex));
}

////////////////////////////////////////////////////////////////////////////////

