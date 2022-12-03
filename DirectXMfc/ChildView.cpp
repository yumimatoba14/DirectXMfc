
// ChildView.cpp : CChildView クラスの実装
//

#include "pch.h"
#include "framework.h"
#include <DirectXMath.h>
#include <vector>
#include "DirectXMfc.h"
#include "ChildView.h"
#include "D3DModelPointList.h"
#include "D3DModelTriangleList.h"

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

static XMVECTOR MakeXmVector(float x, float y, float z)
{
	XMFLOAT3 vec(x, y, z);
	return XMLoadFloat3(&vec);
}

void CChildView::UpdateShaderParam()
{
	m_graphics.UpdateShaderParam(m_viewOp.GetViewMatrix());
}

void CChildView::UpdateView()
{
	UpdateShaderParam();
	Invalidate(TRUE);	// The value of TRUE is actually not used because OnEraseBkgnd() has been overridden.
}

void CChildView::OnPaint()
{
	CPaintDC dc(this); // 描画のデバイス コンテキスト
	
	if (!m_graphics.IsInitialized()) {
		m_graphics.Initialize(*this);

		m_pModel.reset(new D3DModelTriangleList());
		m_pModel.reset(new D3DModelPointList()); m_graphics.SetPointSize(0.1);
		m_viewOp.SetEyePoint(0, 0, 1.5);
		m_graphics.SetFovAngleYInDegree(90);
		UpdateShaderParam();
	}

	m_graphics.DrawBegin();
	m_pModel->DrawTo(m_graphics);
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
