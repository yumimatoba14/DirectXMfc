
// ChildView.h : CChildView クラスのインターフェイス
//


#pragma once

#include "D3DGraphics.h"

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
	struct Model {
		D3D11Graphics::D3DBufferPtr pVertexBuffer;
		D3D11Graphics::D3DBufferPtr pIndexBuffer;
		size_t vertexSize{};
		size_t nVertex{};
		size_t nIndex{};
		Model() : vertexSize(0), nVertex(0), nIndex(0) {}
	};

	void PrepareModels();
	void UpdateShaderParam();
private:
	D3D11Graphics::D3DGraphics m_graphics;
	D3D11Graphics::D3DBufferPtr m_pShaderParamConstBuf;
	D3D11Graphics::D3DShaderContext m_sampleShaderContext;
	Model m_model0;

	// 生成された、メッセージ割り当て関数
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

