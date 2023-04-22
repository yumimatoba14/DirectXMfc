
// MainFrm.cpp : CMainFrame クラスの実装
//

#include "pch.h"
#include "framework.h"
#include "DirectXMfc.h"
#include "MainFrm.h"
#include "D3DPts2PointBlockListConverter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_OPEN_FILE, &CMainFrame::OnOpenFile)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ステータス ライン インジケーター
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame コンストラクション/デストラクション

CMainFrame::CMainFrame() noexcept
{
	// TODO: メンバー初期化コードをここに追加してください。
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// フレームのクライアント領域全体を占めるビューを作成します。
	if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, nullptr))
	{
		TRACE0("ビュー ウィンドウを作成できませんでした。\n");
		return -1;
	}

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("ツール バーの作成に失敗しました。\n");
		return -1;      // 作成できない場合
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("ステータス バーの作成に失敗しました。\n");
		return -1;      // 作成できない場合
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	// TODO: ツール バーをドッキング可能にしない場合は、これらの 3 行を削除してください。
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;
	// TODO: この位置で CREATESTRUCT cs を修正して Window クラスまたはスタイルを
	//  修正してください。

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

// CMainFrame の診断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame メッセージ ハンドラー

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// ビュー ウィンドウにフォーカスを与えます。
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// ビューに最初にコマンドを処理する機会を与えます。
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// それ以外の場合は、既定の処理を行います。
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

static CString ReplaceFileExtension(const CString& inputPath, LPCTSTR pNewExt)
{
	P_ASSERT(pNewExt[0] == _T('.'));
	int extPos = inputPath.ReverseFind(_T('.'));
	// Ignore period at the head.
	if (0 < extPos) {
		return inputPath.Left(extPos) + pNewExt;
	}
	return inputPath + pNewExt;
}

void CMainFrame::OnOpenFile()
{
	LPCTSTR pFilter = _T(
		"View file(*.bin)|*.bin|"
		"PTS file(*.pts)|*.pts||"
	);
	CFileDialog dialog(TRUE, nullptr, nullptr, 6UL, pFilter);
	if (dialog.DoModal() != IDOK) {
		return;
	}
	CString fileExt = dialog.GetFileExt();
	if (fileExt.CompareNoCase(_T("bin")) == 0) {
		m_wndView.LoadViewFile(dialog.GetPathName());
	} else if (fileExt.CompareNoCase(_T("pts")) == 0) {
		CString ptsFilePath = dialog.GetPathName();
		CString viewFilePath = ReplaceFileExtension(ptsFilePath, _T(".bin"));
		CFileDialog binFileDlg(FALSE, _T(".bin"), viewFilePath, 6UL, _T("View file(*.bin)|*.bin||"));
		if (binFileDlg.DoModal() != IDOK) {
			return;
		}
		viewFilePath = binFileDlg.GetPathName();

		CWaitCursor cursor;
		using namespace D3D11Graphics;
		D3DPts2PointBlockListConverter conv;
		conv.ConvertFile(ptsFilePath, viewFilePath);

		m_wndView.LoadViewFile(viewFilePath);
	}
}
