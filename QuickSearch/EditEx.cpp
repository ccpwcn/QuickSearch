// EditEx.cpp : ʵ���ļ�
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



// CEditEx ��Ϣ�������

BOOL CEditEx::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  �ڴ����ר�ô����/����û���
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
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	TCHAR szUserPath[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	TCHAR szDisplayName[MAX_PATH] = { 0 };
	CString szString = _T("ѡ��һ��Դ�ļ����ļ���");
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