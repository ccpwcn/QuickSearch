#pragma once
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

// 事件通知消息
const UINT WM_FILE_CHANGE_NOTIFY = RegisterWindowMessage(_T("WM_FILE_CHANGE_MSG"));

class CNotifyWnd
{
public:
	CNotifyWnd(HINSTANCE hInstance, HWND & hwnd);

public:
	static void WINAPI SetSlogan(__in HDC & hdc, __in RECT & rc, __in LPCTSTR lpszSloganText);
	BOOL SaveNotifyInfo(__in LPCSTR lpszSource, __in LPCTSTR lpszDest);
	BOOL ParseNotifyInfo(__in HWND hWnd, __in WPARAM wParam, __in LPARAM lParam);
private:
	HINSTANCE	m_hInstance;
	TCHAR		m_szClassName[BUFSIZ];
	TCHAR		m_szWndTitle[BUFSIZ];
	BOOL InitInstance(HWND & hwnd);
	static LRESULT CALLBACK m_WindowProc(
		HWND hwnd, // handle to window
		UINT uMsg, // message identifier
		WPARAM wParam, // first message parameter
		LPARAM lParam // second message parameter
		);
};
