#pragma once

#include "D3D11Graphics.h"

namespace D3D11Graphics {

class D3DMappedSubResource
{
public:
	D3DMappedSubResource() noexcept : m_pDC(nullptr), m_pSubResource(nullptr), m_aData(nullptr)
	{}
	D3DMappedSubResource(ID3DDeviceContextPtr pDC, ID3DResourcePtr pSubResource, void* pData) noexcept
		: m_pDC(std::move(pDC)), m_pSubResource(std::move(pSubResource)),
		m_aData(static_cast<char*>(pData))
	{}
	D3DMappedSubResource(const D3DMappedSubResource&) = delete;
	D3DMappedSubResource(D3DMappedSubResource&& other) noexcept : D3DMappedSubResource()
	{
		swap(*this, other);
	}

	/*virtual*/~D3DMappedSubResource();
	
	D3DMappedSubResource& operator = (const D3DMappedSubResource&) = delete;
	D3DMappedSubResource& operator = (D3DMappedSubResource&& other) noexcept
	{
		// It is undesired that old value of this would be returned with 'other'.
		// So create new instance here.
		D3DMappedSubResource tmp(std::move(other));
		swap(*this, tmp);
		return *this;
	}

	bool IsMapped() const { return m_pDC && m_pSubResource; }
	void Unmap();

	void Write(const void* pData, UINT dataByte);

	// It is assumed that this function is called via ADL.
	friend inline void swap(D3DMappedSubResource& left, D3DMappedSubResource& right) noexcept
	{
		left.m_pDC.Swap(right.m_pDC);
		left.m_pSubResource.Swap(right.m_pSubResource);
		swap(left.m_aData, right.m_aData);
	}

private:
	ID3DDeviceContextPtr m_pDC;
	ID3DResourcePtr m_pSubResource;
	char* m_aData;
};

}
