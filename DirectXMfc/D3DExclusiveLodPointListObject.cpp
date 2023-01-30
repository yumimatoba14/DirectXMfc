#include "pch.h"
#include "D3DExclusiveLodPointListObject.h"
#include "D3DMemoryMappedFile.h"

using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DExclusiveLodPointListObject::D3DExclusiveLodPointListObject(D3DMemoryMappedFile& imageFile, int64_t imagePos)
	: m_imageFile(imageFile), m_imageByteBegin(imagePos)
{

}

////////////////////////////////////////////////////////////////////////////////

// Prepare object for drawing.
// This function was used to mark this object re-drawn in the current frame
// but actullay be drawn in the following frame.
void D3DExclusiveLodPointListObject::PrepareFirstDraw(D3DGraphics3D& g)
{
	if (!m_pointListHeader.IsInitialized()) {
		size_t dataSize = 32;
		auto pSrc = m_imageFile.MapView(m_imageByteBegin, dataSize);
		size_t nHeaderByte = D3DExclusiveLodPointListHeader::ReadHeaderSize(
			pSrc.ToConstArray<char>(), dataSize);
		if (dataSize < nHeaderByte) {
			pSrc = m_imageFile.MapView(m_imageByteBegin, nHeaderByte);
		}
		m_pointListHeader.ReadFromBinaryData(pSrc.ToConstArray<char>(), nHeaderByte);
		m_pointByteBegin = m_imageByteBegin + nHeaderByte;

		P_IS_TRUE(m_pointListHeader.IsInitialized());
	}

	m_nextVertex = 0;
}

////////////////////////////////////////////////////////////////////////////////

void D3DExclusiveLodPointListObject::OnDrawTo(D3DGraphics3D& g)
{
	if (!m_pointListHeader.IsInitialized()) {
		PrepareFirstDraw(g);
	}

	if (g.IsProgressiveViewMode()) {
		if (!g.IsProgressiveViewFollowingFrame()) {
			m_nextVertex = 0;
		}
		if (m_nextVertex == 0) {
			m_precisionForFrame = m_pointListHeader.GetFirstLevelLength();
		}
		else {
			if (m_drawingPrecision < m_precisionForFrame) {
				// Delay drawing the next level and prefer to draw other objects.
				uint64_t endVertexInLevel = m_pointListHeader.GetEnoughPointCount(m_precisionForFrame);
				if (endVertexInLevel <= m_nextVertex) {
					// drawing the next level.
					m_precisionForFrame = m_pointListHeader.GetNextLevelLength(m_precisionForFrame);
				}
			}
		}
	}
	else {
		m_nextVertex = 0;
		m_precisionForFrame = m_drawingPrecision;
	}
	// m_precisionForFrame has been decided.

	uint64_t endVertexInLevel = m_pointListHeader.GetEnoughPointCount(m_precisionForFrame);
	if (g.IsProgressiveViewMode()) {
		endVertexInLevel = min(endVertexInLevel, m_nextVertex + m_maxPointCountDrawnPerFrame);
	}
	const uint64_t nVertexPerCall = UINT_MAX / sizeof(Vertex);
	while (m_nextVertex < endVertexInLevel) {
		uint64_t endVertex = min(m_nextVertex + nVertexPerCall, endVertexInLevel);
		int64_t nVertex = static_cast<int64_t>(endVertex - m_nextVertex);
		UINT dataSize = static_cast<UINT>(nVertex * sizeof(Vertex));
		auto pSrc = m_imageFile.MapView(m_pointByteBegin + m_nextVertex * sizeof(Vertex), dataSize);
		g.DrawPointArray(pSrc.ToConstArray<Vertex>(), nVertex);

		m_nextVertex = endVertex;
	}
}

////////////////////////////////////////////////////////////////////////////////
