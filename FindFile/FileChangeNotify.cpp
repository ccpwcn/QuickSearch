#include "stdafx.h"
#include "FileChangeNotify.h"
#include "NotifyWnd.h"

CFileChangeNotify::CFileChangeNotify(HINSTANCE hInstance)
{
	m_bReady = FALSE;
	DWORD dwRet = 0;
	m_ulNotifyId = 0;

	// 数据准备，通过线程创建窗口
	m_stThreadParam.m_hInstance = hInstance;
	m_stThreadParam.m_hEvent = NULL;
	m_stThreadParam.m_hWnd = NULL;
	m_hWndThread = CreateThread(NULL, 0, m_fnStartWnd, (LPVOID)&m_stThreadParam, 0, &m_dwWndThreadId);
	if (m_hWndThread == NULL)
	{
		_tprintf_s(_T("CreateThread failed, status:%d\n"), dwRet = GetLastError());
		return;
	}
	for (int i = 0; i < 20; i++)
	{
		if (m_stThreadParam.m_hWnd && IsWindow(m_stThreadParam.m_hWnd))
			break;
		else
			Sleep(1000);
	}
	if (m_stThreadParam.m_hWnd == NULL || !IsWindow(m_stThreadParam.m_hWnd))
	{
		_tprintf_s(_T("CreateWindow failed!\n"));
		return;
	}

	//加载Shell32.dll
	HMODULE m_hShell32 = LoadLibrary(_T("Shell32.dll"));
	if (m_hShell32 == NULL)
		return ;
	
	//取函数地址
	m_pfnRegister = (FN_SHChangeNotifyRegister)GetProcAddress(m_hShell32, MAKEINTRESOURCEA(2));
	m_pfnDeregister = (FN_SHChangeNotifyDeregister)GetProcAddress(m_hShell32, MAKEINTRESOURCEA(4));
	if (m_pfnRegister == NULL || m_pfnDeregister == NULL)
	{
		_tprintf_s(_T("GetProcAddress failed, status:%d\n"), dwRet = GetLastError());
		return;
	}
	
	SHChangeNotifyEntry shEntry = { 0 };
	shEntry.fRecursive = TRUE;
	shEntry.pidl = 0;
	m_ulNotifyId = m_pfnRegister(
		m_stThreadParam.m_hWnd,
		SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_RecursiveInterrupt | SHCNRF_NewDelivery,
		SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM | SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_RENAMEFOLDER,
		// SHCNE_ALLEVENTS,
		WM_FILE_CHANGE_NOTIFY,	//自定义消息
		1,
		(const SHChangeNotifyEntry *)&shEntry);
	if (m_ulNotifyId == 0)
	{
		_tprintf_s(_T("Notify register failed, status:%d\n"), dwRet = GetLastError());
		return ;
	}
	else
	{
		_tprintf_s(_T(">>>File monitor start success...\n"));
		_tprintf_s(_T(">>>Now, waitting any change...\n"));
	}

	m_bReady = TRUE;
	WaitForSingleObject(m_hWndThread, INFINITE);
	CloseHandle(m_hWndThread);
	m_hWndThread = NULL;
}

CFileChangeNotify::~CFileChangeNotify()
{
	// TODO:
	SendMessage(m_stThreadParam.m_hWnd, WM_CLOSE, 0, 0);

	if (m_hWndThread)
	{
		SendMessage(m_stThreadParam.m_hWnd, WM_CLOSE, 0, 0);
		WaitForSingleObject(m_hWndThread, INFINITE);
		CloseHandle(m_hWndThread);
	}

	if (m_ulNotifyId)
		m_pfnDeregister(m_ulNotifyId);

	if (m_hShell32)
		CloseHandle(m_hShell32);
}

DWORD CFileChangeNotify::SyncMonitor()
{
	DWORD dwRet = 0;
	HANDLE hFile = CreateFile(
		_T("C:\\"),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
		);
	dwRet = GetLastError();
	if (hFile == INVALID_HANDLE_VALUE)   return dwRet;

	char buf[2 * (sizeof(FILE_NOTIFY_INFORMATION)+MAX_PATH)] = { 0 };
	FILE_NOTIFY_INFORMATION * pNotify = (FILE_NOTIFY_INFORMATION *)buf;
	DWORD BytesReturned = 0;
	_tprintf_s(_T("Start monitor...\n"));
	while (TRUE)
	{
		if (ReadDirectoryChangesW(
			hFile,
			pNotify,
			sizeof(buf),
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_CREATION,
			&BytesReturned,
			NULL,
			NULL))
		{
			if (pNotify->NextEntryOffset != 0)
			{
				PFILE_NOTIFY_INFORMATION p = (PFILE_NOTIFY_INFORMATION)((char*)pNotify + pNotify->NextEntryOffset);
				_tprintf_s(_T("FileChange:'%s'--->'%s'\n"), pNotify->FileName, p->FileName);
			}
		}
		else
		{
			dwRet = GetLastError();
			break;
		}

		memset(buf, 0, 2 * (sizeof(FILE_NOTIFY_INFORMATION)+MAX_PATH));
	}

	CloseHandle(hFile);
	return dwRet;
}

DWORD WINAPI CFileChangeNotify::m_fnStartWnd(LPVOID lpParam)
{
	_stThreadParam * lpExchange = (_stThreadParam *)lpParam;
	if (lpExchange == NULL)		return ERROR_INVALID_PARAMETER;

	CNotifyWnd(lpExchange->m_hInstance, lpExchange->m_hWnd);

	return 0;
}