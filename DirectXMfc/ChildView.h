
// ChildView.h : CChildView クラスのインターフェイス
//


#pragma once

#include "D3DGraphics3D.h"
#include "D3DViewOp.h"
#include "D3DDrawingModel.h"
#include <memory>

// CChildView ウィンドウ

class CChildView : public CWnd
{
// コンストラクション
public:
	CChildView();

// 属性
public:

// 操作
public:

// オーバーライド
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// 実装
public:
	virtual ~CChildView();

private:
	void UpdateShaderParam();
	void UpdateView();
private:
	D3D11Graphics::D3DGraphics3D m_graphics;
	D3DViewOp m_viewOp;
	std::unique_ptr<D3D11Graphics::D3DDrawingModel> m_pModel;
	UINT_PTR m_progressiveViewTimerId = 0;
	bool m_restartProgressiveView = true;
	uint64_t m_nDrawnPoint = 0;
	ULONGLONG m_totalStartTickMiliSec = 0;
	ULONGLONG m_maxFrameTimeMiliSec = 0;

	// 生成された、メッセージ割り当て関数
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

