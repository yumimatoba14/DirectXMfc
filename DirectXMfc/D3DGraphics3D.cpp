#include "pch.h"
#include "D3DGraphics3D.h"
#include "D3DModelPointList.h"
#include "D3DModelPointListEnumerator.h"
#include "D3DModelTriangleList.h"
#include <DirectXMath.h>
#define _USE_MATH_DEFINES	// In order to use M_PI, which is not in the C standard.
#include <math.h>

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

void D3DGraphics3D::UpdateShaderParam()
{
	if (!m_graphics.HasDevice()) {
		return;
	}
	ShaderParam shaderParam;
	shaderParam.viewMatrix = PrepareModelToViewMatrix();

	double aspectRatio = 1;
	if (0 < m_viewSize.cx && 0 < m_viewSize.cy) {
		aspectRatio = double(m_viewSize.cx) / m_viewSize.cy;
	}
	XMStoreFloat4x4(&shaderParam.projectionMatrix, XMMatrixTranspose(
		XMMatrixPerspectiveFovRH(XMConvertToRadians((float)m_fovAngleYDeg), (float)aspectRatio, m_viewNearZ, m_viewFarZ)
	));

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
	m_graphics.DrawBegin(isEraseBackground);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::DrawPointList(D3DModelPointList* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	PrepareShaderParam();
	m_graphics.DrawPointList(
		m_pointListSc, pModel->m_pVertexBuffer,
		sizeof(D3DModelPointList::Vertex), pModel->m_nVertex
	);
}

void D3DGraphics3D::DrawPointListEnumerator(D3DModelPointListEnumerator* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	if (pModel->HasCurrent()) {
		PrepareShaderParam();
		m_graphics.DrawPointLists(m_pointListSc, pModel, sizeof(D3DModelPointList::Vertex));
	}
	pModel->PostDraw();
}

void D3DGraphics3D::DrawPointArray(const PointListVertex aVertex[], int64_t nVertex)
{
	P_IS_TRUE(aVertex != nullptr);
	if (!m_pTempVertexBuffer) {
		m_pTempVertexBuffer = m_graphics.CreateVertexBufferWithSize(
			m_tempVertexBufferSize, nullptr, true
		);
	}

	PrepareShaderParam();
	const int64_t nVertexInBufUpperBound = m_tempVertexBufferSize / sizeof(PointListVertex);
	int64_t nRemainingVertex = nVertex;
	while (0 < nRemainingVertex) {
		size_t nVertexInBuf = static_cast<size_t>(min(nVertexInBufUpperBound, nRemainingVertex));
		P_ASSERT(int64_t(nVertexInBuf) <= nRemainingVertex);

		{
			D3DMappedSubResource mappedMemory = m_graphics.MapDyamaicBuffer(m_pTempVertexBuffer);
			UINT dataSize = static_cast<UINT>(nVertexInBuf * sizeof(PointListVertex));
			mappedMemory.Write(aVertex + (nVertex - nRemainingVertex), dataSize);
		}
		m_graphics.DrawPointList(
			m_pointListSc, m_pTempVertexBuffer,
			sizeof(PointListVertex), nVertexInBuf
		);

		nRemainingVertex -= nVertexInBuf;
	}
}

void D3DGraphics3D::DrawTriangleList(D3DModelTriangleList* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	PrepareShaderParam();
	m_graphics.DrawTriangleList(
		m_triangleListSc, pModel->m_pVertexBuffer, pModel->m_pIndexBuffer,
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

	D3D11_INPUT_ELEMENT_DESC aTriangleListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};

	const string hlslFilePath = "SampleShader.hlsl";
	m_triangleListSc.Init(
		m_graphics.CreateInputLayout(aTriangleListElem, sizeof(aTriangleListElem) / sizeof(aTriangleListElem[0]), hlslFilePath, "vsMain"),
		m_graphics.CreateVertexShader(hlslFilePath, "vsMain"),
		m_pShaderParamConstBuf,
		nullptr, nullptr,
		m_graphics.CreatePixelShader(hlslFilePath, "psMain")
	);

	D3D11_INPUT_ELEMENT_DESC aPointListElem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R8G8B8A8_UNORM,	    0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};
	m_pointListSc.Init(
		m_graphics.CreateInputLayout(aPointListElem, sizeof(aPointListElem) / sizeof(aPointListElem[0]), hlslFilePath, "vsMain"),
		m_triangleListSc.GetVertexShader(),
		m_pShaderParamConstBuf,
		m_graphics.CreateGeometryShader(hlslFilePath, "gsMain"),
		m_pShaderParamConstBuf,
		m_triangleListSc.GetPixelShader()
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
