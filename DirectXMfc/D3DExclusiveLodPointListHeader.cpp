#include "pch.h"
#include "D3DExclusiveLodPointListHeader.h"

using namespace D3D11Graphics;

const size_t HEADER_SIZE = 16;
const size_t LEVEL_INFO_SIZE = 16;
const uint8_t FILE_HEADER = D3D_FILE_HEADER_EXCLUSIVE_LOD_POINT_LIST;
const uint8_t CURRENT_FILE_VERSION = 0;	// Increment number when file format has been changed.
const size_t MAX_LEVEL_COUNT = 255;

////////////////////////////////////////////////////////////////////////////////

D3DExclusiveLodPointListHeader::D3DExclusiveLodPointListHeader()
{
	P_IS_TRUE(sizeof(LevelInfo) == LEVEL_INFO_SIZE);
}

D3DExclusiveLodPointListHeader::~D3DExclusiveLodPointListHeader()
{
}

////////////////////////////////////////////////////////////////////////////////

void D3DExclusiveLodPointListHeader::SetLevelInfo(double baseLength, uint64_t nPoint)
{
	P_ASSERT(m_levelInfos.size() < MAX_LEVEL_COUNT);
	LevelInfo level{ baseLength, nPoint };
	m_levelInfos[baseLength] = level;
}

uint64_t D3DExclusiveLodPointListHeader::GetPointCount() const
{
	if (m_levelInfos.empty()) {
		return 0;
	}
	return m_levelInfos.rbegin()->second.nPoint;
}

uint64_t D3DExclusiveLodPointListHeader::GetEnoughPointCount(double baseLength) const
{
	P_IS_TRUE(!m_levelInfos.empty());
	auto itLb = m_levelInfos.lower_bound(baseLength);
	if (itLb == m_levelInfos.end()) {
		return m_levelInfos.rbegin()->second.nPoint;
	}
	return itLb->second.nPoint;
}

double D3DExclusiveLodPointListHeader::GetFirstLevelLength() const
{
	P_IS_TRUE(!m_levelInfos.empty());
	return m_levelInfos.begin()->first;
}

/// <summary>
/// Get the next level.
/// </summary>
/// <param name="currLength"></param>
/// <returns>
/// Base length of the next smaller level.
/// Return currLength if it is the last level.
/// </returns>
double D3DExclusiveLodPointListHeader::GetNextLevelLength(double currLength) const
{
	P_IS_TRUE(!m_levelInfos.empty());
	auto itLb = m_levelInfos.lower_bound(currLength);
	if (itLb == m_levelInfos.end()) {
		return m_levelInfos.rbegin()->first;
	}
	// NOTE: m_levelInfos is ordered in descending order.
	if (currLength > itLb->first) {
		return itLb->first;
	}
	++itLb;
	if (itLb == m_levelInfos.end()) {
		return m_levelInfos.rbegin()->first;
	}
	return itLb->first;
}

////////////////////////////////////////////////////////////////////////////////

static size_t GetBnarySize(size_t nLevel)
{
	return HEADER_SIZE + nLevel * LEVEL_INFO_SIZE;
}

size_t D3DExclusiveLodPointListHeader::GetBinarySize() const
{
	return GetBnarySize(m_levelInfos.size());
}

void D3DExclusiveLodPointListHeader::WriteToBinaryData(char* aByte, size_t nByte) const
{
	P_IS_TRUE(nByte == GetBinarySize());
	P_IS_TRUE(m_levelInfos.size() <= MAX_LEVEL_COUNT);
	uint8_t fileHeader = FILE_HEADER;
	uint8_t version = CURRENT_FILE_VERSION;
	uint8_t nLevel = static_cast<uint8_t>(m_levelInfos.size());

	ZeroMemory(aByte, nByte);
	aByte[0] = fileHeader;
	aByte[1] = version;
	aByte[2] = nLevel;
	// aByte[3-15] are not used now.

	char* pDestByte = aByte + HEADER_SIZE;
	// output in descending order of baseLength.
	for (auto it = m_levelInfos.begin(); it != m_levelInfos.end(); ++it) {
		const LevelInfo& level = it->second;
		memcpy(pDestByte, &level, LEVEL_INFO_SIZE);
		pDestByte += LEVEL_INFO_SIZE;
	}
}

size_t D3DExclusiveLodPointListHeader::ReadHeaderSize(const char* aByte, size_t nByte)
{
	P_IS_TRUE(HEADER_SIZE <= nByte);
	uint8_t fileHeader = aByte[0];
	uint8_t version = aByte[1];
	size_t nLevel = aByte[2];
	
	P_IS_TRUE(fileHeader == FILE_HEADER);
	P_IS_TRUE(version <= CURRENT_FILE_VERSION);
	return GetBnarySize(nLevel);
}

void D3DExclusiveLodPointListHeader::ReadFromBinaryData(const char* aByte, size_t nByte)
{
	P_IS_TRUE(HEADER_SIZE <= nByte);
	uint8_t fileHeader = aByte[0];
	uint8_t version = aByte[1];
	size_t nLevel = aByte[2];

	P_IS_TRUE(nByte == GetBnarySize(nLevel));

	const char* pSrcByte = aByte + HEADER_SIZE;
	m_levelInfos.clear();
	for (size_t iLevel = 0; iLevel < nLevel; ++iLevel) {
		LevelInfo level(0, 0);
		memcpy(&level, pSrcByte, LEVEL_INFO_SIZE);
		pSrcByte += LEVEL_INFO_SIZE;
		SetLevelInfo(level.baseLength, level.nPoint);
	}
}

////////////////////////////////////////////////////////////////////////////////
