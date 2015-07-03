#include "stdafx.h"

#include <objbase.h>
#include "FileChangeNotify.h"
#include "NotifyWnd.h"

#define UPDATE_DB_TIMER_ID		0x0001

CNotifyWnd::CNotifyWnd(HINSTANCE hInstance, HWND & hwnd)
{
	m_hInstance = hInstance;
	StringCchCopy(m_szClassName, BUFSIZ - 1, _T("{C9B38145-C765-4F22-AD2D-7AA4F007D546}"));
	StringCchCopy(m_szWndTitle, BUFSIZ - 1, _T("Notify window frame"));

	MSG msg;

	if (!InitInstance(hwnd))
		return ;

	SetTimer(hwnd, UPDATE_DB_TIMER_ID, 1000, NULL);
	BOOL fGotMessage;
	while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 && fGotMessage != -1)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	KillTimer(hwnd, UPDATE_DB_TIMER_ID);
}


BOOL CNotifyWnd::InitInstance(HWND & hwnd)
{
	WNDCLASSEX wcx;

	// Fill in the window class structure with parameters 
	// that describe the main window. 
	wcx.cbSize = sizeof(wcx);          // size of structure 
	wcx.style = CS_HREDRAW | CS_VREDRAW;                    // redraw if size changes 
	wcx.lpfnWndProc = m_WindowProc;     // points to window procedure 
	wcx.cbClsExtra = 0;                // no extra class memory 
	wcx.cbWndExtra = 0;                // no extra window memory 
	wcx.hInstance = m_hInstance;         // handle to instance 
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);              // predefined app. icon 
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow 
	wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);                  // white background brush 
	wcx.lpszMenuName = _T("MainMenu");    // name of menu resource 
	wcx.lpszClassName = m_szClassName;  // name of window class 
	wcx.hIconSm = (HICON)LoadImage(
		m_hInstance, // small class icon 
		MAKEINTRESOURCE(5),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	// Register the window class. 
	if (!RegisterClassEx(&wcx))
		return FALSE;

	// Create the main window. 

	hwnd = CreateWindowEx(
		NULL,
		m_szClassName,        // name of window class 
		m_szWndTitle,            // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		CW_USEDEFAULT,       // default width 
		CW_USEDEFAULT,       // default height 
		(HWND)NULL,         // no owner window 
		(HMENU)NULL,        // use class menu 
		m_hInstance,           // handle to application instance 
		(LPVOID)this);      // set class self into window-creation data 

	if (!hwnd)
		return FALSE;

	// Show the window and send a WM_PAINT message to the window 
	// procedure. 

	// ShowWindow(hwnd, SW_SHOWNORMAL);
	// UpdateWindow(hwnd);
	// ShowWindow(hwnd, SW_SHOWMINIMIZED);
	return TRUE;
}

BOOL CNotifyWnd::SaveNotifyInfo(__in LPCSTR lpszSource, __in LPCTSTR lpszDest)
{
	_tprintf_s(_T("Change info has be Saved!\n"));
	return TRUE;
}

BOOL CNotifyWnd::ParseNotifyInfo(__in HWND hWnd, __in WPARAM wParam, __in LPARAM lParam)
{
	TCHAR szSrc[MAX_PATH] = { 0 };
	TCHAR szDes[MAX_PATH] = { 0 };

	DWORD dwRet = 0;
	PIDLIST_ABSOLUTE * pppidl = NULL;
	LONG nEvent = 0;
	HANDLE hLock = SHChangeNotification_Lock(
		HANDLE(wParam),
		DWORD(lParam),
		&pppidl,
		&nEvent);
	if (pppidl == NULL)
	{
		_tprintf_s(_T("Get notify infomation failed, status:%d\n"), dwRet = GetLastError());
		return FALSE;
	}
	switch (nEvent)
	{
	case SHCNE_MKDIR:			// 一个目录被创建
		SHGetPathFromIDList(*pppidl, szSrc);
		_tprintf_s(_T("[CREATE DIRECTORY]:%s\n"), szSrc);
		break;

	case SHCNE_RMDIR:			// 一个目录被删除
		SHGetPathFromIDList(*pppidl, szSrc);
		_tprintf_s(_T("[DELETE DIRECTORY]:%s\n"), szSrc);
		break;

	case SHCNE_RENAMEFOLDER:	// 一个目录被重命名
		SHGetPathFromIDList(*pppidl, szSrc);
		pppidl++;
		SHGetPathFromIDList(*pppidl, szDes);
		_tprintf_s(_T("[RENAME DIRECTORY]:%s--->%s\n"), szSrc, szDes);
		break;

	case SHCNE_CREATE:			// 文件被创建
		SHGetPathFromIDList(*pppidl, szSrc);
		_tprintf_s(_T("[CREATE FILE]:%s\n"), szSrc);
		break;

	case SHCNE_DELETE:			// 文件被删除
		SHGetPathFromIDList(*pppidl, szSrc);
		_tprintf_s(_T("[DELETE FILE]:%s\n"), szSrc);
		break;
	case SHCNE_RENAMEITEM:		// 文件被重命名
		SHGetPathFromIDList(*pppidl, szSrc);
		pppidl++;
		SHGetPathFromIDList(*pppidl, szDes);
		_tprintf_s(_T("[RENAME FILE]:%s--->%s\n"), szSrc, szDes);
		break;
	default:
		_tprintf_s(_T("[NOTE]:I do't care this change.\n"));
		break;
	}
	SHChangeNotification_Unlock(hLock);

	return TRUE;
}

void WINAPI CNotifyWnd::SetSlogan(__in HDC & hdc, __in RECT & rc, __in LPCTSTR lpszSloganText)
{
	if (lpszSloganText == NULL)	return;

	SetBkMode(hdc, TRANSPARENT);
	int nWidth = (rc.right - rc.left) / 50;
	int nHigh = (rc.bottom - rc.top) / 20;
	HFONT hFont = CreateFont(
		62,      //字体的逻辑高度
		42,			 //逻辑平均字符宽度
		0,           //与水平线的角度
		0,           //基线方位角度
		FW_REGULAR,  //字形：常规
		FALSE,       //字形：斜体
		FALSE,       //字形：下划线
		FALSE,       //字形：粗体
		GB2312_CHARSET,          //字符集
		OUT_DEFAULT_PRECIS,      //输出精度
		CLIP_DEFAULT_PRECIS,     //剪截精度
		PROOF_QUALITY,           //输出品质
		FIXED_PITCH | FF_MODERN, //倾斜度
		_T("Cambria")                  //字体
		);
	HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);       //选择字体
	SetTextColor(hdc, RGB(233, 108, 82));
	int x = (rc.right - rc.left) / 2 - ((int)_tcslen(lpszSloganText) / 2 * 42);
	int y = (rc.bottom - rc.top) / 2 - 20;
	TextOut(hdc, x, y, lpszSloganText, (int)_tcslen(lpszSloganText));
	SelectObject(hdc, hFontOld);                      //选择旧字体
	DeleteObject(hFont);                              //删除新字体
}

LRESULT CALLBACK CNotifyWnd::m_WindowProc(
	HWND hwnd, // handle to window
	UINT uMsg, // message identifier
	WPARAM wParam, // first message parameter
	LPARAM lParam // second message parameter
	)
{
	static CNotifyWnd * lpNotify = NULL;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rc;

	static TCHAR szSloganText[] = _T("Quick Search");
	switch (uMsg)
	{
	case WM_CREATE:
		lpNotify = (CNotifyWnd *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

	case WM_PAINT:
		GetClientRect(hwnd, &rc);
		hdc = BeginPaint(hwnd, &ps);
		SetSlogan(hdc, rc, szSloganText);
		EndPaint(hwnd, &ps);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case 0x51:
			if (GetKeyState(VK_LSHIFT) < 0)
			{
				_tprintf_s(_T("\n  [NOTE]Recieved quit command!!!\n"));
				PostMessage(hwnd, WM_QUIT, 0, 0);
			}
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		if (uMsg == WM_FILE_CHANGE_NOTIFY)
		{
			if (lpNotify)
				lpNotify->ParseNotifyInfo(hwnd, wParam, lParam);
		}
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}