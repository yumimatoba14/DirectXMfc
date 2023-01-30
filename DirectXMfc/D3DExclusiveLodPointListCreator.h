#pragma once
#include "D3DGraphics3D.h"
#include "D3DAabBox.h"

namespace D3D11Graphics {

class D3DMemoryMappedFile;
class D3DWin32File;

class D3DExclusiveLodPointListCreator
{
public:
	typedef D3DGraphics3D::PointListVertex Vertex;
	class MmFileCreator {
	public:
		virtual D3DMemoryMappedFile Create(uint64_t fileSize) = 0;
	};
public:
	D3DExclusiveLodPointListCreator(double latticeLength)
		: m_latticeLength(latticeLength)
	{}

	void CreateImage(
		D3DWin32File& inputVertexFile, int64_t nInputVertex,
		const D3DAabBox3d& vertexAabb,
		LPCTSTR pResultFilePath
	);
	int64_t CreateImage(
		D3DWin32File& inputVertexFile, int64_t inputVertexFileByteBegin, int64_t nInputVertex,
		const D3DAabBox3d& vertexAabb,
		HANDLE hResultFile, int64_t resultFileByteBegin
	);
private:
	int64_t CreateImageImpl(
		D3DWin32File& inputVertexFile, int64_t inputVertexFileByteBegin, int64_t nInputVertex,
		const D3DAabBox3d& vertexAabb,
		MmFileCreator& resultFileCreator, int64_t resultFileByteBegin
	);

	static uint64_t Build1Level(
		D3DWin32File& inputVertexFile, uint64_t nInputVertex,
		const D3DAabBox3d& pointAabb, double latticeLength,
		D3DMemoryMappedFile& resultFile, uint64_t resultFileBeginPos
	);
	static void CopyVertices(
		D3DWin32File& inputVertexFile, uint64_t nInputVertex,
		D3DMemoryMappedFile& resultFile, uint64_t resultFileBeginPos, uint64_t resultFileEndPos
	);

private:
	double m_latticeLength;
};

}	// end of namespace D3D11Graphics
