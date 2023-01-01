#include "pch.h"
#include "D3DMemoryMappedFile.h"

using namespace D3D11Graphics;

////////////////////////////////////////////////////////////////////////////////

D3DMemoryMappedFile::~D3DMemoryMappedFile()
{
	P_NOEXCEPT_BEGIN("D3DMemoryMappedFile::~D3DMemoryMappedFile");
	Close();
	P_NOEXCEPT_END;
}

////////////////////////////////////////////////////////////////////////////////

static LARGE_INTEGER ToLargeInteger(LONGLONG input)
{
	LARGE_INTEGER val;
	val.QuadPart = input;
	return val;
}

bool D3DMemoryMappedFile::OpenImpl(LPCTSTR pFilePath, Mode mode, uint64_t fileSize)
{
	P_IS_TRUE(pFilePath);

	DWORD desiredAccess = GENERIC_READ;
	DWORD shareMode = FILE_SHARE_READ;
	DWORD creationDisposition = OPEN_EXISTING;
	DWORD fileAttrs = FILE_ATTRIBUTE_NORMAL;
	switch (mode) {
	case MODE_READ:
		break;
	case MODE_NEW_FILE:
	case MODE_TEMP_FILE:
		desiredAccess = GENERIC_READ | GENERIC_WRITE;
		creationDisposition = CREATE_ALWAYS;
		if (mode == MODE_TEMP_FILE) {
			fileAttrs = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
		}
		break;
	case MODE_READ_WRITE:
		P_THROW_ERROR("Not supported value.");
	default:
		P_ASSERT(false);	// TODO: add correct cases.
	}

	Close();

	m_hFileToClose = CreateFile(
		pFilePath, desiredAccess, shareMode, NULL,
		creationDisposition, fileAttrs, NULL
	);
	if (m_hFileToClose == INVALID_HANDLE_VALUE) {
		P_WRITE_LOG("CreateFile");
		return false;
	}

	return CreateFileMapppingObject(m_hFileToClose, mode, fileSize);
}

/// <summary>
/// 
/// </summary>
/// <param name="hFile">File to be mapped. This object doesn't close this file handle.</param>
/// <param name="mode"></param>
/// <param name="fileSize"></param>
void D3DMemoryMappedFile::AttachFileImpl(HANDLE hFile, Mode mode, uint64_t fileSize)
{
	P_IS_TRUE(hFile != INVALID_HANDLE_VALUE && hFile != nullptr);;
	Close();

	bool isOk = CreateFileMapppingObject(hFile, mode, fileSize);
	if (!isOk) {
		P_THROW_ERROR("CreateFileMapppingObject");
	}
}

bool D3DMemoryMappedFile::CreateFileMapppingObject(HANDLE hFile, Mode mode, uint64_t fileSize)
{
	if (m_allocationGranularity == 0) {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		m_allocationGranularity = sysInfo.dwAllocationGranularity;
	}

	DWORD pageProtect = PAGE_READONLY;
	m_isWriteMode = false;
	switch (mode) {
	case MODE_READ:
		break;
	case MODE_NEW_FILE:
	case MODE_TEMP_FILE:
		P_IS_TRUE(0 < static_cast<int64_t>(fileSize));
		//break; // fall through
	case MODE_READ_WRITE:
		pageProtect = PAGE_READWRITE;
		m_isWriteMode = true;
		break;
	default:
		P_ASSERT(false);	// TODO: add correct cases.
	}

	m_hFileMapping = ::CreateFileMapping(
		hFile, nullptr, pageProtect, DWORD(fileSize >> 32), (DWORD)fileSize, nullptr
	);
	if (m_hFileMapping == nullptr) {
		P_WRITE_LOG("CreateFileMapping");
		return false;
	}

	return true;
}

void D3DMemoryMappedFile::Close()
{
	if (m_hFileMapping != nullptr) {
		::CloseHandle(m_hFileMapping);
		m_hFileMapping = nullptr;
	}
	if (m_hFileToClose != INVALID_HANDLE_VALUE) {
		::CloseHandle(m_hFileToClose);
		m_hFileToClose = INVALID_HANDLE_VALUE;
	}
}

D3DMemoryMappedFile::MappedPtr D3DMemoryMappedFile::MapView(uint64_t filePos, size_t nDataByte)
{
	uint64_t viewBegin = (filePos / m_allocationGranularity) * m_allocationGranularity;
	DWORD offsetByte = static_cast<DWORD>(filePos - viewBegin);
	size_t viewSize = offsetByte + nDataByte;
	DWORD desiredAccess = FILE_MAP_READ;
	if (m_isWriteMode) {
		desiredAccess = FILE_MAP_WRITE;
	}
	MappedPtr pMemory(::MapViewOfFile(
		m_hFileMapping, desiredAccess, DWORD(viewBegin >> 32), DWORD(viewBegin), viewSize
	), offsetByte);
	return pMemory;
}

////////////////////////////////////////////////////////////////////////////////

D3DMemoryMappedFile::MappedPtr::~MappedPtr()
{
	P_NOEXCEPT_BEGIN("D3DMemoryMappedFile::MappedPtr::~MappedPtr");
	Close();
	P_NOEXCEPT_END;
}

void D3DMemoryMappedFile::MappedPtr::Close()
{
	if (m_pData) {
		BOOL isOk = ::UnmapViewOfFile(m_pData);
		if (!isOk) {
			P_THROW_ERROR("UnmapViewOfFile");
		}
		m_pData = nullptr;
		m_dataOffsetByte = 0;
	}
}
