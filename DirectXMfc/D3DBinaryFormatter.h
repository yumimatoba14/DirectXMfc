#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {

class D3DBinaryFormatter
{
public:
	D3DBinaryFormatter() : m_bitFlags(0), m_currentPos(0) {}
	D3DBinaryFormatter(size_t nByte, const char* aByte);

	uint32_t GetBitFlags() const { return m_bitFlags; }
	void SetBitFlags(uint32_t value) { m_bitFlags = value; }
	const std::vector<char>& GetBinaryData() const { return m_bytes; }

	size_t GetCurrentPosition() const { return m_currentPos; }
	void SetCurrentPosition(size_t pos) { m_currentPos = pos; }

	void WriteByte(size_t nByte, const char* aByte);
	void ReadByte(size_t nByte, char* aByte);
public:
	void WriteInt8(int8_t value) { WriteRawByte(1, (char*)&value); }
	int8_t ReadInt8() { return ReadInteger<int8_t>(1); }

	void WriteInt32(int32_t value) { WriteRawByte(4, (char*)&value); }
	int32_t ReadInt32() { return ReadInteger<int32_t>(4); }

	void WriteInt64(int64_t value) { WriteRawByte(8, (char*)&value); }
	int64_t ReadInt64() { return ReadInteger<int64_t>(8); }

private:
	template<class T>
	void WriteRawByte(size_t nByte, const T* pValue) { WriteByte(nByte, (const char*)pValue); }
	template<class T>
	T ReadInteger(size_t nByte)
	{
		T value = 0;
		ReadByte(nByte, (char*)&value);
		return value;
	}

private:
	uint32_t m_bitFlags;
	size_t m_currentPos;
	std::vector<char> m_bytes;
};

}	// end of namespace D3D11Graphics
