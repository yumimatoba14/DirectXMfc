#pragma once
#include "D3DGraphics3D.h"
#include "D3DAabBox.h"
#include "D3DWin32File.h"
#include "D3DWin32FileStreamBuf.h"
#include "D3DMemoryMappedFile.h"

namespace D3D11Graphics {

class D3DPointBlockListBuilder
{
public:
	typedef D3DGraphics3D::PointListVertex Vertex;
	struct BlockData {
		int64_t blockByteBegin = 0;
		int64_t nVertex = 0;
		D3DAabBox3d aabb;
	};

public:
	explicit D3DPointBlockListBuilder(LPCTSTR pFilePath);
	virtual ~D3DPointBlockListBuilder();

	void AddVertex(const Vertex& vertex);

	uint64_t GetVertexCount() const { return m_nVertex; }

public:
	void BuildPointBlockFile();

private:
	static void Build1Level(
		D3DWin32File& inputVertexFile, uint64_t nInputVertex, const D3DAabBox3d& pointAabb,
		uint64_t targetVertexCount,
		D3DWin32File& resultFile, int64_t resultFileByteBegin, std::vector<BlockData>* pResultBlockList
	);

private:
	uint64_t m_nVertex = 0;
	CString m_resultFilePath;
	D3DAabBox3d m_pointAabb;
	D3DWin32File m_inputVertexFile;
	D3DWin32FileStreamBuf m_inputVertexStreamBuf;
};

} // end of namespace D3D11Graphics
