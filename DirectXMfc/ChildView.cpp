
// ChildView.cpp : CChildView クラスの実装
//

#include "pch.h"
#include "framework.h"
#include <DirectXMath.h>
#include <vector>
#include "DirectXMfc.h"
#include "ChildView.h"

using namespace std;
using namespace D3D11Graphics;
using namespace DirectX;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CChildView メッセージ ハンドラー

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

namespace {
	struct ShaderParam {
		XMFLOAT4X4 viewMatrix;
		XMFLOAT4X4 projectionMatrix;
	};

}

static XMVECTOR MakeXmVector(float x, float y, float z)
{
	XMFLOAT3 vec(x, y, z);
	return XMLoadFloat3(&vec);
}

void CChildView::PrepareModels()
{
	m_pShaderParamConstBuf = m_graphics.CreateConstantBuffer(sizeof(ShaderParam));

	D3D11_INPUT_ELEMENT_DESC elem[] = {
		{ "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0},
		{ "COLOR"	,	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0},
	};

	const string hlslFilePath = "SampleShader.hlsl";
	m_sampleShaderContext.Init(
		m_graphics.CreateInputLayout(elem, sizeof(elem)/sizeof(elem[0]), hlslFilePath, "vsMain"),
		m_graphics.CreateVertexShader(hlslFilePath, "vsMain"),
		m_pShaderParamConstBuf,
		nullptr, nullptr,
		m_graphics.CreatePixelShader(hlslFilePath, "psMain")
	);

	struct Vertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 col;
	};
	const float z0 = 0.5;
	const float z1 = 0.75;
	const float z2 = 0.25;
	vector<Vertex> vertices =
	{
		{ XMFLOAT3(0.0f, 1.0f, z0), XMFLOAT4(1,0,0,1)},
		{ XMFLOAT3(-1.0f, 0.0f, z0), XMFLOAT4(0,1,0,1)},
		{ XMFLOAT3(0.0f, -1.0f, z0), XMFLOAT4(0,0,1,1)},
		{ XMFLOAT3(1.0f, 0.0f, z0), XMFLOAT4(0,0,0,1)},
		{ XMFLOAT3(0.25f, -0.25f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.75f, -0.75f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.75f, -0.25f, z1), XMFLOAT4(0.5,0.5,0,1)},
		{ XMFLOAT3(0.25f, 0.25f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
		{ XMFLOAT3(0.75f, 0.25f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
		{ XMFLOAT3(0.75f, 0.75f, z2), XMFLOAT4(0.0,0.5,0.5,1)},
	};

	vector<UINT> indices = { 0,1,2,0,2,3,4,5,6,7,9,8 };

	m_model0.vertexSize = sizeof(Vertex);
	m_model0.nVertex = vertices.size();
	m_model0.nIndex = indices.size();
	m_model0.pVertexBuffer = m_graphics.CreateVertexBuffer(vertices.data(), static_cast<UINT>(m_model0.nVertex));
	m_model0.pIndexBuffer = m_graphics.CreateIndexBuffer(indices.data(), static_cast<UINT>(m_model0.nIndex));
}

void CChildView::UpdateShaderParam()
{
	if (!m_graphics.HasDevice()) {
		return;
	}
	ShaderParam shaderParam;
	shaderParam.viewMatrix = m_viewOp.GetViewMatrix();

	CRect rect;
	GetClientRect(&rect);
	float aspectRatio = float(rect.Width()) / rect.Height();
	float nearZ = 0.1f;
	float farZ = 100.f;
	XMStoreFloat4x4(&shaderParam.projectionMatrix, XMMatrixTranspose(
		XMMatrixPerspectiveFovRH(XMConvertToRadians(90), aspectRatio, nearZ, farZ)
	));
	m_graphics.SetConstantBufferData(m_pShaderParamConstBuf, shaderParam);
}

void CChildView::UpdateView()
{
	UpdateShaderParam();
	Invalidate(TRUE);	// The value of TRUE is actually not used because OnEraseBkgnd() has been overridden.
}

void CChildView::OnPaint()
{
	CPaintDC dc(this); // 描画のデバイス コンテキスト
	
	if (!m_graphics.HasDevice()) {
		CRect rect;
		GetClientRect(&rect);
		m_graphics.Setup(*this, rect.Size());

		PrepareModels();
		m_viewOp.SetEyePoint(0, 0, 1.5);
		UpdateShaderParam();
	}

	m_graphics.DrawBegin();
	m_graphics.DrawTriangleList(
		m_sampleShaderContext, m_model0.pVertexBuffer, m_model0.pIndexBuffer,
		m_model0.vertexSize, m_model0.nIndex
	);
	m_graphics.DrawEnd();

	// メッセージの描画のために CWnd::OnPaint() を呼び出さないでください。
}



void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_graphics.ResizeBuffers(CSize(cx, cy));
	UpdateShaderParam();
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_viewOp.IsMouseMoving()) {
		m_viewOp.MouseMove(point);
		UpdateView();
	}

	CWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_viewOp.StartMouseMove(point);
	UpdateView();

	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_viewOp.EndMouseMove(point);
	UpdateView();

	CWnd::OnLButtonUp(nFlags, point);
}


BOOL CChildView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	double stepLength = 1;
	m_viewOp.GoForward((zDelta < 0 ? -1 : 1) * stepLength);
	UpdateView();

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	// Background would be erased in OnPaint().
	return TRUE;
	//return CWnd::OnEraseBkgnd(pDC);
}
