#pragma once
#include "D3DGraphics3D.h"

namespace D3D11Graphics {

class D3DMemoryMappedFile;

class D3DPointBlockListHeader
{
public:
	struct BlockImage {
		D3DGraphics3D::XMFLOAT4X4 localToGlobalMatrix;
		double aAabbPoint[2][3];	// aAabbPoint[0] : min point, aAabbPoint[1] : max point.
		int64_t firstBytePos = 0;
		int64_t nImageByte = 0;
	};

	void ReadFromFile(D3DMemoryMappedFile& mmFile);

	const std::vector<BlockImage>& GetBlockList() const { return m_blockList; }
private:
	std::vector<BlockImage> m_blockList;
};

}	// end of namespace D3D11Graphics
