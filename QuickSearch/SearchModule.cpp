#include "stdafx.h"

#include "SearchModule.h"
#include "QuickSearchDlg.h"
#include "CommonModule.h"
#include <strsafe.h>
#include <iostream>
#include <vector>
#include <Shlwapi.h>

#ifdef _UNICODE
typedef std::wstring STRING;
#else
typedef std::string STRING;
#endif

#pragma comment(lib, "Shlwapi.lib")

const short nMSG_SIZE = 280;

CMutex g_clsMutex(FALSE, NULL);
static std::vector<FIND_DATA> g_vecCurrentFindData;
DWORD g_nTotalFound = 0;		// 已经找到的数量
DWORD g_nTotalView = 0;			// 已经查找了的数量

typedef void(CALLBACK * FN_UPDATE)(CQuickSearchDlg * lpDlg, FIND_DATA * lpfd, LPCTSTR lpszMsg);

typedef struct _tagSearchProgressInfo
{
	CQuickSearchDlg *		m_lpDlg;				// 界面类
	TCHAR					m_szStartLocation[MAX_PATH];	// 当前搜索目录起始位置
	BOOL					m_bFastMode;					// 高速模式
	ULONG					m_nFoundCount;					// 已经找到的计数
	ULONG					m_nViewCount;					// 搜索过的文件计数
	FN_UPDATE				m_Update;						// 回调函数指针
	std::vector<STRING>		m_vecSearchStrings;
}SEARCH_PROGRRESS_INFO, *LPSEARCH_PROGRRESS_INFO;

// 查找进度回调函数
void CALLBACK update(CQuickSearchDlg * lpDlg, FIND_DATA * lpfd, LPCTSTR lpszMsg)
{
	lpfd ? lpDlg->AddList(*lpfd) : 0;
	lpszMsg ? lpDlg->NotifyStatusMsg(lpszMsg) : 0;
}

// 函数功能：查找首个字符在搜索池中的偏移位置
static LPCTSTR __stdcall FindFirstChar(__in LPCTSTR lpszText, __in TCHAR cch)
{
	if (lpszText == NULL || lpszText[0] == _T('\0'))	return NULL;
	if (cch == _T('\0'))	return NULL;

	size_t nTextLen = _tcslen(lpszText);
	TCHAR cchText = _T('\0');
	for (size_t i = 0; i < nTextLen; i++)
	{
		if (lpszText[i] >= 65 && lpszText[i] <= 90)	// 如果是大写，转换成小写
			cchText = lpszText[i] + 32;
		else
			cchText = lpszText[i];
		if (cch >= 65 && cch <= 90)					// 如果是大写，转换成小写
			cch = cch + 32;

		if (cch == lpszText[i])
			return lpszText + i;
	}

	return NULL;
}

LPCTSTR __stdcall _StrStrI(__in LPCTSTR lpszText, __in LPCTSTR lpszPattern)
{
	if (lpszText == NULL || lpszText[0] == _T('\0'))	return NULL;
	if (lpszPattern == NULL || lpszPattern[0] == _T('\0'))	return NULL;

	size_t nTextLen = _tcslen(lpszText);
	size_t nPatternLen = _tcslen(lpszPattern);
	if (nPatternLen > nTextLen)		return NULL;

	// 查找首个字符在搜索池中的偏移位置，在数据量比较大的情况下，能有效降低时间复杂度
	LPCTSTR lpszStartPoint = FindFirstChar(lpszText, lpszPattern[0]);
	if (lpszStartPoint == NULL)
		return NULL;
	LPCTSTR lpszEndPoint = lpszText + nTextLen;
	size_t i = 0;
	for (LPCTSTR p = lpszStartPoint; p != lpszEndPoint; p++)
	{
		i = 0;
		TCHAR cchText = _T('\0'), cchPattern = _T('\0');
		BOOL bFound = TRUE;
		size_t j = 0;
		while (j < nPatternLen)
		{
			if (lpszPattern[j] >= 65 && lpszPattern[j] <= 90)	// 如果是大写，转换成小写
				cchPattern = lpszPattern[j] + 32;
			else
				cchPattern = lpszPattern[j];
			if (p[i] >= 65 && p[i] <= 90)	// 如果是大写，转换成小写
				cchText = p[i] + 32;
			else
				cchText = p[i];

			if (cchPattern != cchText)		// 匹配失败，本轮查找结束
			{
				bFound = FALSE;
				p = p + j;
				break;
			}
			i++;
			j++;
		}

		if (!bFound)
			return NULL;
		else
			return p + i - nPatternLen;
	}
	return NULL;
}


int ParseSearchObject(__in LPTSTR lpszSearchObject, __out std::vector<STRING> & vecStrings)
{
	if (lpszSearchObject == NULL || lpszSearchObject[0] == _T('\0'))	return 0;
	if (vecStrings.size() > 0)	return 0;

	int nCount = 0;

	STRING strTemp;
	if (_tcschr(lpszSearchObject, _T('*')) == NULL && _tcschr(lpszSearchObject, _T('?')) == NULL)
	{
		vecStrings.push_back(lpszSearchObject);
		nCount++;
	}
	else
	{
		TCHAR szSeps1[] = _T("*");
		LPTSTR lpszToken1 = NULL;
		LPTSTR lpszNextToken1 = NULL;
		lpszToken1 = _tcstok_s(lpszSearchObject, szSeps1, &lpszNextToken1);

		while (lpszToken1 != NULL)
		{
			TCHAR szSeps2[] = _T("?");
			LPTSTR lpszToken2 = NULL;
			LPTSTR lpszNextToken2 = NULL;

			lpszToken2 = _tcstok_s(lpszToken1, szSeps2, &lpszNextToken2);
			if (lpszToken2 == NULL)
			{
				vecStrings.push_back(lpszToken1);
				nCount++;
			}
			else
			{
				while (lpszToken2 != NULL)
				{
					vecStrings.push_back(lpszToken1);
					nCount++;
					lpszToken2 = _tcstok_s(NULL, szSeps2, &lpszNextToken2);
				}
			}
			lpszToken1 = _tcstok_s(NULL, szSeps1, &lpszNextToken1);
		}
	}

	return nCount;
}

DWORD GetUserFileSize(__in CString & strFilename)
{
	DWORD dwSize = 0;
	HANDLE hFile = CreateFile(
		strFilename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		dwSize = GetFileSize(hFile, NULL);
		CloseHandle(hFile);
	}
	return dwSize;
}

// 在指定的磁盘查找符合要求的数据，使用递归查找，支持通配符
void RecursiveTraverseFind(__in LPSEARCH_PROGRRESS_INFO lpProgressInfo)
{
	CFileFind fileFinder;
	CString strFilePath, strFileNotMatchPath;
	static short nFirstCall = 0;

	strFileNotMatchPath = lpProgressInfo->m_szStartLocation;
	nFirstCall = 1;
	/*if (lpProgressInfo->m_szStartLocation[_tcslen(lpProgressInfo->m_szStartLocation) - 1] == _T('\\'))
	{
		strFileNotMatchPath.Delete(strFileNotMatchPath.GetLength() - 1);
		strFilePath.Format(_T("%s*.*"), lpProgressInfo->m_szStartLocation);
	}
	else*/
	{
		strFilePath.Format(_T("%s\\*.*"), lpProgressInfo->m_szStartLocation);
	}

	// 如果收到了中断信号，立即返回
	if (g_hBrokenEvent && WaitForSingleObject(g_hBrokenEvent, 5) == WAIT_OBJECT_0)
		return;

	// 先按所有文件查找，看有没有文件，如果有，再往下走，如果没有，直接返回
	BOOL bWorking = fileFinder.FindFile(strFilePath);
	TCHAR szMsg[nMSG_SIZE] = { 0 };
	while (bWorking)
	{
		bWorking = fileFinder.FindNextFile();

		if (fileFinder.IsDots())
			continue;

		lpProgressInfo->m_nViewCount++;

		// 不管是目录还是文件，先按指定规则进行匹配
		CString strCurrentFile, strCurrentPath;
		strCurrentFile = fileFinder.GetFileName();
		strCurrentPath = fileFinder.GetFilePath();
		for (std::vector<STRING>::iterator it = lpProgressInfo->m_vecSearchStrings.begin(); it != lpProgressInfo->m_vecSearchStrings.end(); it++)
		{
			if (_StrStrI(strCurrentFile, it->c_str()))
			{
				FIND_DATA fd = { 0 };
				StringCchCopy(fd.m_szFilename, MAX_PATH - 1, strCurrentFile);
				StringCchCopy(fd.m_szLocation, MAX_PATH - 1, strCurrentPath);
				FILETIME ft = { 0 };
				if (fileFinder.GetLastWriteTime(&ft))
				{
					FileTimeToSystemTime(&ft, &fd.m_stFileDateAndTime);
				}
				fd.m_dwFileSize = GetUserFileSize(strCurrentPath);
				g_clsMutex.Lock();
				g_vecCurrentFindData.push_back(fd);
				lpProgressInfo->m_nFoundCount++;
				StringCchPrintf(
					szMsg,
					nMSG_SIZE - 1,
					_T("正在搜索[%d/%d]%s"),
					lpProgressInfo->m_nFoundCount,
					lpProgressInfo->m_nViewCount,
					strCurrentPath);
				lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, &fd, szMsg);
				g_clsMutex.Unlock();
			}
		}

		if (fileFinder.IsDirectory())
		{
			// 若是目录则递归调用此方法
			StringCchCopy(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, strCurrentPath);
			StringCchPrintf(
				szMsg,
				nMSG_SIZE - 1,
				_T("正在搜索[%d/%d]%s"),
				lpProgressInfo->m_nFoundCount,
				lpProgressInfo->m_nViewCount,
				strCurrentPath);
			g_clsMutex.Lock();
			lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, NULL, szMsg);
			g_clsMutex.Unlock();
			RecursiveTraverseFind(lpProgressInfo);
		}

		// 如果收到了中断信号，立即返回
		if (g_hBrokenEvent && WaitForSingleObject(g_hBrokenEvent, 5) == WAIT_OBJECT_0)
		{
			fileFinder.Close();
			return;
		}
	}

	fileFinder.Close();
}


void RecursiveTraverseFindStd(__in LPSEARCH_PROGRRESS_INFO lpProgressInfo)
{
	if (lpProgressInfo == NULL)		return;
	
	WIN32_FIND_DATA FindFileData = { 0 };
	HANDLE hListFile = INVALID_HANDLE_VALUE;
	TCHAR szFilePath[MAX_PATH] = { 0 }, szFilePathNotMatch[MAX_PATH] = { 0 };
	//构造代表子目录和文件夹路径的字符串，使用通配符“*”
	StringCchCopy(szFilePath, MAX_PATH - 1, lpProgressInfo->m_szStartLocation);
	//注释的代码可以用于查找所有以“.txt结尾”的文件。
	//StringCchCat(szFilePath, "\\*.txt");
	StringCchCopyN(szFilePathNotMatch, MAX_PATH - 1, lpProgressInfo->m_szStartLocation, _tcslen(szFilePath));
	StringCchCat(szFilePath, MAX_PATH - _tcslen(szFilePath) - 1, _T("\\*"));
	
	// 如果收到了中断信号，立即返回
	if (WaitForSingleObject(g_hBrokenEvent, 10) == WAIT_OBJECT_0)
		return;

	//查找第一个文件/目录，获得查找句柄
	hListFile = FindFirstFile(szFilePath, &FindFileData);
	//判断句柄
	if (hListFile == INVALID_HANDLE_VALUE)
		return ;

	static TCHAR szMsg[nMSG_SIZE] = { 0 };		// 预置为静态的，不必每次递归都初始化和处理，提高效率
	do
	{
		// 不需要代表本级目录和上级目录的“.”和“..”
		if ((FindFileData.cFileName[0] == _T('.') && FindFileData.cFileName[1] == _T('\0')) ||
			(FindFileData.cFileName[0] == _T('.') && FindFileData.cFileName[1] == _T('.') && FindFileData.cFileName[2] == _T('\0')))
			continue;

		// 不管是目录还是文件，先按指定规则进行匹配
		g_clsMutex.Lock();
		g_nTotalView++;
		lpProgressInfo->m_nViewCount++;
		g_clsMutex.Unlock();
		for (std::vector<STRING>::iterator it = lpProgressInfo->m_vecSearchStrings.begin(); it != lpProgressInfo->m_vecSearchStrings.end(); it++)
		{
			if (_StrStrI(FindFileData.cFileName, it->c_str()))
			{
				FIND_DATA fd = { 0 };
				StringCchCopy(fd.m_szFilename, MAX_PATH - 1, FindFileData.cFileName);
				StringCchCopy(fd.m_szLocation, MAX_PATH - 1, lpProgressInfo->m_szStartLocation);
				FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &fd.m_stFileDateAndTime);
				fd.m_dwFileSize = FindFileData.nFileSizeLow;
				g_clsMutex.Lock();
				g_vecCurrentFindData.push_back(fd);
				g_nTotalFound++;
				lpProgressInfo->m_nFoundCount++;
				StringCchPrintf(
					szMsg,
					nMSG_SIZE - 1,
					_T("正在搜索[%d/%d]%s"),
					g_nTotalFound,
					g_nTotalView,
					lpProgressInfo->m_szStartLocation);
				lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, &fd, NULL);
				g_clsMutex.Unlock();
				break;
			}
		}

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// 若是目录则递归调用此方法
			StringCchCopy(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, szFilePathNotMatch);
			StringCchCat(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, _T("\\"));
			StringCchCat(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, FindFileData.cFileName);
			g_clsMutex.Lock();
			StringCchPrintf(
				szMsg,
				nMSG_SIZE - 1,
				_T("正在搜索[%d/%d]%s"),
				g_nTotalFound,
				g_nTotalView,
				lpProgressInfo->m_szStartLocation);
			lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, NULL, szMsg);
			g_clsMutex.Unlock();
			RecursiveTraverseFindStd(lpProgressInfo);
		}

		if (WaitForSingleObject(g_hBrokenEvent, 10) == WAIT_OBJECT_0)
			break;
	} while (FindNextFile(hListFile, &FindFileData));
	
	FindClose(hListFile);
	return;
}



DWORD WINAPI SearchThreadProc(LPVOID lpParm)
{
	LPSEARCH_PROGRRESS_INFO lpProgressInfo = (LPSEARCH_PROGRRESS_INFO)lpParm;
	if (lpProgressInfo == NULL)	return ERROR_INVALID_PARAMETER;

	// 先检查搜索对象是否有通配符
	if (lpProgressInfo->m_bFastMode)
		RecursiveTraverseFindStd(lpProgressInfo);
	else
		RecursiveTraverseFind(lpProgressInfo);

	return 0;
}

BOOL EnablePrivilege(LPCTSTR lpszPrivilegeName, BOOL bEnable)
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES |
		TOKEN_QUERY | TOKEN_READ, &hToken))
		return FALSE;
	if (!LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid))
		return TRUE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

	AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, NULL, NULL);

	CloseHandle(hToken);

	return (GetLastError() == ERROR_SUCCESS);

}

DWORD WINAPI SearchWorkThreadMgrProc(LPVOID lpParm)
{
	CQuickSearchDlg * lpDlg = (CQuickSearchDlg *)lpParm;
	if (lpDlg == NULL)
	{
		lpDlg->NotifyStatusMsg(_T("线程的非法引用！请退出程序，然后重试。"));
		return ERROR_INVALID_PARAMETER;
	}
	if (!EnablePrivilege(SE_DEBUG_NAME, TRUE))
	{
		lpDlg->NotifyStatusMsg(_T("提权失败！"));
		return ERROR_ACCESS_DENIED;
	}
	for (int i = 0; lpDlg->GetSafeHwnd() == NULL && i < 10; i++)
		Sleep(100);

	lpDlg->NotifyStatusMsg(_T("就绪..."));
	while (TRUE)
	{
		TCHAR szMsg[nMSG_SIZE] = { 0 };
		ULONG nFoundedCont = 0;

		WaitForSingleObject(g_hSearchEvent, INFINITE);
		if (WaitForSingleObject(g_hQuitEvent, 50) == WAIT_OBJECT_0)
		{
			// 执行搜索之前检测有无退出信号
			break;
		}

		lpDlg->InitList();
		SEARCH_PROGRRESS_INFO spi[26] = { 0 };
		for (short i = 0; i < 26; i++)
		{
			spi[i].m_nFoundCount = 0;
			spi[i].m_nViewCount = 0;
			spi[i].m_lpDlg = lpDlg;
			spi[i].m_Update = update;
			ParseSearchObject(lpDlg->GetSearchString().GetBuffer(), spi[i].m_vecSearchStrings);
			lpDlg->GetSearchString().ReleaseBuffer();
		}

		g_nTotalFound = 0;			// 已经找到的数量
		g_nTotalView = 0;			// 已经查找了的数量
		CString strSearchStartPath(lpDlg->GetSearchLocation());
		short nSearchThreadIndex = 0;
		HANDLE hSearchThread[26] = { 0 };
		DWORD dwSeachThreadId[26] = { 0 };
		if (strSearchStartPath == _T("0"))
		{
			TCHAR szAllDriverLetters[100] = { 0 };
			DWORD len = GetLogicalDriveStrings(sizeof(szAllDriverLetters) / sizeof(TCHAR), szAllDriverLetters);
			for (TCHAR * lpszCurrentDriverLetter = szAllDriverLetters; *lpszCurrentDriverLetter; lpszCurrentDriverLetter += _tcslen(lpszCurrentDriverLetter) + 1)
			{
				// 创建搜索线程
				Sleep(nSearchThreadIndex * 1000);
				StringCchPrintf(spi[nSearchThreadIndex].m_szStartLocation, MAX_PATH - 1, _T("%C:"), lpszCurrentDriverLetter[0]);
				spi[nSearchThreadIndex].m_bFastMode = lpDlg->GetFastMode();
				hSearchThread[nSearchThreadIndex] = CreateThread(
					NULL,         // 使用默认的安全描述符
					0,            // 使用默认的栈大小
					(LPTHREAD_START_ROUTINE)SearchThreadProc,
					(LPVOID)(spi + nSearchThreadIndex),
					CREATE_SUSPENDED,						// 先挂起
					dwSeachThreadId + nSearchThreadIndex);	// 取得线程ID
				if (hSearchThread[nSearchThreadIndex])
				{
					if (spi[nSearchThreadIndex].m_bFastMode)
						if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_TIME_CRITICAL))
							if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
								if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_ABOVE_NORMAL))
									if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
										SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_NORMAL);
					ResumeThread(hSearchThread[nSearchThreadIndex]);
					SetThreadPriorityBoost(hSearchThread[nSearchThreadIndex], !spi[nSearchThreadIndex].m_bFastMode);	// 系统动态调整线程优先级选项
					nSearchThreadIndex++;
				}
			}
			WaitForMultipleObjects(nSearchThreadIndex, hSearchThread, TRUE, INFINITE);
			for (short i = 0; i < nSearchThreadIndex; i++)
			{
				if (hSearchThread[i])
					CloseHandle(hSearchThread[i]);
			}
		}
		else
		{
			// 创建搜索线程
			DWORD ThreadID = 0;
			StringCchPrintf(spi[0].m_szStartLocation, MAX_PATH - 1, _T("%s"), strSearchStartPath);
			spi[0].m_bFastMode = lpDlg->GetFastMode();
			hSearchThread[0] = CreateThread(
				NULL,         // 使用默认的安全描述符
				0,            // 使用默认的栈大小
				(LPTHREAD_START_ROUTINE)SearchThreadProc,
				(LPVOID)&spi[0],
				CREATE_SUSPENDED,						// 先挂起
				&ThreadID);	// 取得线程ID
			if (hSearchThread)
			{
				if (spi[0].m_bFastMode)
					if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_TIME_CRITICAL))
						if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
							if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_ABOVE_NORMAL))
								if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
									SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_NORMAL);
				ResumeThread(hSearchThread[nSearchThreadIndex]);
				SetThreadPriorityBoost(hSearchThread[nSearchThreadIndex], !spi[nSearchThreadIndex].m_bFastMode);	// 系统动态调整线程优先级选项

				WaitForSingleObject(hSearchThread[nSearchThreadIndex], INFINITE);
				CloseHandle(hSearchThread[nSearchThreadIndex]);
				nSearchThreadIndex++;
			}
		}
		if (g_vecCurrentFindData.size())
		{
			lpDlg->InitList();
			lpDlg->NotifyStatusMsg(_T("正在更新搜索结果列表，请稍候..."));
			lpDlg->UpdateList(g_vecCurrentFindData);
			ULONG nViewCount = 0, nFoundCount = 0;
			for (short i = 0; i < nSearchThreadIndex; i++)
			{
				nViewCount += spi[i].m_nViewCount;
				nFoundCount += spi[i].m_nFoundCount;
			}
			StringCchPrintf(szMsg, nMSG_SIZE - 1, _T("搜索完成，在%d个项目中共找到了%d个对象。"), nViewCount, nFoundCount);
			lpDlg->NotifyStatusMsg(szMsg);
		}
		else
		{
			lpDlg->NotifyStatusMsg(_T("未找到！"));
		}

		// 搜索完了，重置状态
		g_vecCurrentFindData.clear();
		std::vector<FIND_DATA> vecCurrentFindDataTemp;
		g_vecCurrentFindData.swap(vecCurrentFindDataTemp);
		g_hBrokenEvent ? ResetEvent(g_hBrokenEvent) : 0;
		g_hSearchEvent ? ResetEvent(g_hSearchEvent) : 0;

		if (WaitForSingleObject(g_hQuitEvent, 50) == WAIT_OBJECT_0)
		{
			// 搜索任务完成之后检测有无退出信号
			OutputDebugString(_T("Quiting..."));
			break;
		}
	}

	return 0;
}