#include "pch.h"
#include "D3DBinaryFormatter.h"
#include <iterator>

using namespace std;
using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DBinaryFormatter::D3DBinaryFormatter(size_t nByte, const char* aByte)
	: D3DBinaryFormatter()
{
	m_bytes.assign(aByte + 0, aByte + nByte);
}

////////////////////////////////////////////////////////////////////////////////

void D3DBinaryFormatter::WriteByte(size_t nByte, const char* aByte)
{
	size_t nWrite = nByte;
	if (m_currentPos < m_bytes.size()) {
		if (m_bytes.size() < m_currentPos + nWrite) {
			nWrite = m_bytes.size() - m_currentPos;
		}
		copy(aByte + 0, aByte + nWrite, m_bytes.begin() + m_currentPos);
		m_currentPos += nWrite;
		nWrite = nByte - nWrite;
	}
	if (0 < nWrite) {
		copy(aByte + (nByte - nWrite), aByte + nByte, back_inserter(m_bytes));
		m_currentPos += nWrite;
	}
}

void D3DBinaryFormatter::ReadByte(size_t nByte, char* aByte)
{
	size_t endPos = m_currentPos + nByte;
	P_IS_TRUE(endPos <= m_bytes.size());
	copy(m_bytes.begin() + m_currentPos, m_bytes.begin() + endPos, aByte);
	m_currentPos = endPos;
}

////////////////////////////////////////////////////////////////////////////////
