#include "pch.h"
#include "D3DGraphics.h"
#include <exception>
#include "D3DVertexBufferEnumerator.h"
#include "ScreenGrab.h"

using namespace Microsoft::WRL;
using namespace std;
using namespace D3D11Graphics;

#define VS_COMPILE_TARGET "vs_5_0"

D3DGraphics::D3DGraphics()
{
	m_pAdapter = SelectGraphicsAdapter();
}

D3DGraphics::DXGIAdapterPtr D3DGraphics::SelectGraphicsAdapter()
{
	ComPtr<IDXGIFactory> pFactory;

	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), &pFactory);
	if (FAILED(hr))
	{
		P_THROW_ERROR("CreateDXGIFactory");
	}

	// Find the best GPU Output.
	UINT iChosenAdapter = 0;
	const UINT maxAdapter = 100;
	size_t maxGpuMemorySize = 0;
	for (UINT i = 0; i < maxAdapter; i++)
	{
		ComPtr<IDXGIAdapter> pAdd;
		hr = pFactory->EnumAdapters(i, &pAdd);
		if (FAILED(hr)) {
			break;
		}

		DXGI_ADAPTER_DESC adapterDesc;
		hr = pAdd->GetDesc(&adapterDesc);
		if (FAILED(hr)) {
			P_THROW_ERROR("GetDesc");
		}

		CString description = adapterDesc.Description;
		size_t videoCardMemorySize = adapterDesc.DedicatedVideoMemory;

		ComPtr<IDXGIAdapter> pAdapter;
		ComPtr<IDXGIOutput> pOutput;
		hr = pAdd->EnumOutputs(0, &pOutput);
		if (FAILED(hr))
		{
			continue;
		}

		UINT numModes = 0;
		hr = pOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
		if (FAILED(hr))
		{
			P_IGNORE_ERROR("GetDisplayModeList");
			continue;
		}

		if (videoCardMemorySize > maxGpuMemorySize)
		{
			maxGpuMemorySize = videoCardMemorySize;
			iChosenAdapter = i;
		}
	}

	ComPtr<IDXGIAdapter> pAddapter;
	hr = pFactory->EnumAdapters(iChosenAdapter, &pAddapter);
	if (FAILED(hr))
	{
		P_THROW_ERROR("EnumAdapters");
	}
	return pAddapter;
}

void D3DGraphics::Setup(HWND hWnd, const CSize& rectSize)
{
	UINT cdev_flags = 0;
#ifdef _DEBUG
	cdev_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = rectSize.cx;
	sd.BufferDesc.Height = rectSize.cy;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;	//1/60 = 60fps
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = true;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D_FEATURE_LEVEL aFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
	};

	// m_pDC is Immediate Device Context.
	HRESULT hr = D3D11CreateDeviceAndSwapChain(m_pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL,
		cdev_flags, aFeatureLevels, 1, D3D11_SDK_VERSION, &sd,
		&m_pSwapChain, &m_pDevice, NULL, &m_pDC);
	if (FAILED(hr)) {
		P_THROW_ERROR("D3D11CreateDeviceAndSwapChain");
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	hr = m_pDevice->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateDepthStencilState");
	}

	D3D11_DEPTH_STENCIL_DESC depthStencilForSelectedEntityDesc = depthStencilDesc;
	depthStencilForSelectedEntityDesc.DepthEnable = FALSE;
	hr = m_pDevice->CreateDepthStencilState(&depthStencilForSelectedEntityDesc, &m_pDepthStencilStateForSelectedEntity);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateDepthStencilState");
	}

	PrepareDepthStencilView();
	PrepareRenderTargetView();

	// viewport
	m_viewport.Width = static_cast<FLOAT>(rectSize.cx);
	m_viewport.Height = static_cast<FLOAT>(rectSize.cy);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;

	D3D11_RASTERIZER_DESC hRasterizerDesc = {
		D3D11_FILL_SOLID,
		D3D11_CULL_NONE,
		TRUE,
		0,
		0.0f,
		FALSE,
		FALSE,
		FALSE,
		FALSE,
		FALSE
	};
	hr = m_pDevice->CreateRasterizerState(&hRasterizerDesc, &m_pRasterizerState);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateRasterizerState");
	}
}

////////////////////////////////////////////////////////////////////////////////

D3DBufferPtr D3DGraphics::CreateVertexBufferWithSize(UINT nByte, const void* aVertex, bool writeByCpu)
{
	D3D11_BUFFER_DESC hBufferDesc;
	ZeroMemory(&hBufferDesc, sizeof(hBufferDesc));
	hBufferDesc.ByteWidth = nByte;
	hBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	if (writeByCpu) {
		hBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		hBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else {
		hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		hBufferDesc.CPUAccessFlags = 0;
	}
	//hBufferDesc.MiscFlags = 0;
	//hBufferDesc.StructureByteStride = sizeof(float);

	D3D11_SUBRESOURCE_DATA hSubResourceData;
	if (aVertex) {
		ZeroMemory(&hSubResourceData, sizeof(hSubResourceData));
		hSubResourceData.pSysMem = aVertex;
		//hSubResourceData.SysMemPitch = 0;
		//hSubResourceData.SysMemSlicePitch = 0;
	}

	D3DBufferPtr pBuffer;
	HRESULT hr = m_pDevice->CreateBuffer(&hBufferDesc, aVertex ? &hSubResourceData : nullptr, &pBuffer);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateBuffer");
	}
	return pBuffer;
}

D3DBufferPtr D3DGraphics::CreateIndexBuffer(const UINT* aIndex, UINT nIndex)
{
	D3D11_BUFFER_DESC hBufferDesc;
	ZeroMemory(&hBufferDesc, sizeof(hBufferDesc));
	hBufferDesc.ByteWidth = sizeof(UINT) * nIndex;
	hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	hBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	hBufferDesc.CPUAccessFlags = 0;
	//hBufferDesc.MiscFlags = 0;
	//hBufferDesc.StructureByteStride = sizeof(UINT);

	D3D11_SUBRESOURCE_DATA hSubResourceData;
	ZeroMemory(&hSubResourceData, sizeof(hSubResourceData));
	hSubResourceData.pSysMem = aIndex;
	//hSubResourceData.SysMemPitch = 0;
	//hSubResourceData.SysMemSlicePitch = 0;

	D3DBufferPtr pBuffer;
	HRESULT hr = m_pDevice->CreateBuffer(&hBufferDesc, &hSubResourceData, &pBuffer);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateBuffer");
	}
	return pBuffer;
}

D3DBufferPtr D3DGraphics::CreateConstantBuffer(size_t nByte)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = ((static_cast<UINT>(nByte) + 15) / 16) * 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	P_IS_TRUE(bd.ByteWidth <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT);

	D3DBufferPtr pBuffer;
	HRESULT hr = m_pDevice->CreateBuffer(&bd, nullptr, &pBuffer);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateBuffer");
	}

	return pBuffer;
}

D3DInputLayoutPtr D3DGraphics::CreateInputLayout(
	const D3D11_INPUT_ELEMENT_DESC* aElement, UINT nElement,
	const string& fileName, const string& entryPoint, const D3D_SHADER_MACRO* aMacro
)
{
	bool showError = true;

#if defined(_DEBUG)
	// グラフィックデバッグツールによるシェーダーのデバッグを有効にする
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> pErrorBlob = NULL;
	HRESULT hr = D3DCompileFromFile(
		CA2W(fileName.c_str()), aMacro, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), VS_COMPILE_TARGET, compileFlags, 0,
		&blob, &pErrorBlob
	);
	if (FAILED(hr))
	{
		if (showError) {
			if (pErrorBlob != NULL)
			{
				MessageBox(NULL, CA2T((char*)pErrorBlob->GetBufferPointer()), _T(""), 0);
			}
		}
	}

	D3DInputLayoutPtr pLayout;
	hr = m_pDevice->CreateInputLayout(aElement, nElement, blob->GetBufferPointer(),
		blob->GetBufferSize(), &pLayout);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateInputLayout");
	}

	return pLayout;
}

D3DVertexShaderPtr D3DGraphics::CreateVertexShader(
	const std::string& fileName, const std::string& entryPoint, const D3D_SHADER_MACRO* aMacro
)
{
	bool showError = true;
	D3DVertexShaderPtr pShader;

#if defined(_DEBUG)
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif
	ComPtr<ID3DBlob> blob;
	//setlocale(LC_CTYPE, "jpn");
	ComPtr<ID3DBlob> pErrorBlob = NULL;
	HRESULT hr = D3DCompileFromFile(CA2W(fileName.c_str()), aMacro, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), VS_COMPILE_TARGET, compileFlags, 0, &blob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (showError)
		{
			if (pErrorBlob != NULL)
			{
				MessageBox(NULL, CA2T((char*)pErrorBlob->GetBufferPointer()), _T(""), 0);
			}
		}
		return nullptr;
	}

	hr = m_pDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &pShader);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateVertexShader");
	}

	return pShader;
}

D3DGeometryShaderPtr D3DGraphics::CreateGeometryShader(
	const string& fileName, const string& entryPoint, const D3D_SHADER_MACRO* aMacro
)
{
	bool showError = true;
	D3DGeometryShaderPtr pShader;

#if defined(_DEBUG)
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> pErrorBlob = NULL;
	HRESULT hr = D3DCompileFromFile(CA2W(fileName.c_str()), aMacro, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), "gs_5_0", compileFlags, 0, &blob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (showError) {
			if (pErrorBlob != NULL)
			{
				MessageBox(NULL, CA2T((char*)pErrorBlob->GetBufferPointer()), _T(""), 0);
			}
		}
		return nullptr;
	}

	hr = m_pDevice->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &pShader);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateGeometryShader");
	}

	return pShader;
}

D3DPixelShaderPtr D3DGraphics::CreatePixelShader(
	const string& fileName, const string& entryPoint, const D3D_SHADER_MACRO* aMacro
)
{
	bool showError = true;
	D3DPixelShaderPtr pShader;

#if defined(_DEBUG)
	UINT	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT	compileFlags = 0;
#endif
	ComPtr<ID3DBlob> blob;
	ComPtr<ID3DBlob> pErrorBlob = NULL;
	HRESULT hr = D3DCompileFromFile(CA2W(fileName.c_str()), aMacro, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), "ps_5_0", compileFlags, 0, &blob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (showError) {
			if (pErrorBlob != NULL)
			{
				MessageBox(NULL, CA2T((char*)pErrorBlob->GetBufferPointer()), _T(""), 0);
			}
		}
		return nullptr;
	}

	hr = m_pDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), NULL, &pShader);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreatePixelShader");
	}

	return pShader;
}

////////////////////////////////////////////////////////////////////////////////

D3DTexture2DPtr D3DGraphics::CaptureRenderTargetStagingTexture(const D3DRenderTargetViewPtr& pRtView)
{
	D3DResourcePtr pRtResource;
	pRtView->GetResource(&pRtResource);
	P_IS_TRUE(pRtResource);

	D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	pRtResource->GetType(&resType);
	if (resType != D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		P_THROW_ERROR("CaptureTexture2D");
	}
	D3DTexture2DPtr pRtTexture;
	HRESULT hr = pRtResource->QueryInterface(IID_PPV_ARGS(&pRtTexture));
	if (FAILED(hr)) {
		P_THROW_ERROR("QueryInterface");
	}

	D3D11_TEXTURE2D_DESC desc;
	pRtTexture->GetDesc(&desc);

	if (1 < desc.SampleDesc.Count) {
		P_THROW_ERROR("Not supported case (1 < desc.SampleDesc.Count)");
	}

	D3DTexture2DPtr pStaging;
	if ((desc.Usage == D3D11_USAGE_STAGING) && (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ)) {
		pStaging = pRtTexture;
	}
	else {
		D3D11_TEXTURE2D_DESC stagingDesc = desc;
		stagingDesc.BindFlags = 0;
		stagingDesc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		stagingDesc.Usage = D3D11_USAGE_STAGING;

		hr = m_pDevice->CreateTexture2D(&stagingDesc, nullptr, &pStaging);
		if (FAILED(hr)) {
			P_THROW_ERROR("CreateTexture2D");
		}
		P_IS_TRUE(pStaging);

		m_pDC->CopyResource(pStaging.Get(), pRtResource.Get());
	}

	return pStaging;
}

D3DMappedSubResource D3DGraphics::MapDyamaicBuffer(const D3DBufferPtr& pDynamicBuffer)
{
	P_IS_TRUE(pDynamicBuffer != nullptr);
	D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
	UINT mapFlags = 0;
	D3D11_MAPPED_SUBRESOURCE mappedData;
	ZeroMemory(&mappedData, sizeof(D3D11_MAPPED_SUBRESOURCE));
	m_pDC->Map(pDynamicBuffer.Get(), 0, mapType, mapFlags, &mappedData);
	return D3DMappedSubResource(m_pDC, pDynamicBuffer, mappedData.pData);
}

D3DMappedSubResource D3DGraphics::MapStagingBuffer(
	const D3DResourcePtr& pBuffer, D3D11_MAP mapType, D3D11_MAPPED_SUBRESOURCE* pMappedSubResource
)
{
	UINT subResourceId = 0;
	UINT mapFlags = 0;
	D3D11_MAPPED_SUBRESOURCE mappedData;
	ZeroMemory(&mappedData, sizeof(D3D11_MAPPED_SUBRESOURCE));

	HRESULT hr = m_pDC->Map(pBuffer.Get(), subResourceId, mapType, mapFlags, &mappedData);
	if (FAILED(hr)) {
		P_THROW_ERROR("Map");
	}

	if (pMappedSubResource != nullptr) {
		*pMappedSubResource = mappedData;
	}
	return D3DMappedSubResource(m_pDC, pBuffer, mappedData.pData);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics::SetDrawSelectedEntityMode(bool isSelectedEntityMode)
{
	if (isSelectedEntityMode) {
		P_IS_TRUE(GetDrawMode() == DrawMode::DRAW_NORMAL);	// not supported in other cases.
		m_drawMode = DrawMode::DRAW_SELECTED_ENTITY;
		m_pDC->OMSetDepthStencilState(m_pDepthStencilStateForSelectedEntity.Get(), 1);
	}
	else {
		P_IS_TRUE(GetDrawMode() == DrawMode::DRAW_SELECTED_ENTITY);	// not supported in other cases.
		m_drawMode = DrawMode::DRAW_NORMAL;
		m_pDC->OMSetDepthStencilState(m_pDepthStencilState.Get(), 1);
	}
}

void D3DGraphics::DrawBegin(bool isForSelection, bool isEraseBackground)
{
	PrepareDepthStencilView();
	if (isForSelection) {
		m_drawMode = DrawMode::DRAW_FOR_SELECTION;
		PrepareRenderTargetViewForSelection();
	} else {
		m_drawMode = DrawMode::DRAW_NORMAL;
		PrepareRenderTargetView();
	}

	if (isEraseBackground) {
		float aClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; //red,green,blue,alpha
		if (isForSelection) {
			// Default color shall be 0 because D3D_SELECTION_TARGET_NULL is 0.
			// Transparency shall not be used in DRAW_FOR_SELECTION mode.
			aClearColor[3] = 0.0f;
		}
		m_pDC->ClearRenderTargetView(m_pRenderTargetView.Get(), aClearColor);
		m_pDC->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	m_pDC->RSSetViewports(1, &m_viewport);
	m_pDC->RSSetState(m_pRasterizerState.Get());
	m_pDC->OMSetDepthStencilState(m_pDepthStencilState.Get(), 1);

	ID3D11RenderTargetView* apRtv[1] = { m_pRenderTargetView.Get() };
	m_pDC->OMSetRenderTargets(1, apRtv, m_pDepthStencilView.Get());

	m_nDrawnPoint = 0;
}

void D3DGraphics::DrawPointList(
	const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, size_t vertexSize, size_t nVertex
)
{
	P_ASSERT(nVertex <= UINT_MAX);
	m_pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	ID3D11Buffer* apVB[1] = { pVertexBuf.Get() };
	UINT aVertexSize[1] = { (UINT)vertexSize };
	UINT aOffset[1] = { 0 };
	m_pDC->IASetVertexBuffers(0, 1, apVB, aVertexSize, aOffset);

	SetShaderContext(sc);
	m_pDC->Draw((UINT)nVertex, 0);

	if (GetDrawMode() == DrawMode::DRAW_NORMAL) {
		m_nDrawnPoint += nVertex;
	}
}

void D3DGraphics::DrawPointList(
	const D3DShaderContext& sc, size_t nVertex,
	const D3DBufferPtr& pVertexBuf0, size_t vertexSize0,
	const D3DBufferPtr& pVertexBuf1, size_t vertexSize1
)
{
	P_ASSERT(nVertex <= UINT_MAX);
	m_pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	ID3D11Buffer* apVB[2] = { pVertexBuf0.Get(), pVertexBuf1.Get() };
	UINT aVertexSize[2] = { (UINT)vertexSize0, (UINT)vertexSize1 };
	UINT aOffset[2] = { 0, 0 };
	m_pDC->IASetVertexBuffers(0, 2, apVB, aVertexSize, aOffset);

	SetShaderContext(sc);
	m_pDC->Draw((UINT)nVertex, 0);

	if (GetDrawMode() == DrawMode::DRAW_NORMAL) {
		m_nDrawnPoint += nVertex;
	}
}

void D3DGraphics::DrawPointLists(
	const D3DShaderContext& sc, D3DVertexBufferEnumerator* pVertexBufs, size_t vertexSize
)
{
	P_IS_TRUE(pVertexBufs != nullptr);
	m_pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	SetShaderContext(sc);
	UINT aVertexSize[1] = { (UINT)vertexSize };
	const bool isCount = GetDrawMode() == DrawMode::DRAW_NORMAL;

	for (; pVertexBufs->HasCurrent();  pVertexBufs->GoNext()) {
		ID3D11Buffer* apVB[1] = { pVertexBufs->GetCurrent().pVertexBuffer.Get() };
		UINT aOffset[1] = { 0 };
		m_pDC->IASetVertexBuffers(0, 1, apVB, aVertexSize, aOffset);

		const UINT nVertex = pVertexBufs->GetCurrent().nVertex;
		m_pDC->Draw(nVertex, 0);

		if (isCount) {
			m_nDrawnPoint += nVertex;
		}
	}
}

void D3DGraphics::DrawTriangleList(
	const D3DShaderContext& sc, const D3DBufferPtr& pVertexBuf, const D3DBufferPtr& pIndexBuf, size_t vertexSize, size_t nIndex
)
{
	P_ASSERT(nIndex <= UINT_MAX);
	m_pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11Buffer* apVB[1] = { pVertexBuf.Get() };
	UINT aVertexSize[1] = { (UINT)vertexSize };
	UINT anOffset[1] = { 0 };
	m_pDC->IASetVertexBuffers(0, 1, apVB, aVertexSize, anOffset);

	m_pDC->IASetIndexBuffer(pIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);

	SetShaderContext(sc);
	m_pDC->DrawIndexed((UINT)nIndex, 0, 0);
}

void D3DGraphics::SetShaderContext(const D3DShaderContext& context)
{
	ID3D11Buffer* apBuffer[1] = { nullptr };
	m_pDC->IASetInputLayout(context.GetIAInputLayout().Get());

	m_pDC->VSSetShader(context.GetVertexShader().Get(), nullptr, 0);
	apBuffer[0] = { context.GetVSConstantBuffer().Get() };
	m_pDC->VSSetConstantBuffers(0, 1, apBuffer);

	m_pDC->GSSetShader(context.GetGeometryShader().Get(), nullptr, 0);
	apBuffer[0] = { context.GetGSConstantBuffer().Get() };
	m_pDC->GSSetConstantBuffers(0, 1, apBuffer);

	m_pDC->PSSetShader(context.GetPixelShader().Get(), nullptr, 0);
}

D3DRenderTargetViewPtr D3DGraphics::DrawEnd()
{
	D3DRenderTargetViewPtr pResult = m_pRenderTargetView;
	if (IsSelectionMode()) {
		m_pRenderTargetView.Reset();
		m_drawMode = DrawMode::DRAW_NORMAL;
	}
	else {
		HRESULT hr = m_pSwapChain->Present(0, 0);
		if (FAILED(hr)) {
			P_IGNORE_ERROR("Present");
		}
	}
	return pResult;
}

void D3DGraphics::ResizeBuffers(const CSize& newSize)
{
	P_NOEXCEPT_BEGIN("D3DGraphics::ResizeBuffers");
	if (m_pSwapChain) {
		// release related buffer objects.
		// See https://learn.microsoft.com/ja-jp/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers?source=recommendations
		// See https://www.sfpgmr.net/blog/entry/dxgi-resizebuffers%E3%81%99%E3%82%8B%E3%81%A8%E3%81%8D%E3%81%AB%E6%B0%97%E3%82%92%E3%81%A4%E3%81%91%E3%82%8B%E3%81%93%E3%81%A8.html
		m_pRenderTargetView.Reset();
		m_pDepthStencilView.Reset();

		HRESULT hr = m_pSwapChain->ResizeBuffers(
			0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if (FAILED(hr)) {
			P_THROW_ERROR("ResizeBuffers");
		}

		m_viewport.Width = float(newSize.cx);
		m_viewport.Height = float(newSize.cy);
	}
	P_NOEXCEPT_END;
}

/// <summary>
/// Save view image to file.
/// </summary>
/// <param name="targetFormat">GUID_ContainerFormatPng for example.</param>
/// <param name="targetFilePath"></param>
/// <returns>false if failed.</returns>
/// This function may throw an exception, also.
bool D3DGraphics::SaveViewToFile(REFGUID targetFormat, LPCTSTR targetFilePath)
{
	P_IS_TRUE(targetFilePath != nullptr);

	D3DResourcePtr pResource;
	m_pRenderTargetView->GetResource(&pResource);
	P_IS_TRUE(pResource);

	HRESULT hr = DirectX::SaveWICTextureToFile(
		m_pDC.Get(), pResource.Get(), targetFormat, CT2W(targetFilePath)
	);
	if (FAILED(hr)) {
		P_IGNORE_ERROR("DirectX::SaveWICTextureToFile");
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics::PrepareDepthStencilView()
{
	if (m_pDepthStencilView.Get()) {
		return;
	}

	DXGI_SWAP_CHAIN_DESC sd;
	HRESULT hr = m_pSwapChain->GetDesc(&sd);
	if (FAILED(hr)) {
		P_THROW_ERROR("GetDesc");
	}

	D3D11_TEXTURE2D_DESC hTexture2dDesc;
	hTexture2dDesc.Width = sd.BufferDesc.Width;
	hTexture2dDesc.Height = sd.BufferDesc.Height;
	hTexture2dDesc.MipLevels = 1;
	hTexture2dDesc.ArraySize = 1;
	hTexture2dDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	hTexture2dDesc.SampleDesc = sd.SampleDesc;
	hTexture2dDesc.Usage = D3D11_USAGE_DEFAULT;
	hTexture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	hTexture2dDesc.CPUAccessFlags = 0;
	hTexture2dDesc.MiscFlags = 0;
	D3DTexture2DPtr pDepthStencilBuffer;
	hr = m_pDevice->CreateTexture2D(&hTexture2dDesc, NULL, &pDepthStencilBuffer);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateTexture2D");
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC hDepthStencilViewDesc;
	hDepthStencilViewDesc.Format = hTexture2dDesc.Format;
	hDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	hDepthStencilViewDesc.Flags = 0;
	hr = m_pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), &hDepthStencilViewDesc, &m_pDepthStencilView);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateDepthStencilState");
	}
}

void D3DGraphics::PrepareRenderTargetView()
{
	P_IS_TRUE(!IsSelectionMode());
	if (m_pRenderTargetView.Get()) {
		return;
	}

	D3DTexture2DPtr pRenderTargetBuffer;
	HRESULT hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pRenderTargetBuffer));
	if (FAILED(hr)) {
		P_THROW_ERROR("GetBuffer");
	}

	hr = m_pDevice->CreateRenderTargetView(pRenderTargetBuffer.Get(), NULL, &m_pRenderTargetView);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateRenderTargetView");
	}
}

void D3DGraphics::PrepareRenderTargetViewForSelection()
{
	m_pRenderTargetView.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	HRESULT hr = m_pSwapChain->GetDesc(&sd);
	if (FAILED(hr)) {
		P_THROW_ERROR("GetDesc");
	}

	D3D11_TEXTURE2D_DESC hTexture2dDesc;
	hTexture2dDesc.Width = sd.BufferDesc.Width;
	hTexture2dDesc.Height = sd.BufferDesc.Height;
	hTexture2dDesc.MipLevels = 1;
	hTexture2dDesc.ArraySize = 1;
	hTexture2dDesc.Format = DXGI_FORMAT_R16G16B16A16_UINT;
	hTexture2dDesc.SampleDesc = sd.SampleDesc;
	hTexture2dDesc.Usage = D3D11_USAGE_DEFAULT;
	hTexture2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	hTexture2dDesc.CPUAccessFlags = 0;
	hTexture2dDesc.MiscFlags = 0;
	D3DTexture2DPtr pRenderTargetBuffer;
	hr = m_pDevice->CreateTexture2D(&hTexture2dDesc, NULL, &pRenderTargetBuffer);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateTexture2D");
	}

	hr = m_pDevice->CreateRenderTargetView(pRenderTargetBuffer.Get(), NULL, &m_pRenderTargetView);
	if (FAILED(hr)) {
		P_THROW_ERROR("CreateRenderTargetView");
	}
}
