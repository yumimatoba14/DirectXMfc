#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "D3DShaderContext.h"

namespace D3D11Graphics {

typedef Microsoft::WRL::ComPtr<ID3D11VertexShader> D3DVertexShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11GeometryShader> D3DGeometryShaderPtr;
typedef Microsoft::WRL::ComPtr<ID3D11PixelShader> D3DPixelShaderPtr;

class D3DGraphics
{
public:
	typedef Microsoft::WRL::ComPtr<IDXGIAdapter> DXGIAdapterPtr;
	typedef Microsoft::WRL::ComPtr<ID3D11Texture2D> D3D11Texture2DPtr;

	static DXGIAdapterPtr SelectGraphicsAdapter();

	D3DGraphics();

	bool HasDevice() const { return m_pDevice.Get() != nullptr; }
	void Setup(HWND hWnd, const CSize& rectSize);

	// functions for Device.
public:
	template<class T>
	D3DBufferPtr CreateVertexBuffer(const T* aVertex, UINT nVertex)
	{
		//頂点バッファ作成
		D3D11_BUFFER_DESC hBufferDesc;
		ZeroMemory(&hBufferDesc, sizeof(hBufferDesc));
		hBufferDesc.ByteWidth = sizeof(T) * nVertex;
		hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		hBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		hBufferDesc.CPUAccessFlags = 0;
		//hBufferDesc.MiscFlags = 0;
		//hBufferDesc.StructureByteStride = sizeof(float);

		D3D11_SUBRESOURCE_DATA hSubResourceData;
		ZeroMemory(&hSubResourceData, sizeof(hSubResourceData));
		hSubResourceData.pSysMem = aVertex;
		//hSubResourceData.SysMemPitch = 0;
		//hSubResourceData.SysMemSlicePitch = 0;

		D3DBufferPtr pBuffer;
		HRESULT hr = m_pDevice->CreateBuffer(&hBufferDesc, &hSubResourceData, &pBuffer);
		if (FAILED(hr)) {
			P_THROW_ERROR("CreateBuffer");
		}
		return pBuffer;
	}

	D3DBufferPtr CreateIndexBuffer(const UINT* Index, UINT IndexNum);

	D3DBufferPtr D3DGraphics::CreateConstantBuffer(size_t nByte);
	template<class T>
	void SetConstantBufferData(const D3DBufferPtr& pCBuffer, const T& data) {
		m_pDC->UpdateSubresource(pCBuffer.Get(), 0, nullptr, &data, 0, 0);
	}

	D3DInputLayoutPtr CreateInputLayout(
		const D3D11_INPUT_ELEMENT_DESC* aElement, UINT nElement,
		const std::string& fileName, const std::string& entryPoint
	);
	D3DVertexShaderPtr CreateVertexShader(const std::string& fileName, const std::string& entryPoint);
	D3DGeometryShaderPtr CreateGeometryShader(const std::string& fileName, const std::string& entryPoint);
	D3DPixelShaderPtr CreatePixelShader(const std::string& fileName, const std::string& entryPoint);

	// functions for DeviceContext.
public:
	void DrawBegin();
	void DrawTriangleList(
		const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, const D3DBufferPtr& pIndexBuf,
		size_t nVertex, size_t nIndex
	);
	void SetShaderContext(const D3DShaderContext& context);
	void DrawEnd();

	void ResizeBuffers(const CSize& newSize);

private:
	void PrepareDepthStencilView();
	void PrepareRenderTargetView();

private:
	DXGIAdapterPtr m_pAdapter;

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDC;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDepthStencilState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	D3D11_VIEWPORT m_viewport = { 0, 0, 0, 0, 0, 0 };

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterizerState;
};

} // namespace D3D11Graphics
