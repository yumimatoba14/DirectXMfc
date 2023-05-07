#pragma once

#include "D3D11Graphics.h"
#include <map>

namespace D3D11Graphics {

class D3DExclusiveLodPointListHeader
{
public:
	struct LevelInfo {
		double baseLength;
		uint64_t nPoint;

		LevelInfo() = default;
		LevelInfo(double length, uint64_t nPnt)
			: baseLength(length), nPoint(nPnt)
		{}
	};
public:
	D3DExclusiveLodPointListHeader();
	virtual ~D3DExclusiveLodPointListHeader();

	bool IsInitialized() const { return !m_levelInfos.empty(); }
	void ClearLevelInfo() { m_levelInfos.clear(); }
	void SetLevelInfo(double baseLength, uint64_t nPoint);
	uint64_t GetPointCount() const;
	uint64_t GetEnoughPointCount(double baseLength) const;
	double GetFirstLevelLength() const;
	double GetNextLevelLength(double currLength) const;

	// File image related functions.
public:
	size_t GetBinarySize() const;
	void WriteToBinaryData(char* aByte, size_t nByte) const;
	static size_t ReadHeaderSize(const char* aByte, size_t nByte);
	void ReadFromBinaryData(const char* aByte, size_t nByte);

private:
	std::map<double, LevelInfo, std::greater<double> > m_levelInfos;
};

} // namespace D3D11Graphics
