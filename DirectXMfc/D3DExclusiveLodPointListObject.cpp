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

bool D3DExclusiveLodPointListObject::FindPointBySelectionTargetId(D3DSelectionTargetId id, Vertex* pFoundVertex) const
{
	if (m_pointStIdFirst != D3D_SELECTION_TARGET_NULL) {
		uint64_t pointNum = m_pointListHeader.GetPointCount();
		bool isFound = m_pointStIdFirst <= id && id < m_pointStIdFirst + pointNum;
		if (isFound && pFoundVertex != nullptr) {
			int64_t vertexIndex = id - m_pointStIdFirst;
			auto pSrc = m_imageFile.MapView(m_pointByteBegin + vertexIndex * sizeof(Vertex), sizeof(Vertex));
			*pFoundVertex = pSrc.ToConstArray<Vertex>()[0];
		}
		return isFound;
	}
	return false;
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
	m_drawingVertexEnd = m_pointListHeader.GetEnoughPointCount(m_drawingPrecision);
}

/// This function can be called sometimes in a frame.
/// PrepareFirstDraw() must have been called beforehand to restart drawing from the begining.
void D3DExclusiveLodPointListObject::DrawAfterPreparation(D3DGraphics3D& g)
{
	P_ASSERT(m_pointListHeader.IsInitialized());

	if (g.IsProgressiveViewMode()) {
		if (m_nextVertex == 0) {
			m_precisionForFrame = m_pointListHeader.GetFirstLevelLength();
		}
		else {
			if (m_drawingPrecision < m_precisionForFrame) {
				// Delay drawing the next level and prefer to draw other objects.
				uint64_t endVertexInLevel = m_pointListHeader.GetEnoughPointCount(m_precisionForFrame);
				if (endVertexInLevel <= m_nextVertex) {
#if 1
					// drawing the next level.
					m_precisionForFrame = m_pointListHeader.GetNextLevelLength(m_precisionForFrame);
#else
					// draw the required level.
					m_precisionForFrame = m_drawingPrecision;
#endif
				}
			}
		}
	}
	else {
		m_precisionForFrame = m_drawingPrecision;
	}
	// m_precisionForFrame has been decided.

	uint64_t endVertexInLevel = m_pointListHeader.GetEnoughPointCount(m_precisionForFrame);
	if (g.IsProgressiveViewMode()) {
		endVertexInLevel = min(endVertexInLevel, m_nextVertex + m_maxPointCountDrawnPerFrame);
	}
#if D3D_IS_32BIT_MODULE
	const uint64_t nVertexPerCall = UINT_MAX / 24 / sizeof(Vertex);	// avoid large array.
#else
	const uint64_t nVertexPerCall = UINT_MAX / sizeof(Vertex);
#endif
	while (m_nextVertex < endVertexInLevel) {
		uint64_t endVertex = min(m_nextVertex + nVertexPerCall, endVertexInLevel);
		int64_t nVertex = static_cast<int64_t>(endVertex - m_nextVertex);
		UINT dataSize = static_cast<UINT>(nVertex * sizeof(Vertex));
		auto pSrc = m_imageFile.MapView(m_pointByteBegin + m_nextVertex * sizeof(Vertex), dataSize);
		D3DSelectionTargetId firstId = (m_pointStIdFirst == D3D_SELECTION_TARGET_NULL ? D3D_SELECTION_TARGET_NULL : m_pointStIdFirst + m_nextVertex);
		g.DrawPointArray(pSrc.ToConstArray<Vertex>(), nVertex, firstId);

		m_nextVertex = endVertex;
	}
}

////////////////////////////////////////////////////////////////////////////////

void D3DExclusiveLodPointListObject::OnDrawTo(D3DGraphics3D& g)
{
	if(!m_pointListHeader.IsInitialized() || !g.IsProgressiveViewFollowingFrame()) {
		PrepareFirstDraw(g);
	}

	DrawAfterPreparation(g);
}

////////////////////////////////////////////////////////////////////////////////
