#include "pch.h"
#include "D3DWin32File.h"

using namespace D3D11Graphics;

static LARGE_INTEGER MakeLargeInteger(LONGLONG value)
{
	LARGE_INTEGER ret;
	ret.QuadPart = value;
	return ret;
}

////////////////////////////////////////////////////////////////////////////////

void D3DWin32File::OpenNewFile(LPCTSTR pFilePath)
{
	Close();

	DWORD dwDesiredAccess = FILE_GENERIC_READ | GENERIC_WRITE;
	DWORD dwCreationDisposition = CREATE_ALWAYS;
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	m_hFile.reset(::CreateFile(
		pFilePath, dwDesiredAccess, 0, nullptr,
		dwCreationDisposition, dwFlagsAndAttributes, nullptr
	));
	if (m_hFile.get() == INVALID_HANDLE_VALUE) {
		P_THROW_ERROR("CreateFile");
	}
}

static CString MakeTempFilePath()
{
	TCHAR aTempDir[_MAX_PATH];
	DWORD ret = ::GetTempPath(_MAX_PATH, aTempDir);
	if (ret == 0 || _MAX_PATH < ret) {
		P_THROW_ERROR("GetTempPath");
	}

	TCHAR aTempFilePath[_MAX_PATH];
	ret = ::GetTempFileName(aTempDir, _T("pnt"), 0, aTempFilePath);
	if (ret == 0) {
		P_THROW_ERROR("GetTempFileName");
	}
	return aTempFilePath;
}

void D3DWin32File::OpenTempFile(DWORD dwCreationDisposition)
{
	Close();

	CString tempFilePath = MakeTempFilePath();
	m_hFile.reset(::CreateFile(
		tempFilePath, FILE_GENERIC_READ | GENERIC_WRITE, 0, nullptr,
		dwCreationDisposition, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr
	));
	if (m_hFile.get() == INVALID_HANDLE_VALUE) {
		P_THROW_ERROR("CreateFile");
	}
}

////////////////////////////////////////////////////////////////////////////////

void D3DWin32File::Read(void* pBuffer, size_t nByte)
{
	P_ASSERT(nByte <= 0xFFFFFFFF);
	DWORD nReadByte = 0;
	BOOL isOk = ::ReadFile(m_hFile.get(), pBuffer, static_cast<DWORD>(nByte), &nReadByte, nullptr);
	if (!isOk) {
		P_THROW_ERROR("ReadFile");
	}
	P_IS_TRUE(nReadByte == nByte);
}

void D3DWin32File::Write(const void* pBuffer, size_t nByte)
{
	P_ASSERT(nByte <= 0xFFFFFFFF);
	DWORD nWrittenByte = 0;
	BOOL isOk = ::WriteFile(m_hFile.get(), pBuffer, static_cast<DWORD>(nByte), &nWrittenByte, nullptr);
	if (!isOk) {
		P_THROW_ERROR("WriteFile");
	}
	P_IS_TRUE(nWrittenByte == nByte);
}

/// <summary>
/// 
/// </summary>
/// <param name="distanceToMove"></param>
/// <param name="moveMethod">FILE_BEGIN, FILE_CURRENT or FILE_END</param>
/// <returns>New pointer position</returns>
int64_t D3DWin32File::SetFilePointer(int64_t distanceToMove, DWORD moveMethod)
{
	LARGE_INTEGER newPos = { 0 };
	BOOL isOk = ::SetFilePointerEx(m_hFile.get(), MakeLargeInteger(distanceToMove), &newPos, moveMethod);
	if (!isOk) {
		P_THROW_ERROR("SetFilePointerEx");
	}
	return newPos.QuadPart;
}

////////////////////////////////////////////////////////////////////////////////
