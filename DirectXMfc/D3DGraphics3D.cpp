#include "pch.h"
#include "D3DGraphics3D.h"
#include "D3DModelPointList.h"
#include "D3DModelPointListEnumerator.h"
#include "D3DModelTriangleList.h"
#include <DirectXMath.h>
#define _USE_MATH_DEFINES	// In order to use M_PI, which is not in the C standard.
#include <math.h>
#include <map>

using namespace std;
using namespace DirectX;
using namespace D3D11Graphics;

D3DGraphics3D::D3DGraphics3D() : m_viewSize{1,1}
{
	XMStoreFloat4x4(&m_modelMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
	m_viewNearZ = 0.1f;
	m_viewFarZ = 100.f;
	m_fovAngleYDeg = 45;
	m_pointSize = -0.1;
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::Initialize(HWND hWnd)
{
	CRect rect;
	BOOL isOk = ::GetClientRect(hWnd, &rect);
	if (!isOk) {
		P_THROW_ERROR("GetClientRect");
	}
	m_viewSize = rect.Size();
	m_graphics.Setup(hWnd, m_viewSize);

	InitializeShaderContexts();
}

void D3DGraphics3D::SetProgressiveViewMode(bool enableProgressiveView, bool isFollowingFrame)
{
	m_isProgressiveViewMode = enableProgressiveView;
	m_isProgressiveViewFollowingFrame = enableProgressiveView && isFollowingFrame;
}

XMMATRIX D3DGraphics3D::GetModelToProjectionMatrix()
{
	XMFLOAT4X4 modelToViewMatrix = PrepareModelToViewMatrix();
	double aspectRatio = GetAspectRatio();
	return XMMatrixMultiply(
		XMMatrixTranspose(XMLoadFloat4x4(&modelToViewMatrix)),
		MakeProjectionMatrix(aspectRatio)
	);
}

void D3DGraphics3D::SetPerspectiveViewNearZ(double z)
{
	P_IS_TRUE(0 < z);
	m_viewNearZ = static_cast<float>(z);
	OnShaderParamModified();
}

void D3DGraphics3D::SetPerspectiveViewFarZ(double z)
{
	P_IS_TRUE(0 < z);
	m_viewFarZ = static_cast<float>(z);
	OnShaderParamModified();
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::UpdateShaderParam()
{
	if (!m_graphics.HasDevice()) {
		return;
	}
	ShaderParam shaderParam;
	shaderParam.viewMatrix = PrepareModelToViewMatrix();

	double aspectRatio = GetAspectRatio();
	XMStoreFloat4x4(&shaderParam.projectionMatrix, XMMatrixTranspose(MakeProjectionMatrix(aspectRatio)));

	// view coordinates is normalized as [-1, 1]. So half size is used.
	shaderParam.pixelSizeX = (0 < m_viewSize.cx ? 2.0f / m_viewSize.cx : 1);
	shaderParam.pixelSizeY = (0 < m_viewSize.cy ? 2.0f / m_viewSize.cy : 1);

	if (m_pointSize < 0) {
		// Set in pixel.
		shaderParam.pointSizeX = float(m_pointSize * shaderParam.pixelSizeX);
		shaderParam.pointSizeY = float(m_pointSize * shaderParam.pixelSizeY);
	}
	else {
		// Set length in model space.
		// Drawn range in Y direction is defined by FovAngleY.
		// Drawn range in X directino is defined by FovAgnleY and aspectRatio.
		// So size in X direction should be adjusted with aspectRatio.
		const double tanY = tan(m_fovAngleYDeg * 0.5 * M_PI / 180);
		shaderParam.pointSizeX = float(m_pointSize / (tanY * aspectRatio));
		shaderParam.pointSizeY = float(m_pointSize / tanY);
	}
	m_graphics.SetConstantBufferData(m_pShaderParamConstBuf, shaderParam);

	m_isNeedToUpdateShaderParameter = false;
}

void D3DGraphics3D::DrawBegin()
{
	bool isEraseBackground = !(IsProgressiveViewMode() && IsProgressiveViewFollowingFrame());
	m_graphics.DrawBegin(false, isEraseBackground);
}

static void CaptureTexture2D(D3DGraphics& g, const D3DTexture2DPtr& pStaging)
{
	P_NOEXCEPT_BEGIN("CaptureTexture2D");
	D3D11_TEXTURE2D_DESC desc;
	pStaging->GetDesc(&desc);

	D3D11_MAPPED_SUBRESOURCE mapped;
	D3DMappedSubResource mappedTexture = g.MapStagingBuffer(pStaging.Get(), D3D11_MAP_READ, &mapped);
	const int64_t imageSize = int64_t(mapped.RowPitch) * int64_t(desc.Height);
	P_IS_TRUE(imageSize < UINT_MAX);
	const int pixelSize = mapped.RowPitch / desc.Width;
	P_IS_TRUE(pixelSize <= 8);
	map<uint64_t, size_t> colorCounter;
	for (UINT row = 0; row < desc.Height; ++row) {
		for (UINT col = 0; col < desc.Width; ++col) {
			const unsigned char* pHead = mappedTexture.ToArray<unsigned char>(size_t(row) * mapped.RowPitch + col * pixelSize);
			uint64_t value = 0;
			for (int i = 0; i < pixelSize; ++i) {
				value += (uint64_t(pHead[i]) << (8 * i));
			}
			auto it = colorCounter.find(value);
			if (it == colorCounter.end()) {
				colorCounter[value] = 1;
			}
			else {
				it->second++;
			}
		}
	}

	P_NOEXCEPT_END;
}

D3DSelectionTargetId GetTexture2DInt64Pixel(D3DGraphics& g, const D3DTexture2DPtr& pStaging, const CPoint& mousePos)
{
	D3D11_TEXTURE2D_DESC desc;
	pStaging->GetDesc(&desc);
	P_ASSERT(0 <= mousePos.x && UINT(mousePos.x) < desc.Width);
	P_ASSERT(0 <= mousePos.y && UINT(mousePos.y) < desc.Height);

	D3D11_MAPPED_SUBRESOURCE mapped;
	D3DMappedSubResource mappedTexture = g.MapStagingBuffer(pStaging.Get(), D3D11_MAP_READ, &mapped);
	const int64_t imageSize = int64_t(mapped.RowPitch) * int64_t(desc.Height);
	P_ASSERT(imageSize < UINT_MAX);
	const int pixelSize = mapped.RowPitch / desc.Width;
	P_IS_TRUE(pixelSize == 8);
	const unsigned char* pPixelHead = mappedTexture.ToArray<unsigned char>(size_t(mousePos.y) * mapped.RowPitch + mousePos.x * pixelSize);
	uint64_t value = 0;
	for (int i = 0; i < pixelSize; ++i) {
		value += (uint64_t(pPixelHead[i]) << (8 * i));
	}
	return D3DSelectionTargetId(value);
}

D3DSelectionTargetId D3DGraphics3D::DrawEndForSelection(const CPoint& mousePos)
{
	P_IS_TRUE(m_graphics.IsSelectionMode());
	D3DRenderTargetViewPtr pRtView = m_graphics.DrawEnd();
	if (!pRtView) {
		return D3D_SELECTION_TARGET_NULL;
	}
	D3DTexture2DPtr pStaging = m_graphics.CaptureRenderTargetStagingTexture(pRtView);
	if (false) {
		CaptureTexture2D(m_graphics, pStaging);
	}

	return GetTexture2DInt64Pixel(m_graphics, pStaging, mousePos);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::SetDrawSelectedEntityMode(bool selectedEntityMode)
{
	m_graphics.SetDrawSelectedEntityMode(selectedEntityMode);
}


void D3DGraphics3D::DrawPointList(D3DModelPointList* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	PrepareShaderParam();
	m_graphics.DrawPointList(
		GetPointListShaderContext(), pModel->m_pVertexBuffer,
		sizeof(D3DModelPointList::Vertex), pModel->m_nVertex
	);
}

void D3DGraphics3D::DrawPointListEnumerator(D3DModelPointListEnumerator* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	if (pModel->HasCurrent()) {
		PrepareShaderParam();
		m_graphics.DrawPointLists(
			GetPointListShaderContext(), pModel, sizeof(D3DModelPointList::Vertex));
	}
	pModel->PostDraw();
}

void D3DGraphics3D::DrawPointArray(const PointListVertex aVertex[], int64_t nVertex, D3DSelectionTargetId selectionTargetIdFirst)
{
	P_IS_TRUE(aVertex != nullptr);
	const bool isSelectionMode = m_graphics.IsSelectionMode();
	if (!m_pTempVertexBuffer) {
		m_pTempVertexBuffer = m_graphics.CreateVertexBufferWithSize(
			m_tempBufferVertexNumMax * sizeof(PointListVertex), nullptr, true
		);
	}
	if (isSelectionMode && !m_pTempVertexStIdBuffer) {
		m_pTempVertexStIdBuffer = m_graphics.CreateVertexBufferWithSize(
			m_tempBufferVertexNumMax * sizeof(D3DSelectionTargetId), nullptr, true
		);
	}

	PrepareShaderParam();
	const int64_t nVertexInBufUpperBound = m_tempBufferVertexNumMax;
	int64_t nRemainingVertex = nVertex;
	while (0 < nRemainingVertex) {
		size_t nVertexInBuf = static_cast<size_t>(min(nVertexInBufUpperBound, nRemainingVertex));
		P_ASSERT(int64_t(nVertexInBuf) <= nRemainingVertex);

		{
			D3DMappedSubResource mappedMemory = m_graphics.MapDyamaicBuffer(m_pTempVertexBuffer);
			UINT dataSize = static_cast<UINT>(nVertexInBuf * sizeof(PointListVertex));
			mappedMemory.Write(aVertex + (nVertex - nRemainingVertex), dataSize);
		}
		if (!isSelectionMode) {
			m_graphics.DrawPointList(
				GetPointListShaderContext(), m_pTempVertexBuffer,
				sizeof(PointListVertex), nVertexInBuf
			);
		} else {
			{
				D3DMappedSubResource mappedMemory = m_graphics.MapDyamaicBuffer(m_pTempVertexStIdBuffer);
				if (selectionTargetIdFirst == D3D_SELECTION_TARGET_NULL) {
					memset(mappedMemory.ToArray<char*>(0), 0, nVertexInBuf * sizeof(D3DSelectionTargetId));
				}
				else {
					D3DSelectionTargetId* aIdInBuf = mappedMemory.ToArray<D3DSelectionTargetId>(0);
					for (size_t iVertexInBuf = 0; iVertexInBuf < nVertexInBuf; ++iVertexInBuf) {
						D3DSelectionTargetId stId = selectionTargetIdFirst + nVertex - nRemainingVertex + iVertexInBuf;
						aIdInBuf[iVertexInBuf] = stId;
					}
				}
			}

			m_graphics.DrawPointList(
				GetPointListShaderContext(), nVertexInBuf,
				m_pTempVertexBuffer, sizeof(PointListVertex),
				m_pTempVertexStIdBuffer, sizeof(D3DSelectionTargetId)
			);
		}

		nRemainingVertex -= nVertexInBuf;
	}
}

void D3DGraphics3D::DrawTriangleList(D3DModelTriangleList* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	PrepareShaderParam();
	m_graphics.DrawTriangleList(
		GetTriangleListShaderContext(), pModel->m_pVertexBuffer, pModel->m_pIndexBuffer,
		sizeof(D3DModelTriangleList::Vertex), pModel->m_nIndex
	);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::ResizeBuffers(const SIZE& newSize)
{
	m_viewSize = newSize; OnShaderParamModified();
	m_graphics.ResizeBuffers(newSize);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::InitializeShaderContexts()
{
	m_pShaderParamConstBuf = m_graphics.CreateConstantBuffer(sizeof(ShaderParam));

	InitializeShaderContextsForNormalRendering();
	InitializeShaderContextsForSelection();
}

void D3DGraphics3D::InitializeShaderContextsForNormalRendering()
{
	const string hlslFilePath = "SampleShader.hlsl";
	const D3D_SHADER_MACRO aMacro[] = {
		{ "RGBA_TYPE", "float4" },
		{ nullptr, nullptr }
	};

	D3D11_INPUT_ELEMENT_DESC aTriangleListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};
	m_triangleListSc.Init(
		m_graphics.CreateInputLayout(aTriangleListElem, sizeof(aTriangleListElem) / sizeof(aTriangleListElem[0]), hlslFilePath, "vsMain", aMacro),
		m_graphics.CreateVertexShader(hlslFilePath, "vsMain", aMacro),
		m_pShaderParamConstBuf,
		nullptr, nullptr,
		m_graphics.CreatePixelShader(hlslFilePath, "psMain", aMacro)
	);

	D3D11_INPUT_ELEMENT_DESC aPointListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R8G8B8A8_UNORM,	    0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};
	m_pointListSc.Init(
		m_graphics.CreateInputLayout(aPointListElem, sizeof(aPointListElem) / sizeof(aPointListElem[0]), hlslFilePath, "vsMain", aMacro),
		m_triangleListSc.GetVertexShader(),
		m_pShaderParamConstBuf,
		m_graphics.CreateGeometryShader(hlslFilePath, "gsMain", aMacro),
		m_pShaderParamConstBuf,
		m_triangleListSc.GetPixelShader()
	);
}

void D3DGraphics3D::InitializeShaderContextsForSelection()
{
	const string hlslFilePath = "SampleShader.hlsl";
	const D3D_SHADER_MACRO aMacro[] = {
		{ "RGBA_TYPE", "uint4" },
		{ nullptr, nullptr }
	};

	D3D11_INPUT_ELEMENT_DESC aTriangleListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R16G16B16A16_UINT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};
	m_selectionTriangleListSc.Init(
		m_graphics.CreateInputLayout(aTriangleListElem, sizeof(aTriangleListElem) / sizeof(aTriangleListElem[0]), hlslFilePath, "vsMain", aMacro),
		m_graphics.CreateVertexShader(hlslFilePath, "vsMain", aMacro),
		m_pShaderParamConstBuf,
		nullptr, nullptr,
		m_graphics.CreatePixelShader(hlslFilePath, "psMain", aMacro)
	);

	D3D11_INPUT_ELEMENT_DESC aPointListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R16G16B16A16_UINT,	1,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};
	m_selectionPointListSc.Init(
		m_graphics.CreateInputLayout(aPointListElem, sizeof(aPointListElem) / sizeof(aPointListElem[0]), hlslFilePath, "vsMain", aMacro),
		m_selectionTriangleListSc.GetVertexShader(),
		m_pShaderParamConstBuf,
		m_graphics.CreateGeometryShader(hlslFilePath, "gsMain", aMacro),
		m_pShaderParamConstBuf,
		m_selectionTriangleListSc.GetPixelShader()
	);
}

XMFLOAT4X4 D3DGraphics3D::PrepareModelToViewMatrix()
{
	if (m_isNeedToUpdateModelToViewMatrix) {
		XMStoreFloat4x4(
			&m_modelToViewMatrix,
			XMMatrixMultiply(XMLoadFloat4x4(&m_viewMatrix), XMLoadFloat4x4(&m_modelMatrix))
		);
		m_isNeedToUpdateModelToViewMatrix = false;
	}
	return m_modelToViewMatrix;
}

double D3DGraphics3D::GetAspectRatio() const
{
	double aspectRatio = 1;
	if (0 < m_viewSize.cx && 0 < m_viewSize.cy) {
		aspectRatio = double(m_viewSize.cx) / m_viewSize.cy;
	}
	return aspectRatio;
}

XMMATRIX D3DGraphics3D::MakeProjectionMatrix(double aspectRatio) const
{
	return XMMatrixPerspectiveFovRH(XMConvertToRadians((float)m_fovAngleYDeg), (float)aspectRatio, m_viewNearZ, m_viewFarZ);
}
