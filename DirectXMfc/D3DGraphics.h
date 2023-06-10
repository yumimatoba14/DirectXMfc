#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "D3DShaderContext.h"
#include "D3DMappedSubResource.h"

namespace D3D11Graphics {

class D3DVertexBufferEnumerator;

class D3DGraphics
{
public:
	enum class DrawMode {
		DRAW_NORMAL = 0,
		DRAW_FOR_SELECTION,
		DRAW_SELECTED_ENTITY
	};

	static DXGIAdapterPtr SelectGraphicsAdapter();

	D3DGraphics();
	D3DGraphics(const D3DGraphics&) = delete;

	bool HasDevice() const { return m_pDevice.Get() != nullptr; }
	void Setup(HWND hWnd, const CSize& rectSize);

	/// <summary>
	/// return a number of points to be drawn by DrawPointList() or DrawPointLists().
	/// </summary>
	/// <returns>Number of points to be drawn after the last DrawBegin().</returns>
	int64_t GetDrawnPointCount() const { return m_nDrawnPoint; }

	// functions for Device.
public:
	template<class T>
	D3DBufferPtr CreateVertexBuffer(const T* aVertex, UINT nVertex)
	{
		return CreateVertexBuffer(aVertex, nVertex, false);
	}
	
	template<class T>
	D3DBufferPtr CreateVertexBuffer(const T* aVertex, UINT nVertex, bool writeByCpu)
	{
		return CreateVertexBufferWithSize(sizeof(T) * nVertex, aVertex, writeByCpu);
	}

	D3DBufferPtr CreateVertexBufferWithSize(UINT nByte, const void* aVertex, bool writeByCpu);

	D3DBufferPtr CreateIndexBuffer(const UINT* Index, UINT IndexNum);

	D3DBufferPtr CreateConstantBuffer(size_t nByte);
	template<class T>
	void SetConstantBufferData(const D3DBufferPtr& pCBuffer, const T& data) {
		m_pDC->UpdateSubresource(pCBuffer.Get(), 0, nullptr, &data, 0, 0);
	}

	D3DInputLayoutPtr CreateInputLayout(
		const D3D11_INPUT_ELEMENT_DESC* aElement, UINT nElement,
		const std::string& fileName, const std::string& entryPoint, const D3D_SHADER_MACRO* aMacro
	);
	D3DVertexShaderPtr CreateVertexShader(
		const std::string& fileName, const std::string& entryPoint, const D3D_SHADER_MACRO* aMacro
	);
	D3DGeometryShaderPtr CreateGeometryShader(
		const std::string& fileName, const std::string& entryPoint, const D3D_SHADER_MACRO* aMacro
	);
	D3DPixelShaderPtr CreatePixelShader(
		const std::string& fileName, const std::string& entryPoint, const D3D_SHADER_MACRO* aMacro
	);

public:
	D3DTexture2DPtr CaptureRenderTargetStagingTexture(const D3DRenderTargetViewPtr& pRenderTarget);
	D3DMappedSubResource MapDyamaicBuffer(const D3DBufferPtr& pDynamicBuffer);
	D3DMappedSubResource MapStagingBuffer(
		const D3DResourcePtr& pBuffer, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE* pMappedSubResource
	);

	// functions for DeviceContext.
public:
	DrawMode GetDrawMode() const { return m_drawMode; }
	bool IsSelectionMode() const { return m_drawMode == DrawMode::DRAW_FOR_SELECTION; }
	void SetDrawSelectedEntityMode(bool isSelectedEntityMode);

	void DrawBegin(bool isForSelection, bool isEraseBackground);
	void DrawPointList(
		const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, size_t vertexSize, size_t nVertex
	);
	void DrawPointList(
		const D3DShaderContext& sc, size_t nVertex,
		const D3DBufferPtr& pVertexBuf0, size_t vertexSize0,
		const D3DBufferPtr& pVertexBuf1, size_t vertexSize1
	);
	void DrawPointLists(
		const D3DShaderContext& sc, D3DVertexBufferEnumerator* pVertexBufs, size_t vertexSize
	);
	void DrawLineStrip(
		const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, size_t vertexSize, size_t nVertex
	);
	void DrawTriangleList(
		const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, const D3DBufferPtr& pIndexBuf,
		size_t vertexSize, size_t nIndex
	);
	void SetShaderContext(const D3DShaderContext& context);
	D3DRenderTargetViewPtr DrawEnd();

	void ResizeBuffers(const CSize& newSize);
	bool SaveViewToFile(REFGUID targetFormat, LPCTSTR targetFilePath);

private:
	void PrepareDepthStencilView();
	void PrepareRenderTargetView();
	void PrepareRenderTargetViewForSelection();

private:
	DXGIAdapterPtr m_pAdapter;

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDC;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;

	D3DDepthStencilStatePtr m_pDepthStencilState;
	D3DDepthStencilStatePtr m_pDepthStencilStateForSelectedEntity;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	D3D11_VIEWPORT m_viewport = { 0, 0, 0, 0, 0, 0 };

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_pRasterizerState;

	DrawMode m_drawMode = DrawMode::DRAW_NORMAL;
	int64_t m_nDrawnPoint = 0;
};

} // namespace D3D11Graphics
