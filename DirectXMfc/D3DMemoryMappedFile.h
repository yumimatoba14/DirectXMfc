#pragma once

#include "D3D11Graphics.h"
#include <utility>

namespace D3D11Graphics {

class D3DMemoryMappedFile
{
public:
	/// <summary>
	/// RAII class to have a meory allocated by MapViewOfFile().
	/// </summary>
	class MappedPtr {
	public:
		MappedPtr() noexcept : MappedPtr(nullptr, 0) {}
		MappedPtr(void* pViewBegin, DWORD dataOffset) noexcept
			: m_pData(pViewBegin), m_dataOffsetByte(dataOffset)
		{}
		MappedPtr(const MappedPtr&) = delete;
		MappedPtr(MappedPtr&& other) noexcept : MappedPtr() { swap(*this, other); }
		/*virtual*/~MappedPtr();

		MappedPtr& operator = (const MappedPtr&) = delete;
		MappedPtr& operator = (MappedPtr&& other) noexcept
		{
			// It is undesired that old value of this would be returned with 'other'.
			// So create new instance here.
			MappedPtr p(std::move(other));
			swap(*this, p);
			return *this;
		}

		void* Get() const { return IsNull() ? nullptr : (static_cast<char*>(m_pData) + m_dataOffsetByte); }

		template<class T>
		T* ToArray() const { return (T*)Get(); }

		template<class T>
		const T* ToConstArray() const { return (T*)Get(); }

		operator bool() { return !IsNull(); }
		bool IsNull() const { return m_pData == nullptr; }

		// It is assumed that this function is called via ADL.
		friend inline void swap(MappedPtr& left, MappedPtr& right) noexcept
		{
			swap(left.m_pData, right.m_pData);
			swap(left.m_dataOffsetByte, right.m_dataOffsetByte);
		}
	private:
		void Close();

	private:
		void* m_pData = nullptr;
		DWORD m_dataOffsetByte;
	};
	 
	enum Mode {
		MODE_READ,
		MODE_READ_WRITE,
		MODE_NEW_FILE,
		MODE_TEMP_FILE
	};
public:
	D3DMemoryMappedFile() noexcept {}
	D3DMemoryMappedFile(const D3DMemoryMappedFile&) = delete;
	D3DMemoryMappedFile(D3DMemoryMappedFile&& other) noexcept
		: D3DMemoryMappedFile()
	{
		swap(*this, other);
	}

	/*virtual*/~D3DMemoryMappedFile();

	D3DMemoryMappedFile& operator = (const D3DMemoryMappedFile&) = delete;
	D3DMemoryMappedFile& operator = (D3DMemoryMappedFile&& other)
	{
		Close();
		swap(*this, other);
		return *this;
	}


	bool OpenToRead(LPCTSTR pFilePath) { return OpenImpl(pFilePath, MODE_READ, 0); }
	bool OpenNewFile(LPCTSTR pFilePath, uint64_t newFileSize)
	{
		return OpenImpl(pFilePath, MODE_NEW_FILE, newFileSize);
	}
	bool OpenTempFile(LPCTSTR pFilePath, uint64_t newFileSize)
	{
		return OpenImpl(pFilePath, MODE_TEMP_FILE, newFileSize);
	}
	void AttachFileToRead(HANDLE hFile)
	{
		AttachFileImpl(hFile, MODE_READ, 0);
	}
	/// <summary>
	/// 
	/// </summary>
	/// <param name="hFile"></param>
	/// <param name="fileSize">It can be 0 when this object modifies the current size of hFile.</param>
	void AttachFileToWrite(HANDLE hFile, uint64_t fileSize)
	{
		AttachFileImpl(hFile, MODE_READ_WRITE, fileSize);
	}

	void Close();

	bool IsOpened() const { return m_hFileMapping != INVALID_HANDLE_VALUE; }
	MappedPtr MapView(uint64_t filePos, size_t viewSize);

	friend inline void swap(D3DMemoryMappedFile& left, D3DMemoryMappedFile& right) noexcept
	{
		swap(left.m_hFileToClose, right.m_hFileToClose);
		swap(left.m_hFileMapping, right.m_hFileMapping);
		swap(left.m_allocationGranularity, right.m_allocationGranularity);
		swap(left.m_isWriteMode, right.m_isWriteMode);
	}

private:
	bool OpenImpl(LPCTSTR pFilePath, Mode mode, uint64_t fileSize);
	void AttachFileImpl(HANDLE hFile, Mode mode, uint64_t fileSize);
	bool CreateFileMapppingObject(HANDLE hFile, Mode mode, uint64_t fileSize);
private:
	HANDLE m_hFileToClose = INVALID_HANDLE_VALUE;
	HANDLE m_hFileMapping = nullptr;
	DWORD m_allocationGranularity = 0;
	bool m_isWriteMode = false;
};

} // end of namespace D3D11Graphics


