// FindFile.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "FileChangeNotify.h"
#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <locale.h>
#include <conio.h>

void printError(FILE * fp, TCHAR* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, 
		eNum,
		// MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // language is English (en-us)
		MAKELANGID(LANG_SYSTEM_DEFAULT, SUBLANG_SYS_DEFAULT),
		sysMsg, 
		256, 
		NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do { *p-- = 0; } while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	// Display the message
	_ftprintf_s(fp, TEXT("\n  WARNING: %s failed with error %d (%s)\n"), msg, eNum, sysMsg);
}

BOOL ListProcessModules(FILE * fp, DWORD dwPID)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		printError(fp, TEXT("CreateToolhelp32Snapshot (of modules)"));
		return(FALSE);
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof(MODULEENTRY32);

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if (!Module32First(hModuleSnap, &me32))
	{
		printError(fp, TEXT("Module32First"));  // show cause of failure
		CloseHandle(hModuleSnap);           // clean the snapshot object
		return(FALSE);
	}

	// Now walk the module list of the process,
	// and display information about each module
	do
	{
		_ftprintf_s(fp, TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
		_ftprintf_s(fp, TEXT("\n     Executable     = %s"), me32.szExePath);
		_ftprintf_s(fp, TEXT("\n     Process ID     = 0x%08X"), me32.th32ProcessID);
		_ftprintf_s(fp, TEXT("\n     Ref count (g)  = 0x%04X"), me32.GlblcntUsage);
		_ftprintf_s(fp, TEXT("\n     Ref count (p)  = 0x%04X"), me32.ProccntUsage);
		_ftprintf_s(fp, TEXT("\n     Base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
		_ftprintf_s(fp, TEXT("\n     Base size      = %d"), me32.modBaseSize);

	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);
	return(TRUE);
}

BOOL ListProcessThreads(FILE * fp, DWORD dwOwnerPID)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return(FALSE);

	// Fill in the size of the structure before using it. 
	te32.dwSize = sizeof(THREADENTRY32);

	// Retrieve information about the first thread,
	// and exit if unsuccessful
	if (!Thread32First(hThreadSnap, &te32))
	{
		printError(fp, TEXT("Thread32First")); // show cause of failure
		CloseHandle(hThreadSnap);          // clean the snapshot object
		return(FALSE);
	}

	// Now walk the thread list of the system,
	// and display information about each thread
	// associated with the specified process
	do
	{
		if (te32.th32OwnerProcessID == dwOwnerPID)
		{
			_ftprintf_s(fp, TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID);
			_ftprintf_s(fp, TEXT("\n     Base priority  = %d"), te32.tpBasePri);
			_ftprintf_s(fp, TEXT("\n     Delta priority = %d"), te32.tpDeltaPri);
			_ftprintf_s(fp, TEXT("\n"));
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);
	return(TRUE);
}

int GetProcessList(__in const BOOL bListModules = FALSE, __in const BOOL bListThreads = FALSE, __in const BOOL bSave = FALSE)
{
	FILE * fp = NULL;
	if (bSave)
	{
		_tfopen_s(&fp, _T("Process.txt"), _T("w"));
		if (fp == NULL)
		{
			for (int i = 3; i >= 0; i--)
			{
				_tprintf_s(_T("\r创建文件失败，%d秒之后程序继续运行..."), i);
				Sleep(1000);
			}
			fp = stderr;
		}
	}
	else
	{
		fp = stdout;
	}

	DWORD dwCurrentSessionId = WTSGetActiveConsoleSessionId();

	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError(fp, TEXT("CreateToolhelp32Snapshot (of processes)"));
		return 0;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		printError(fp, TEXT("Process32First")); // show cause of failure
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return 0;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	DWORD dwCount = 1;
	do
	{
		_ftprintf_s(fp, TEXT("\n-------------------------------------------------------"));
		// _ftprintf_s(fp, TEXT("\n\n====================================================="));
		_ftprintf_s(fp, TEXT("\nPROCESS NAME (%d):  %s"), dwCount, pe32.szExeFile);
		// _ftprintf_s(fp, TEXT("\n-------------------------------------------------------"));

		// Retrieve the priority class.
		/*dwPriorityClass = 0;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if (hProcess == NULL)
			printError(TEXT("OpenProcess"));
		else
		{
			dwPriorityClass = GetPriorityClass(hProcess);
			if (!dwPriorityClass)
				printError(TEXT("GetPriorityClass"));
			CloseHandle(hProcess);
		}*/

		_ftprintf_s(fp, TEXT("\n  PID          = 0x%08X"), pe32.th32ProcessID);
		_ftprintf_s(fp, TEXT("      Parent PID = 0x%08X"), pe32.th32ParentProcessID);
		_ftprintf_s(fp, TEXT("\n  Thread count = %d"), pe32.cntThreads);
		// _ftprintf_s(fp, TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase);
		// if (dwPriorityClass)
			// _ftprintf_s(fp, TEXT("\n  Priority class    = %d"), dwPriorityClass);

		DWORD dwPidRelationSessionId = 0;
		ProcessIdToSessionId(pe32.th32ProcessID, &dwPidRelationSessionId);
		_ftprintf_s(fp, TEXT("               Session ID = %d"), dwPidRelationSessionId);
		
		// List the modules and threads associated with this process
		if (bListModules)
			ListProcessModules(fp, pe32.th32ProcessID);
		if (bListThreads)
			ListProcessThreads(fp, pe32.th32ProcessID);

		dwCount++;
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	_tprintf(TEXT("\n"));
	if (bSave && fp)
	{
		fclose(fp);
		HWND hWnd = GetConsoleWindow();
		if (hWnd && IsWindow(hWnd))
			ShowWindow(hWnd, SW_SHOWMINIMIZED);
		_tsystem(_T("notepad.exe Process.txt"));
		if (hWnd && IsWindow(hWnd))
			ShowWindow(hWnd, SW_SHOWNORMAL);
	}
	return dwCount;
}

int ProcessMgr(int argc, _TCHAR* argv[])
{
	BOOL bListModules = FALSE, bListThreads = FALSE, bSave = FALSE;
	_tprintf_s(_T("按下m键显示进程加载的模块信息，否则请按任意键继续："));
	TCHAR cch = _getche();
	if (cch == _T('m') || cch == _T('M'))
		bListModules = TRUE;
	_tprintf_s(_T("\n"));

	_tprintf_s(_T("按下t键显示进程运行的线程信息，否则请按任意键继续："));
	cch = _getche();
	if (cch == _T('t') || cch == _T('T'))
		bListThreads = TRUE;
	_tprintf_s(_T("\n"));

	_tprintf_s(_T("按下s键保存所有输出信息到文件，否则请按任意键继续："));
	cch = _getche();
	if (cch == _T('s') || cch == _T('s'))
		bSave = TRUE;
	_tprintf_s(_T("\n"));

	int nProcessCount = GetProcessList(bListModules, bListThreads, bSave);

	_tprintf_s(_T("提示：%d个进程信息显示完毕，按任意键退出程序...\n"), nProcessCount);
	cch = _getche();

	return nProcessCount;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// Configure per-thread locale to cause all subsequently created 
	// threads to have their own locale.
	_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);

	// Set the locale of the main thread to US English.
	_tsetlocale(LC_ALL, _T(".OCP"));

	// ProcessMgr(argc, argv);
	
	CFileChangeNotify Notify(GetModuleHandle(NULL));
	// Notify.SyncMonitor();

	return 0;
}

