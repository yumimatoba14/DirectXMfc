#include "pch.h"
#include "PointListSampleModel.h"
#include "D3DModelColorUtil.h"

using namespace std;
using namespace D3D11Graphics;
using namespace D3D11Graphics::D3DModelColorUtil;

void PointListSampleModel::OnCreateBuffer(
	D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g,
	D3D11Graphics::D3DBufferPtr* ppVB, size_t* pnVertex
)
{
	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 1;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -0.1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	const UINT color2 = RgbaF(0.5f, 0.0f, 1.0f, 1);
	const bool useAnotherColor = false;
	vector<Vertex> vertices;
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				Vertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				if (useAnotherColor && iX % 2 == 1) {
					vtx.rgba = color2;
				}
				vertices.push_back(vtx);
			}
		}
	}

	*pnVertex = vertices.size();
	*ppVB = g.CreateVertexBuffer(vertices.data(), static_cast<UINT>(*pnVertex), false);
}

////////////////////////////////////////////////////////////////////////////////

PointListEnumeratorSampleModel::PointListEnumeratorSampleModel()
{
	m_nVertexInUnit = 1000 * 1000 * 1;
}

void PointListEnumeratorSampleModel::OnPreDraw(D3D11Graphics::D3DGraphics3D& g3D, D3D11Graphics::D3DGraphics& g)
{
	if (g3D.IsProgressiveViewMode()) {
		m_nMaxDrawnPointInFrame = m_nVertexInUnit;
		if (!g3D.IsProgressiveViewFollowingFrame()) {
			m_nextVertex = 0;
		}
		m_firstVertexInFrame = m_nextVertex;
	} else {
		m_nextVertex = 0;
		m_firstVertexInFrame = 0;
		m_nMaxDrawnPointInFrame = 0;
	}
	m_pGraphics = &g;
	m_isViewMoving = g3D.IsViewMoving();
	if (m_pVertexBuffer.Get() == nullptr) {
		PrepareVertices(g);
		P_ASSERT(m_pVertexBuffer);
	}
	GoNext();
}

void PointListEnumeratorSampleModel::OnPostDraw()
{
	m_pGraphics = nullptr;
}

D3DVertexBufferEnumerator::VertexBufferData PointListEnumeratorSampleModel::OnGetNext()
{
	size_t endVertex = min(m_nextVertex + m_nVertexInUnit, m_vertices.size());

	D3DVertexBufferEnumerator::VertexBufferData data;
	data.pVertexBuffer = m_pVertexBuffer;
	data.nVertex = UINT(endVertex - m_nextVertex);

	if (m_isViewMoving && 0 < m_nextVertex) {
		data.nVertex = 0;
	}
	else if (0 < m_nMaxDrawnPointInFrame) {
		if (m_firstVertexInFrame + m_nMaxDrawnPointInFrame <= m_nextVertex) {
			// This frame should be ended when m_nMaxDrawnPointInFrame points had been drawn.
			data.nVertex = 0;
		}
	}
		
	if (data.nVertex == 0) {
		data.pVertexBuffer = nullptr;
		return data;
	}

	{
		D3DMappedSubResource mappedMemory = m_pGraphics->MapDyamaicBuffer(m_pVertexBuffer);
		mappedMemory.Write(m_vertices.data() + m_nextVertex, (data.nVertex)*sizeof(Vertex));
	}

	m_nextVertex = endVertex;
	return data;
}

void PointListEnumeratorSampleModel::PrepareVertices(D3D11Graphics::D3DGraphics& g)
{
	const size_t nX = 1000;
	const size_t nY = 1000;
	const size_t nZ = 10;
	const double x0 = 0;
	const double y0 = 0;
	const double z0 = 0;
	const double aDist[3] = { 1.0 / nX, 1.0 / nY, -0.1 };
	const UINT aColor[3] = {
		RgbaF(0.0f, 1.0f, 1.0f, 1),
		RgbaF(0.0f, 0.0f, 1.0f, 1),
		RgbaF(1.0f, 0.0f, 0.5f, 1)
	};
	const UINT color2 = RgbaF(0.5f, 0.0f, 1.0f, 1);
	const bool useAnotherColor = false;
	m_vertices.clear();
	for (size_t iZ = 0; iZ < nZ; ++iZ) {
		const float z = float(z0 + iZ * aDist[2]);
		const UINT color = aColor[iZ % 3];
		for (size_t iY = 0; iY < nY; ++iY) {
			for (size_t iX = 0; iX < nX; ++iX) {
				Vertex vtx{ XMFLOAT3(float(x0 + iX * aDist[0]), float(y0 + iY * aDist[1]), float(z)), color };
				if (useAnotherColor && iX % 2 == 1) {
					vtx.rgba = color2;
				}
				m_vertices.push_back(vtx);
			}
		}
	}

	m_pVertexBuffer = g.CreateVertexBufferWithSize(
		static_cast<UINT>(m_nVertexInUnit * sizeof(Vertex)), nullptr, true);
}
