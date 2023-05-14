#include "pch.h"
#include "D3DWin32FileStreamBuf.h"

using namespace std;
using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DWin32FileStreamBuf::~D3DWin32FileStreamBuf()
{
	P_NOEXCEPT_BEGIN("D3DWin32FileStreamBuf::~D3DWin32FileStreamBuf");
	OnDetachHandle();
	P_NOEXCEPT_END;
}

////////////////////////////////////////////////////////////////////////////////

void D3DWin32FileStreamBuf::Read(void* pBuffer, size_t nByte)
{
	size_t nRead = TryRead(pBuffer, nByte);
	if (nRead != nByte) {
		P_THROW_ERROR("Failed to read enough data.");
	}
}

size_t D3DWin32FileStreamBuf::TryRead(void* pBuffer, size_t nByte)
{
	P_IS_TRUE(nByte < (1ui64 << 32));	// This function doesn't support over 32bit integer.
	return (size_t)sgetn(reinterpret_cast<char_type*>(pBuffer), nByte);
}

void D3DWin32FileStreamBuf::Write(const void* pBuffer, size_t nByte)
{
	size_t nWrite = TryWrite(pBuffer, nByte);
	if (nWrite != nByte) {
		P_THROW_ERROR("Failed to write enough data.");
	}
}

size_t D3DWin32FileStreamBuf::TryWrite(const void* pBuffer, size_t nByte)
{
	P_IS_TRUE(nByte < (1ui64 << 32));	// This function doesn't support over 32bit integer.
	return (size_t)sputn(reinterpret_cast<const char_type*>(pBuffer), nByte);
}

////////////////////////////////////////////////////////////////////////////////

std::streamsize D3DWin32FileStreamBuf::xsgetn(char_type* aOutByte, std::streamsize nOutByte)
{
	streamsize streamBufferSize = static_cast<streamsize>(m_buffer.size());
	streamsize nCopied = 0;
	while (nCopied < nOutByte) {
		streamsize nRequiredByte = nOutByte - nCopied;
		streamsize nByteInBuf = egptr() - gptr();
		if (0 < nByteInBuf) {
			// Copy data from buffer.
			streamsize nCopy = min(nByteInBuf, nRequiredByte);
			if (INT_MAX < nCopy) {
				// gbump() supports only int. So maximum number here shall be INT_MAX.
				nCopy = INT_MAX;
			}
			traits_type::copy(aOutByte + nCopied, gptr(), (int)nCopy);
			gbump((int)nCopy);
			nCopied += nCopy;
		}
		else {
			if (nRequiredByte < streamBufferSize) {
				// Read to the buffer and copy from buffer at the next turn.
				if (traits_type::eq_int_type(underflow(), traits_type::eof())) {
					// undeflow() didn't work.
					break;
				}
			}
			else {
				// Read to the output buffer directly.
				streamsize nCopy = ReadFromFile(nRequiredByte, aOutByte + nCopied);
				nCopied += nCopy;
				if (nCopy < nRequiredByte) {
					// If data is not read enough, return from this function call.
					break;
				}
			}
		}
	}
	return nCopied;
}

D3DWin32FileStreamBuf::int_type D3DWin32FileStreamBuf::underflow()
{
	// This function may enable input buffer. So output buffer must have been disabled.
	P_IS_TRUE(!IsWriteBufferEnabled());
	P_ASSERT(gptr() == egptr());
	if (m_buffer.empty()) {
		// sgetc() may call underflow() in this case.
		// Add a buffer for single character to hold current position.
		// In any case, small buffer is not used by xsgetn() or xsputn().
		m_buffer.resize(1);
	}
	streamsize streamBufferSize = static_cast<streamsize>(m_buffer.size());
	streamsize nRead = ReadFromFile(streamBufferSize, m_buffer.data());
	if (nRead == 0) {
		return traits_type::eof();
	}
	setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + nRead);
	return traits_type::to_int_type(*gptr());
}

D3DWin32FileStreamBuf::int_type D3DWin32FileStreamBuf::uflow()
{
	if (m_buffer.empty()) {
		// implementation for sbumpc() in the case of no buffer.
		P_ASSERT(gptr() == egptr());
		char_type nextByte = 0;
		streamsize nRead = ReadFromFile(1, &nextByte);
		if (nRead == 0) {
			return traits_type::eof();
		}
		return traits_type::to_int_type(nextByte);
	}
	else {
		// Use implementation of the base class which uses underflow().
		return streambuf::uflow();
	}
}

////////////////////////////////////////////////////////////////////////////////

std::streamsize D3DWin32FileStreamBuf::xsputn(const char_type* aBuffer, std::streamsize bufCount)
{
	// This function may enable output buffer. So input buffer must have been disabled.
	P_IS_TRUE(!IsReadBufferEnabled());

	if (pbase() == epptr() && !m_buffer.empty()) {
		// enable buffer
		setp(m_buffer.data(), m_buffer.data() + m_buffer.size());
	}

	if (pbase() < pptr() && pptr() == epptr()) {
		// There are some bytes in the buffer and the buffer is full.
		// Write the buffer to file in order to try to make some space in the buffer.
		streamsize nByteInBuf = pptr() - pbase();
		P_ASSERT(nByteInBuf <= SIZE_MAX);
		streamsize nWritten = WriteToFile(nByteInBuf, pbase());
		P_ASSERT(0 <= nWritten);
		if (nWritten == 0) {
			return 0;	// failed to write.
		}
		if (nWritten < nByteInBuf) {
			size_t nRemainingByte = static_cast<size_t>(nByteInBuf - nWritten);
			traits_type::move(pbase(), pbase() + nWritten, nRemainingByte);
			setp(pbase(), pbase() + nRemainingByte, epptr());
		}
		else {
			setp(pbase(), epptr());
		}
	}

	streamsize bufferSpace = epptr() - pptr();
	if (bufCount <= bufferSpace) {
		// If there are enough space in the buffer, write data to the buffer.
		streamsize i = bufCount;
		streamsize numCopied = 0;
		while (INT_MAX < i) {
			traits_type::copy(pptr(), aBuffer + numCopied, INT_MAX);
			pbump(INT_MAX);
			numCopied += INT_MAX;
			i -= INT_MAX;
		}
		traits_type::copy(pptr(), aBuffer + numCopied, static_cast<int>(i));
		pbump(static_cast<int>(i));
		return bufCount;
	}
	// There is no enough space in the buffer, write buffer data at first, and write aBuffer after that.

	if (pubsync()) {
		return 0;	// pubsync() was failed.
	}

	return WriteToFile(bufCount, aBuffer);
}

int D3DWin32FileStreamBuf::sync()
{
	if (pbase() < pptr()) {
		// write buffer to file.
		streamsize nByteInBuf = pptr() - pbase();
		streamsize nWritten = WriteToFile(nByteInBuf, pbase());
		if (nWritten < nByteInBuf) {
			return -1;	// failed to write.
		}
		setp(pbase(), epptr());
	}
	return 0;
}

D3DWin32FileStreamBuf::int_type D3DWin32FileStreamBuf::overflow(int_type nextValue)
{
	if (!traits_type::eq_int_type(nextValue, traits_type::eof())) {
		char_type c = traits_type::to_char_type(nextValue);
		streamsize nWritten = sputn(&c, 1);
		if (nWritten == 0) {
			return traits_type::eof();
		}
	}
	return traits_type::not_eof(nextValue);
}

////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// 
/// </summary>
/// <param name="offset"></param>
/// <param name="way"></param>
/// <param name="mode"></param>
/// <returns></returns>
/// This function disables both input and output buffers.
D3DWin32FileStreamBuf::pos_type D3DWin32FileStreamBuf::seekoff(
	off_type offset, ios_base::seekdir way, ios_base::openmode mode /*= ios_base::in | ios_base::out*/
)
{
	if (pbase() < pptr()) {
		if (pubsync()) {
			P_THROW_ERROR("pubsync() failed.");
		}
	}
	off_type fileOffset = offset;
	if (way == ios_base::cur) {
		if (IsReadBufferEnabled()) {
			// maintain offset value so as to cancel bytes
			// which are in the buffer and have not been read, yet.
			ptrdiff_t numByteInBuffer = egptr() - gptr();
			P_ASSERT(0 <= numByteInBuffer);
			fileOffset -= numByteInBuffer;
		}
	}
	ResetBuffer();

	return SeekFile(fileOffset, way);
}

D3DWin32FileStreamBuf::pos_type D3DWin32FileStreamBuf::seekpos(
	pos_type pos, ios_base::openmode mode /*= ios_base::in | ios_base::out*/
)
{
	return seekoff(pos, ios_base::beg, mode);
}

////////////////////////////////////////////////////////////////////////////////

void D3DWin32FileStreamBuf::OnDetachHandle()
{
	if (pubsync()) {
		P_THROW_ERROR("pubsync");
	}
	m_hFile = nullptr;
}

std::streamsize D3DWin32FileStreamBuf::ReadFromFile(std::streamsize nBufferByte, char* aBufferByte)
{
	P_ASSERT(sizeof(streamsize) == 8);
	streamsize nRemainingByte = nBufferByte;
	streamsize nReadByte = 0;
	while (0 < nRemainingByte) {
		DWORD bufSize = static_cast<DWORD>(nRemainingByte);
		if (DWORD_MAX < nRemainingByte) {
			bufSize = DWORD_MAX;
		}
		DWORD nReadByteLocal = 0;
		BOOL isOk = ::ReadFile(m_hFile, aBufferByte + nReadByte, bufSize, &nReadByteLocal, nullptr);
		if (!isOk) {
			P_THROW_ERROR("ReadFile");
		}
		nReadByte += nReadByteLocal;
		if (nReadByteLocal != bufSize) {
			break;
		}
		nRemainingByte -= nReadByteLocal;
	}
	return nReadByte;
}

std::streamsize D3DWin32FileStreamBuf::WriteToFile(std::streamsize nBufferByte, const char* aBufferByte)
{
	P_ASSERT(sizeof(streamsize) == 8);
	streamsize nRemainingByte = nBufferByte;
	streamsize nWrittenByte = 0;
	while (0 < nRemainingByte) {
		DWORD bufSize = static_cast<DWORD>(nRemainingByte);
		if (DWORD_MAX < nRemainingByte) {
			bufSize = DWORD_MAX;
		}
		DWORD nWritten = 0;
		BOOL isOk = ::WriteFile(m_hFile, aBufferByte + nWrittenByte, bufSize, &nWritten, nullptr);
		if (!isOk) {
			P_THROW_ERROR("WriteFile");
		}
		nWrittenByte += nWritten;
		if (nWritten != bufSize) {
			break;
		}
		nRemainingByte -= nWritten;
	}
	return nWrittenByte;
}

streambuf::pos_type D3DWin32FileStreamBuf::SeekFile(off_type offset, ios_base::seekdir way)
{
	DWORD method;
	switch (way) {
	case ios_base::beg: method = FILE_BEGIN; break;
	case ios_base::cur: method = FILE_CURRENT; break;
	case ios_base::end: method = FILE_END; break;
	default:
		P_THROW_ERROR("Not supported seekdir.");
	}

	LARGE_INTEGER offsetLI;
	offsetLI.QuadPart = offset;
	LARGE_INTEGER newPosLI;
	BOOL isOk = ::SetFilePointerEx(m_hFile, offsetLI, &newPosLI, method);
	if (!isOk) {
		P_THROW_ERROR("SetFilePointerEx");
	}
	return streampos(newPosLI.QuadPart);
}

////////////////////////////////////////////////////////////////////////////////
