
// ChildView.cpp : CChildView クラスの実装
//

#include "pch.h"
#include "framework.h"
#include <DirectXMath.h>
#include <vector>
#include <sstream>
#include "DirectXMfc.h"
#include "ChildView.h"
#include "D3DModelPointList.h"
#include "PointListSampleModel.h"
#include "D3DModelTriangleList.h"

using namespace std;
using namespace D3D11Graphics;
using namespace DirectX;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ID_PROGRESSIVE_VIEW_TIMER (1)

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
	ON_WM_TIMER()
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
	m_graphics.SetViewMatrix(m_viewOp.GetViewMatrix());
}

void CChildView::UpdateView()
{
	UpdateShaderParam();
	Invalidate(TRUE);	// The value of TRUE is actually not used because OnEraseBkgnd() has been overridden.
}

void CChildView::OnPaint()
{
	CPaintDC dc(this); // 描画のデバイス コンテキスト
	ULONGLONG startTickMiliSec = ::GetTickCount64();
	
	if (!m_graphics.IsInitialized()) {
		m_graphics.Initialize(*this);

		m_pModel.reset(new D3DModelTriangleList());
		m_pModel.reset(new D3DModelPointList()); m_graphics.SetPointSize(0.1);
		m_pModel.reset(new PointListSampleModel()); m_graphics.SetPointSize(0.001);
		m_pModel.reset(new PointListEnumeratorSampleModel()); m_graphics.SetPointSize(0.001);
		CString filePath = _T("..\\ELodPointList.bin");
		m_pModel.reset(new MemoryMappedPointListEnumeratorSampleModel(filePath, true)); m_graphics.SetPointSize(0.001);
		m_pModel.reset(new MultiPointListSampleModel1(filePath)); m_graphics.SetPointSize(0.001);
		m_viewOp.SetEyePoint(0, 0, 3);
		UpdateShaderParam();
	}

	bool isProgressiveViewMode = m_viewOp.IsMouseMoving();
	isProgressiveViewMode = true;
	m_graphics.SetProgressiveViewMode(isProgressiveViewMode, !m_restartProgressiveView);
	if (!isProgressiveViewMode || m_restartProgressiveView) {
		m_nDrawnPoint = 0;
		m_totalStartTickMiliSec = startTickMiliSec;
		m_maxFrameTimeMiliSec = 0;
	}

	const bool isMouseMoveCullingEnabled = false;
	if (isMouseMoveCullingEnabled) {
		m_graphics.SetViewMoving(m_viewOp.IsMouseMoving());
	}
	m_graphics.DrawBegin();
	m_pModel->DrawTo(m_graphics);
	m_graphics.DrawEnd();

	bool isSaveViewImage = false;
	if (isSaveViewImage) {
		static int fileNum = 0;
		wstringstream fileNameStream;
		fileNameStream << "./ViewImage" << (fileNum++) << ".png";
		m_graphics.SaveViewToFile(GUID_ContainerFormatPng, fileNameStream.str().c_str());
	}

	uint64_t nDrawnPointInFrame = static_cast<uint64_t>(m_graphics.GetDrawnPointCount());
	m_nDrawnPoint += nDrawnPointInFrame;

	ULONGLONG endMiliSec = ::GetTickCount64();
	ULONGLONG frameMiliSec = endMiliSec - startTickMiliSec;
	m_maxFrameTimeMiliSec = max(m_maxFrameTimeMiliSec, frameMiliSec);
	CString msg;
	const int textHeight = 20;
	int textY = 50;
	msg.Format(_T("%lg msec"), double(frameMiliSec));
	dc.TextOut(0, textY, msg);
	textY += textHeight;
	msg.Format(_T("%zu points drawn in a frame"), nDrawnPointInFrame);
	dc.TextOut(0, textY, msg);
	textY += textHeight;
	msg.Format(_T("%I64u points drawn(%lg msec, max %lg msec/frame)"),
		m_nDrawnPoint, double(endMiliSec - m_totalStartTickMiliSec), double(m_maxFrameTimeMiliSec));
	dc.TextOut(0, textY, msg);

	bool isViewUpdaed = nDrawnPointInFrame > 0;
	if (m_graphics.IsProgressiveViewMode() && isViewUpdaed) {
		UINT nEllapse = 0;
		if (m_progressiveViewTimerId == 0) {
			m_progressiveViewTimerId = SetTimer(ID_PROGRESSIVE_VIEW_TIMER, nEllapse, nullptr);
		}
	}
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


void CChildView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_PROGRESSIVE_VIEW_TIMER) {
		KillTimer(m_progressiveViewTimerId);
		m_progressiveViewTimerId = 0;
		m_restartProgressiveView = false;
		Invalidate(FALSE);
	}
	CWnd::OnTimer(nIDEvent);
}


BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
	// Background would be erased in OnPaint().
	m_restartProgressiveView = true;
	return TRUE;
	//return CWnd::OnEraseBkgnd(pDC);
}
