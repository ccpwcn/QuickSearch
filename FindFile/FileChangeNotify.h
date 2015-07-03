#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <Shlobj.h>

#define SHCNRF_InterruptLevel		0x0001			//Interrupt level notifications from the file system
#define SHCNRF_ShellLevel			0x0002			//Shell-level notifications from the shell
#define SHCNRF_RecursiveInterrupt	0x1000			//Interrupt events on the whole subtree
#define SHCNRF_NewDelivery			0x8000			//Messages received use shared memorytypedef struct


typedef struct
{
	DWORD dwItem1;  // dwItem1 contains the previous PIDL or name of the folder.
	DWORD dwItem2;  // dwItem2 contains the new PIDL or name of the folder. 
}SHNotifyInfo;

typedef struct tagFILECHANGENOTIFY 
{
	tagFILECHANGENOTIFY()
	{
		m_dwRefCount = 0;
		memset(&m_fci, 0, sizeof(m_fci));
	}
	DWORD			m_dwRefCount;
	SHNotifyInfo	m_fci;
}FILE_CHANGE_NOTIFY, LPFILE_CHANGE_NOTIFY;

typedef ULONG(WINAPI * FN_SHChangeNotifyRegister)(__in HWND hWnd, int fSource, LONG fEvents, UINT wMsg, int cEntries, const SHChangeNotifyEntry * pfsne);
typedef BOOL(WINAPI * FN_SHChangeNotifyDeregister)(ULONG ulID);

class CFileChangeNotify
{
public:
	CFileChangeNotify(HINSTANCE hInstance);
	~CFileChangeNotify();
	DWORD SyncMonitor();
private:
	HMODULE		m_hShell32;
	BOOL		m_bReady;
	ULONG		m_ulNotifyId;			//×¢²áShell¼àÊÓº¯Êý×¢²áºÅ
	HANDLE		m_hWndThread;
	DWORD		m_dwWndThreadId;
	struct _stThreadParam
	{
		HINSTANCE	m_hInstance;
		HWND		m_hWnd;
		HANDLE		m_hEvent;
	}m_stThreadParam;
	FN_SHChangeNotifyRegister	m_pfnRegister;
	FN_SHChangeNotifyDeregister m_pfnDeregister;
	static DWORD WINAPI m_fnStartWnd(LPVOID lpParam);
};

