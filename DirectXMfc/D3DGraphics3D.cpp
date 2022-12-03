#include "pch.h"
#include "D3DGraphics3D.h"
#include "D3DModelTriangleList.h"
#include <DirectXMath.h>

using namespace std;
using namespace DirectX;
using namespace D3D11Graphics;

D3DGraphics3D::D3DGraphics3D() : m_viewSize{1,1}
{
	m_viewNearZ = 0.1f;
	m_viewFarZ = 100.f;
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

void D3DGraphics3D::UpdateShaderParam(const XMFLOAT4X4& viewMatrix)
{
	if (!m_graphics.HasDevice()) {
		return;
	}
	ShaderParam shaderParam;
	shaderParam.viewMatrix = viewMatrix;

	float aspectRatio = float(m_viewSize.cx) / m_viewSize.cy;
	XMStoreFloat4x4(&shaderParam.projectionMatrix, XMMatrixTranspose(
		XMMatrixPerspectiveFovRH(XMConvertToRadians(90), aspectRatio, m_viewNearZ, m_viewFarZ)
	));
	m_graphics.SetConstantBufferData(m_pShaderParamConstBuf, shaderParam);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::DrawTriangleList(D3DModelTriangleList* pModel)
{
	P_IS_TRUE(pModel != nullptr);
	pModel->PreDraw(*this, m_graphics);
	m_graphics.DrawTriangleList(
		m_triangleListSc, pModel->m_pVertexBuffer, pModel->m_pIndexBuffer,
		sizeof(D3DModelTriangleList::Vertex), pModel->m_nIndex
	);
}

////////////////////////////////////////////////////////////////////////////////

void D3DGraphics3D::ResizeBuffers(const SIZE& newSize)
{
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
}
