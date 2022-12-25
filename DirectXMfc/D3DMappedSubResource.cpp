#include "pch.h"
#include "D3DMappedSubResource.h"

using namespace D3D11Graphics;

D3DMappedSubResource::~D3DMappedSubResource()
{
	P_NOEXCEPT_BEGIN("D3DMappedSubResource::~D3DMappedSubResource");
	Unmap();
	P_NOEXCEPT_END;
}

void D3DMappedSubResource::Unmap()
{
	if (m_pDC && m_pSubResource) {
		m_pDC->Unmap(m_pSubResource.Get(), 0);
		m_pDC = nullptr;
		m_pSubResource = nullptr;
		m_aData = nullptr;
	}
	P_ASSERT(!IsMapped());
}

////////////////////////////////////////////////////////////////////////////////

void D3DMappedSubResource::Write(const void* pData, UINT dataByte)
{
	memcpy(m_aData, pData, dataByte);
}
