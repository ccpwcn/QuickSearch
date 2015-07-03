// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "SearchIndex.h"

HANDLE g_hQuitEvent = NULL;
HANDLE g_hWorkThread = NULL;
HMODULE g_hModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	static TCHAR szHostFilename[MAX_PATH] = { 0 };
	LPCTSTR lpszCurrentProcessName = NULL;
	g_hModule = hModule;

	GetModuleFileName(NULL, szHostFilename, MAX_PATH - 1);
	lpszCurrentProcessName = _tcsrchr(szHostFilename, _T('\\'));
	if (lpszCurrentProcessName)
		lpszCurrentProcessName++;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hQuitEvent = CreateEvent(
			NULL,               // default security attributes
			TRUE,               // manual-reset event
			FALSE,              // initial state is nonsignaled
			NULL  // object name
			);
		g_hWorkThread = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			WorkThreadProc,       // thread function name
			NULL,          // argument to thread function 
			0,                      // use default creation flags 
			NULL);   // returns the thread identifier
		if (g_hWorkThread == NULL)
			return FALSE;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		if (_tcsicmp(lpszCurrentProcessName, _T("explorer.exe")) == 0)
			SetEvent(g_hQuitEvent);
		break;
	}
	return TRUE;
}

