// SettingDialog.cpp : 実装ファイル
//

#include "pch.h"
#include "DirectXMfc.h"
#include "SettingDialog.h"
#include "afxdialogex.h"


// CSettingDialog ダイアログ

IMPLEMENT_DYNAMIC(CSettingDialog, CDialogEx)

CSettingDialog::CSettingDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING, pParent)
{

}

CSettingDialog::~CSettingDialog()
{
}

void CSettingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTING_DLG_PROPERTYGRID, m_propertyGrid);
}


BEGIN_MESSAGE_MAP(CSettingDialog, CDialogEx)
END_MESSAGE_MAP()


// CSettingDialog メッセージ ハンドラー


BOOL CSettingDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO: ここに初期化を追加してください
	CRect gridRect;
	m_propertyGrid.GetClientRect(&gridRect);
	m_propertyGrid.SetLeftColumnWidth(gridRect.Width() / 2);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 例外 : OCX プロパティ ページは必ず FALSE を返します。
}


void CSettingDialog::OnOK()
{
	// TODO: ここに特定なコードを追加するか、もしくは基底クラスを呼び出してください。

	CDialogEx::OnOK();
}
