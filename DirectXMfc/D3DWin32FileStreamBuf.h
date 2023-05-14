#pragma once
#include "D3D11Graphics.h"
#include <streambuf>

namespace D3D11Graphics {

class D3DWin32FileStreamBuf : public std::streambuf
{
private:
	typedef std::ios_base ios_base;
public:
	D3DWin32FileStreamBuf(size_t nBufferByte = 1024) : m_hFile(INVALID_HANDLE_VALUE), m_buffer(nBufferByte) {}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="hFile"></param>
	/// <param name="nBufferByte">buffer size</param>
	/// If nBufferByte is 0, data is directly read/write from the file.
	/// If nBufferByte is 0, underflow() and uflow() are not available.
	/// Then, some functions such as sgetc() are not available.
	explicit D3DWin32FileStreamBuf(HANDLE hFile, size_t nBufferByte = 1024)
		: m_hFile(hFile), m_buffer(nBufferByte)
	{
	}

	virtual ~D3DWin32FileStreamBuf();

	void AttachHandle(HANDLE hFile)
	{
		OnDetachHandle();
		m_hFile = hFile;
		ResetBuffer();
	}

	// utility functions
public:
	void Read(void* pBuffer, size_t nByte);
	size_t TryRead(void* pBuffer, size_t nByte);

	void Write(const void* pBuffer, size_t nByte);
	size_t TryWrite(const void* pBuffer, size_t nByte);

	// functions for read.
protected:
	virtual std::streamsize xsgetn(char_type* aBuffer, std::streamsize bufCount);
	virtual int_type underflow();
	virtual int_type uflow();

	// functions for write.
protected:
	virtual std::streamsize xsputn(const char_type* aBuffer, std::streamsize bufCount);
	virtual int sync();
	virtual int_type overflow(int_type nextValue);

	// seek functions
protected:
	virtual pos_type seekoff(
		off_type offset, ios_base::seekdir way, ios_base::openmode mode = ios_base::in | ios_base::out
	);
	virtual pos_type seekpos(pos_type pos, ios_base::openmode mode = ios_base::in | ios_base::out);

private:
	bool IsReadBufferEnabled() const { return eback() != nullptr; }
	bool IsWriteBufferEnabled() const { return pbase() != nullptr; }
	void OnDetachHandle();
	void ResetBuffer()
	{
		setp(nullptr, nullptr);
		setg(nullptr, nullptr, nullptr);
	}
	std::streamsize ReadFromFile(std::streamsize nBufferByte, char* aBufferByte);
	std::streamsize WriteToFile(std::streamsize nBufferByte, const char* aBufferByte);
	pos_type SeekFile(off_type offset, ios_base::seekdir way);
private:
	HANDLE m_hFile;
	std::vector<char> m_buffer;
};

}	// end of namespace D3D11Graphics
