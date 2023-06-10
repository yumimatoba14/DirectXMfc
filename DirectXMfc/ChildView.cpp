
// ChildView.cpp : CChildView クラスの実装
//

#include "pch.h"
#include "framework.h"
#include <DirectXMath.h>
#include <vector>
#include <sstream>
#include "DirectXMfc.h"
#include "ChildView.h"
#include "D3DModelColorUtil.h"
#include "D3DModelPointList.h"
#include "PointListSampleModel.h"
#include "D3DModelTriangleList.h"
#include "D3DPts2PointBlockListConverter.h"
#include "SettingDialog.h"

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

void CChildView::LoadViewFile(LPCTSTR pFilePath)
{
	auto pModel = make_unique< MultiPointListSampleModel2>(pFilePath);
	pModel->PrepareBlockData();
	D3DAabBox3d modelAabb;
	for (auto& data : pModel->GetIntanceList()) {
		modelAabb.Extend(data.aabb.GetMinPoint());
		modelAabb.Extend(data.aabb.GetMaxPoint());
	}
	P_IS_TRUE(modelAabb.IsInitialized());

	D3DVector3d centerPoint = (modelAabb.GetMinPoint() + modelAabb.GetMaxPoint()) * 0.5;
	m_viewOp.SetEyePoint(centerPoint[0], centerPoint[1], centerPoint[2]);
	m_viewOp.SetUpDirection(0, 0, 1);
	m_viewOp.SetVerticalDirection(0, 0, 1);
	m_pModel = move(pModel);
	Invalidate();
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_SETTING, &CChildView::OnViewSetting)
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

void CChildView::SelectPointImpl(const CPoint& selectPointOnView)
{
	if (!m_pModel) {
		return;
	}
	m_graphics.SetProgressiveViewMode(false);
	m_graphics.DrawBeginForSelection();
	m_pModel->DrawTo(m_graphics);
	D3DSelectionTargetId pickedId = m_graphics.DrawEndForSelection(selectPointOnView);
	if (pickedId == D3D_SELECTION_TARGET_NULL) {
		m_selectedPoints.clear();
	}
	else {
		auto newPoints = m_pModel->SelectPoints(pickedId);
		copy(newPoints.begin(), newPoints.end(), back_inserter(m_selectedPoints));
	}

	Invalidate();
}

void CChildView::DrawSelectedEntities()
{
	vector<D3DGraphics3D::PointListVertex> vertices;
	for (auto pointVec : this->m_selectedPoints) {
		D3DGraphics3D::PointListVertex vertex;
		D3DVector::CopyTo(pointVec, &(vertex.pos));
		vertex.rgba = D3DModelColorUtil::Rgb(255, 0, 0);
		vertices.push_back(vertex);
	}
	m_graphics.DrawPointArray(vertices.data(), vertices.size());
	if (1 < vertices.size()) {
		m_graphics.DrawPolyline(vertices.data(), vertices.size());
	}
}

void CChildView::OnPaint()
{
	P_NOEXCEPT_BEGIN("CChildView::OnPaint");
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
		//m_pModel.reset(new MultiPointListSampleModel1(filePath)); m_graphics.SetPointSize(0.001);
		filePath = _T("..\\PointBlockList.bin");
		m_pModel.reset(new MultiPointListSampleModel2(filePath)); m_graphics.SetPointSize(0.001);
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
	if (!m_selectedPoints.empty()) {
		m_graphics.SetDrawSelectedEntityMode(true);
		DrawSelectedEntities();
		m_graphics.SetDrawSelectedEntityMode(false);
	}
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
	P_NOEXCEPT_END;
}



void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	CSize newSize(cx, cy);
	m_graphics.ResizeBuffers(newSize);
	m_viewOp.SetViewSize(newSize);
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
	m_viewOp.StartMouseMove(point, D3DViewOp::MOUSE_L_BUTTON);

	CWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (point != m_viewOp.GetMouseLastPoint()) {
		UpdateView();
	}
	if (!m_viewOp.IsMouseMoved()) {
		SelectPointImpl(point);
	}
	m_viewOp.EndMouseMove();

	CWnd::OnLButtonUp(nFlags, point);
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_viewOp.StartMouseMove(point, D3DViewOp::MOUSE_R_BUTTON);
	UpdateView();

	CWnd::OnRButtonDown(nFlags, point);
}


void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_viewOp.EndMouseMove(point);
	UpdateView();

	CWnd::OnRButtonUp(nFlags, point);
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

namespace {
	class ViewSettingDialog : public CSettingDialog
	{
	public:
		enum ValueIndex {
			POINT_SIZE = 1,
			VIEW_NEAR_Z,
			VIEW_FAR_Z
		};
		ViewSettingDialog(D3DGraphics3D& graphics) : m_graphics(graphics) {}

		int GetChangedCount() const { return m_numChanged; }
	protected:
		virtual BOOL OnInitDialog()
		{
			m_numChanged = 0;
			std::unique_ptr<CMFCPropertyGridProperty> pProperty(
				new CMFCPropertyGridProperty(_T("点サイズ"), COleVariant(m_graphics.GetPointSize()), _T("点の描画サイズ[mm]です"), POINT_SIZE)
			);
			m_propertyGrid.AddProperty(pProperty.release());
			std::unique_ptr<CMFCPropertyGridProperty> pGroup(new CMFCPropertyGridProperty(_T("透視投影")));
			pProperty.reset(
				new CMFCPropertyGridProperty(_T("Near Z"), COleVariant(m_graphics.GetPerspectiveViewNearZ()), _T("表示される範囲の視点からの距離の下限値[m]です"), VIEW_NEAR_Z)
			);
			pGroup->AddSubItem(pProperty.release());
			pProperty.reset(
				new CMFCPropertyGridProperty(_T("Far Z"), COleVariant(m_graphics.GetPerspectiveViewFarZ()), _T("表示される範囲の視点からの距離の上限値[m]です"), VIEW_FAR_Z)
			);
			pGroup->AddSubItem(pProperty.release());
			m_propertyGrid.AddProperty(pGroup.release());
			return CSettingDialog::OnInitDialog();
		}

		virtual void OnOK()
		{
			int nProperty = m_propertyGrid.GetPropertyCount();
			for (int iProperty = 0; iProperty < nProperty; ++iProperty) {
				CMFCPropertyGridProperty* pProp = m_propertyGrid.GetProperty(iProperty);
				ParsePropertyData(pProp);
			}
			CSettingDialog::OnOK();
		}
	private:
		void ParsePropertyData(CMFCPropertyGridProperty* pProp)
		{
			P_NOEXCEPT_BEGIN("ParsePropertyData");
			if (pProp->IsGroup()) {
				VisitPropertyTree(pProp);
				return;
			}
			if (!pProp->IsModified()) {
				return;
			}
			++m_numChanged;
			switch (pProp->GetData()) {
			case POINT_SIZE:
				m_graphics.SetPointSize(pProp->GetValue().dblVal);
				break;
			case VIEW_NEAR_Z:
				m_graphics.SetPerspectiveViewNearZ(pProp->GetValue().dblVal);
				break;
			case VIEW_FAR_Z:
				m_graphics.SetPerspectiveViewFarZ(pProp->GetValue().dblVal);
				break;
			}
			P_NOEXCEPT_END;
		}

		void VisitPropertyTree(CMFCPropertyGridProperty* pParent)
		{
			int nSubItem = pParent->GetSubItemsCount();
			for (int iSubItem = 0; iSubItem < nSubItem; ++iSubItem) {
				CMFCPropertyGridProperty* pSubItem = pParent->GetSubItem(iSubItem);
				ParsePropertyData(pSubItem);
			}
		}
	private:
		D3DGraphics3D& m_graphics;
		int m_numChanged = 0;
	};

}
void CChildView::OnViewSetting()
{
	ViewSettingDialog dlg(m_graphics);
	if (dlg.DoModal() != IDOK) {
		return;
	}
	if (0 < dlg.GetChangedCount()) {
		Invalidate(TRUE);
	}
}
