#pragma once
#include "D3D11Graphics.h"

namespace D3D11Graphics {

/// <summary>
/// This class wraps file pointer of Win32.
/// </summary>
///
/// This class uses int64_t to specify file position instead of LARGE_INTEGER.
class D3DWin32File
{
public:
	D3DWin32File() noexcept {}
	explicit D3DWin32File(HANDLE hFile) : m_hFile(hFile) {}

	void OpenNewFile(LPCTSTR pFilePath);
	void OpenTempFile(DWORD dwCreationDisposition = CREATE_ALWAYS);

	bool IsOpend() const { return m_hFile.get() != nullptr && m_hFile.get() != INVALID_HANDLE_VALUE; }

	void Read(void* pBuffer, size_t nByte);
	void Write(const void* pBuffer, size_t nByte);

	int64_t GetFilePointer() { return SetFilePointer(0, FILE_CURRENT); }
	int64_t SetFilePointer(int64_t distanceToMove, DWORD moveMethod);

	void Close() { m_hFile.reset(nullptr); }
	HANDLE GetHandle() const { return m_hFile.get(); }

private:
	D3DUniqueHandle m_hFile;
};

} // namespace D3D11Graphics
