
// DirectXMfc.h : DirectXMfc アプリケーションのメイン ヘッダー ファイル
//
#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'pch.h' をインクルードしてください"
#endif

#include "resource.h"       // メイン シンボル


// CDirectXMfcApp:
// このクラスの実装については、DirectXMfc.cpp を参照してください
//

class CDirectXMfcApp : public CWinApp
{
public:
	CDirectXMfcApp() noexcept;


// オーバーライド
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 実装

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CDirectXMfcApp theApp;
