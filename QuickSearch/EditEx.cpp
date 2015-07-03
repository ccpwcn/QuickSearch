// EditEx.cpp : 实现文件
//

#include "stdafx.h"
#include "QuickSearch.h"
#include "EditEx.h"


// CEditEx

IMPLEMENT_DYNAMIC(CEditEx, CEdit)

CEditEx::CEditEx()
{

}

CEditEx::~CEditEx()
{
}


BEGIN_MESSAGE_MAP(CEditEx, CEdit)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()



// CEditEx 消息处理程序

BOOL CEditEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
	/*if (pMsg->message == WM_KEYDOWN)
	{
		BOOL b = GetKeyState(VK_CONTROL) & 0x80;
		if (b && (pMsg->wParam == _T('a') || pMsg->wParam == _T('A')))
		{
			SetSel(0, -1);
			return TRUE;
		}
	}*/
	return CEdit::PreTranslateMessage(pMsg);
}


void CEditEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	TCHAR szUserPath[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	TCHAR szDisplayName[MAX_PATH] = { 0 };
	CString szString = _T("选择一个源文件子文件夹");
	bi.hwndOwner = ::GetFocus();
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDisplayName;
	bi.lpszTitle = szString;
	bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	LPITEMIDLIST pItemIDList = ::SHBrowseForFolder(&bi);
	if (pItemIDList == NULL)
	{
		return;
	}
	::SHGetPathFromIDList(pItemIDList, szUserPath);

	m_strSearchLocation = szUserPath;
	SetWindowText(szUserPath);

	// CEdit::OnLButtonDblClk(nFlags, point);
}

void CEditEx::UpdateSearchPath(__in const CString & strPath)
{
	if (strPath.GetLength() > 0)
		m_strSearchLocation = strPath;
}