#include "pch.h"
#include "D3DPointBlockListHeader.h"
#include "D3DBinaryFormatter.h"
#include "D3DMemoryMappedFile.h"

using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

void D3DPointBlockListHeader::ReadFromFile(D3DMemoryMappedFile& mmFile)
{
	P_IS_TRUE(mmFile.IsOpened());
	m_blockList.clear();

	size_t nReadByte = 128;
	auto pMem = mmFile.MapView(0, nReadByte);

	size_t nHeaderByte = 10;
	D3DBinaryFormatter formatter(nHeaderByte, pMem.ToArray<char>());
	formatter.SetBitFlags(0);
	P_IS_TRUE(formatter.ReadInt8() == D3D_FILE_HEADER_POINT_BLOCK_LIST);
	int8_t fileVersion = formatter.ReadInt8();
	formatter.SetBitFlags(formatter.ReadInt32());
	size_t nBlock = formatter.ReadInt32();
	P_IS_TRUE(formatter.GetCurrentPosition() == nHeaderByte);

	m_blockList.resize(nBlock);

	size_t nBlockImageListByte = sizeof(BlockImage) * nBlock;
	if (nReadByte < nHeaderByte + nBlockImageListByte) {
		pMem = mmFile.MapView(0, nHeaderByte + nBlockImageListByte);
	}

	formatter = D3DBinaryFormatter(nBlockImageListByte, pMem.ToArray<char>() + nHeaderByte);
	for (size_t iBlock = 0; iBlock < nBlock; ++iBlock) {
		formatter.ReadByte(sizeof(BlockImage), (char*)&(m_blockList[iBlock]));
	}
}
