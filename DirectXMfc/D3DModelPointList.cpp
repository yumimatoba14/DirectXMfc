#include "pch.h"
#include "D3DModelPointList.h"
#include "D3DModelColorUtil.h"

using namespace std;
using namespace D3D11Graphics;
using namespace D3D11Graphics::D3DModelColorUtil;

D3DModelPointList::D3DModelPointList()
	: m_nVertex(0)
{

}

D3DModelPointList::~D3DModelPointList()
{

}

void D3DModelPointList::OnDrawTo(D3DGraphics3D& g)
{
	g.DrawPointList(this);
}

void D3DModelPointList::PreDraw(D3DGraphics3D& g3D, D3DGraphics& g)
{
	if (!m_pVertexBuffer)
	{
		OnCreateBuffer(g3D, g, &m_pVertexBuffer, &m_nVertex);
		P_IS_TRUE(m_pVertexBuffer);
	}
	else {
		// TODO: update buffers if needed.
	}
}

void D3DModelPointList::OnCreateBuffer(D3DGraphics3D& g3D, D3DGraphics& g, D3DBufferPtr* ppVB, size_t* pnVertex)
{
	const float z0 = 0.5;
	const float z1 = 0.75;
	const float z2 = 0.25;
	UINT color2 = Rgba(128, 128, 0, 255);
	UINT color3 = Rgba(0, 128, 128, 255);
	vector<Vertex> vertices =
	{
		{ XMFLOAT3(0.0f, 1.0f, z0), RgbaF(1,0,0,0)},
		{ XMFLOAT3(-1.0f, 0.0f, z0), RgbaF(0,1,0,1)},
		{ XMFLOAT3(0.0f, -1.0f, z0), RgbaF(0,0,1,1)},
		{ XMFLOAT3(1.0f, 0.0f, z0), RgbaF(1,1,1,1)},
		{ XMFLOAT3(0.25f, -0.25f, z1), color2},
		{ XMFLOAT3(0.75f, -0.75f, z1), color2},
		{ XMFLOAT3(0.75f, -0.25f, z1), color2},
		{ XMFLOAT3(0.25f, 0.25f, z2), color3},
		{ XMFLOAT3(0.75f, 0.25f, z2), color3},
		{ XMFLOAT3(0.75f, 0.75f, z2), color3},
	};

	*pnVertex = vertices.size();
	*ppVB = g.CreateVertexBuffer(vertices.data(), static_cast<UINT>(*pnVertex));
}
