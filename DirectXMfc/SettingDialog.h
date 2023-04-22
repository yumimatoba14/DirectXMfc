#pragma once


// CSettingDialog ダイアログ

class CSettingDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingDialog)

private:
	class PropertyGridCtrl : public CMFCPropertyGridCtrl
	{
	public:
		void SetLeftColumnWidth(int width) { m_nLeftColumnWidth = width; }
	};
public:
	CSettingDialog(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CSettingDialog();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING };
#endif

public:
	PropertyGridCtrl m_propertyGrid;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
};
