#include "pch.h"
#include "D3DExclusiveLodPointListBuilder.h"
#include "D3DExclusiveLodPointListCreator.h"

using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DExclusiveLodPointListBuilder::D3DExclusiveLodPointListBuilder(
	LPCTSTR pFilePath, double latticeLength
) : m_resultFilePath(pFilePath), m_latticeLength(latticeLength)
{
	P_IS_TRUE(0 < latticeLength);
	m_tempFile.OpenTempFile();
}

D3DExclusiveLodPointListBuilder::~D3DExclusiveLodPointListBuilder()
{
	P_NOEXCEPT_BEGIN("D3DExclusiveLodPointListBuilder::~D3DExclusiveLodPointListBuilder");
	BuildPointListFile();
	P_NOEXCEPT_END;
}

////////////////////////////////////////////////////////////////////////////////

void D3DExclusiveLodPointListBuilder::OnAddVertex(const Vertex& vertex)
{
	m_tempFile.Write(&vertex, sizeof(Vertex));
	m_pointAabb.Extend(D3DVector3d(vertex.pos));
	++m_nVertex;
}

////////////////////////////////////////////////////////////////////////////////

static double DecideLatticeLength(const D3DAabBox3d& pointAabb, int nDiv)
{
	P_IS_TRUE(0 < nDiv);
	D3DVector3d aabbSize = pointAabb.GetMaxPoint() - pointAabb.GetMinPoint();
	int longestAxis = 0;
	for (int i = 1; longestAxis < 3; ++i) {
		if (aabbSize[longestAxis] < aabbSize[i]) {
			longestAxis = i;
		}
	}

	return aabbSize[longestAxis] / nDiv;
}

void D3DExclusiveLodPointListBuilder::BuildPointListFile()
{
	if (!m_tempFile.IsOpend()) {
		return;
	}

#if 0
	m_latticeLength = DecideLatticeLength(m_pointAabb, 100);
#endif

	D3DExclusiveLodPointListCreator creator(m_latticeLength);
	creator.CreateImage(m_tempFile, m_nVertex, m_pointAabb, m_resultFilePath);

	m_tempFile.Close();
}
