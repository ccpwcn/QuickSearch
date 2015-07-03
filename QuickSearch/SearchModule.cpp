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
DWORD g_nTotalFound = 0;		// �Ѿ��ҵ�������
DWORD g_nTotalView = 0;			// �Ѿ������˵�����

typedef void(CALLBACK * FN_UPDATE)(CQuickSearchDlg * lpDlg, FIND_DATA * lpfd, LPCTSTR lpszMsg);

typedef struct _tagSearchProgressInfo
{
	CQuickSearchDlg *		m_lpDlg;				// ������
	TCHAR					m_szStartLocation[MAX_PATH];	// ��ǰ����Ŀ¼��ʼλ��
	BOOL					m_bFastMode;					// ����ģʽ
	ULONG					m_nFoundCount;					// �Ѿ��ҵ��ļ���
	ULONG					m_nViewCount;					// ���������ļ�����
	FN_UPDATE				m_Update;						// �ص�����ָ��
	std::vector<STRING>		m_vecSearchStrings;
}SEARCH_PROGRRESS_INFO, *LPSEARCH_PROGRRESS_INFO;

// ���ҽ��Ȼص�����
void CALLBACK update(CQuickSearchDlg * lpDlg, FIND_DATA * lpfd, LPCTSTR lpszMsg)
{
	lpfd ? lpDlg->AddList(*lpfd) : 0;
	lpszMsg ? lpDlg->NotifyStatusMsg(lpszMsg) : 0;
}

// �������ܣ������׸��ַ����������е�ƫ��λ��
static LPCTSTR __stdcall FindFirstChar(__in LPCTSTR lpszText, __in TCHAR cch)
{
	if (lpszText == NULL || lpszText[0] == _T('\0'))	return NULL;
	if (cch == _T('\0'))	return NULL;

	size_t nTextLen = _tcslen(lpszText);
	TCHAR cchText = _T('\0');
	for (size_t i = 0; i < nTextLen; i++)
	{
		if (lpszText[i] >= 65 && lpszText[i] <= 90)	// ����Ǵ�д��ת����Сд
			cchText = lpszText[i] + 32;
		else
			cchText = lpszText[i];
		if (cch >= 65 && cch <= 90)					// ����Ǵ�д��ת����Сд
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

	// �����׸��ַ����������е�ƫ��λ�ã����������Ƚϴ������£�����Ч����ʱ�临�Ӷ�
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
			if (lpszPattern[j] >= 65 && lpszPattern[j] <= 90)	// ����Ǵ�д��ת����Сд
				cchPattern = lpszPattern[j] + 32;
			else
				cchPattern = lpszPattern[j];
			if (p[i] >= 65 && p[i] <= 90)	// ����Ǵ�д��ת����Сд
				cchText = p[i] + 32;
			else
				cchText = p[i];

			if (cchPattern != cchText)		// ƥ��ʧ�ܣ����ֲ��ҽ���
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

// ��ָ���Ĵ��̲��ҷ���Ҫ������ݣ�ʹ�õݹ���ң�֧��ͨ���
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

	// ����յ����ж��źţ���������
	if (g_hBrokenEvent && WaitForSingleObject(g_hBrokenEvent, 5) == WAIT_OBJECT_0)
		return;

	// �Ȱ������ļ����ң�����û���ļ�������У��������ߣ����û�У�ֱ�ӷ���
	BOOL bWorking = fileFinder.FindFile(strFilePath);
	TCHAR szMsg[nMSG_SIZE] = { 0 };
	while (bWorking)
	{
		bWorking = fileFinder.FindNextFile();

		if (fileFinder.IsDots())
			continue;

		lpProgressInfo->m_nViewCount++;

		// ������Ŀ¼�����ļ����Ȱ�ָ���������ƥ��
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
					_T("��������[%d/%d]%s"),
					lpProgressInfo->m_nFoundCount,
					lpProgressInfo->m_nViewCount,
					strCurrentPath);
				lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, &fd, szMsg);
				g_clsMutex.Unlock();
			}
		}

		if (fileFinder.IsDirectory())
		{
			// ����Ŀ¼��ݹ���ô˷���
			StringCchCopy(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, strCurrentPath);
			StringCchPrintf(
				szMsg,
				nMSG_SIZE - 1,
				_T("��������[%d/%d]%s"),
				lpProgressInfo->m_nFoundCount,
				lpProgressInfo->m_nViewCount,
				strCurrentPath);
			g_clsMutex.Lock();
			lpProgressInfo->m_Update(lpProgressInfo->m_lpDlg, NULL, szMsg);
			g_clsMutex.Unlock();
			RecursiveTraverseFind(lpProgressInfo);
		}

		// ����յ����ж��źţ���������
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
	//���������Ŀ¼���ļ���·�����ַ�����ʹ��ͨ�����*��
	StringCchCopy(szFilePath, MAX_PATH - 1, lpProgressInfo->m_szStartLocation);
	//ע�͵Ĵ���������ڲ��������ԡ�.txt��β�����ļ���
	//StringCchCat(szFilePath, "\\*.txt");
	StringCchCopyN(szFilePathNotMatch, MAX_PATH - 1, lpProgressInfo->m_szStartLocation, _tcslen(szFilePath));
	StringCchCat(szFilePath, MAX_PATH - _tcslen(szFilePath) - 1, _T("\\*"));
	
	// ����յ����ж��źţ���������
	if (WaitForSingleObject(g_hBrokenEvent, 10) == WAIT_OBJECT_0)
		return;

	//���ҵ�һ���ļ�/Ŀ¼����ò��Ҿ��
	hListFile = FindFirstFile(szFilePath, &FindFileData);
	//�жϾ��
	if (hListFile == INVALID_HANDLE_VALUE)
		return ;

	static TCHAR szMsg[nMSG_SIZE] = { 0 };		// Ԥ��Ϊ��̬�ģ�����ÿ�εݹ鶼��ʼ���ʹ������Ч��
	do
	{
		// ����Ҫ������Ŀ¼���ϼ�Ŀ¼�ġ�.���͡�..��
		if ((FindFileData.cFileName[0] == _T('.') && FindFileData.cFileName[1] == _T('\0')) ||
			(FindFileData.cFileName[0] == _T('.') && FindFileData.cFileName[1] == _T('.') && FindFileData.cFileName[2] == _T('\0')))
			continue;

		// ������Ŀ¼�����ļ����Ȱ�ָ���������ƥ��
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
					_T("��������[%d/%d]%s"),
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
			// ����Ŀ¼��ݹ���ô˷���
			StringCchCopy(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, szFilePathNotMatch);
			StringCchCat(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, _T("\\"));
			StringCchCat(lpProgressInfo->m_szStartLocation, MAX_PATH - 1, FindFileData.cFileName);
			g_clsMutex.Lock();
			StringCchPrintf(
				szMsg,
				nMSG_SIZE - 1,
				_T("��������[%d/%d]%s"),
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

	// �ȼ�����������Ƿ���ͨ���
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
		lpDlg->NotifyStatusMsg(_T("�̵߳ķǷ����ã����˳�����Ȼ�����ԡ�"));
		return ERROR_INVALID_PARAMETER;
	}
	if (!EnablePrivilege(SE_DEBUG_NAME, TRUE))
	{
		lpDlg->NotifyStatusMsg(_T("��Ȩʧ�ܣ�"));
		return ERROR_ACCESS_DENIED;
	}
	for (int i = 0; lpDlg->GetSafeHwnd() == NULL && i < 10; i++)
		Sleep(100);

	lpDlg->NotifyStatusMsg(_T("����..."));
	while (TRUE)
	{
		TCHAR szMsg[nMSG_SIZE] = { 0 };
		ULONG nFoundedCont = 0;

		WaitForSingleObject(g_hSearchEvent, INFINITE);
		if (WaitForSingleObject(g_hQuitEvent, 50) == WAIT_OBJECT_0)
		{
			// ִ������֮ǰ��������˳��ź�
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

		g_nTotalFound = 0;			// �Ѿ��ҵ�������
		g_nTotalView = 0;			// �Ѿ������˵�����
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
				// ���������߳�
				Sleep(nSearchThreadIndex * 1000);
				StringCchPrintf(spi[nSearchThreadIndex].m_szStartLocation, MAX_PATH - 1, _T("%C:"), lpszCurrentDriverLetter[0]);
				spi[nSearchThreadIndex].m_bFastMode = lpDlg->GetFastMode();
				hSearchThread[nSearchThreadIndex] = CreateThread(
					NULL,         // ʹ��Ĭ�ϵİ�ȫ������
					0,            // ʹ��Ĭ�ϵ�ջ��С
					(LPTHREAD_START_ROUTINE)SearchThreadProc,
					(LPVOID)(spi + nSearchThreadIndex),
					CREATE_SUSPENDED,						// �ȹ���
					dwSeachThreadId + nSearchThreadIndex);	// ȡ���߳�ID
				if (hSearchThread[nSearchThreadIndex])
				{
					if (spi[nSearchThreadIndex].m_bFastMode)
						if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_TIME_CRITICAL))
							if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
								if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_ABOVE_NORMAL))
									if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
										SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_NORMAL);
					ResumeThread(hSearchThread[nSearchThreadIndex]);
					SetThreadPriorityBoost(hSearchThread[nSearchThreadIndex], !spi[nSearchThreadIndex].m_bFastMode);	// ϵͳ��̬�����߳����ȼ�ѡ��
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
			// ���������߳�
			DWORD ThreadID = 0;
			StringCchPrintf(spi[0].m_szStartLocation, MAX_PATH - 1, _T("%s"), strSearchStartPath);
			spi[0].m_bFastMode = lpDlg->GetFastMode();
			hSearchThread[0] = CreateThread(
				NULL,         // ʹ��Ĭ�ϵİ�ȫ������
				0,            // ʹ��Ĭ�ϵ�ջ��С
				(LPTHREAD_START_ROUTINE)SearchThreadProc,
				(LPVOID)&spi[0],
				CREATE_SUSPENDED,						// �ȹ���
				&ThreadID);	// ȡ���߳�ID
			if (hSearchThread)
			{
				if (spi[0].m_bFastMode)
					if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_TIME_CRITICAL))
						if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
							if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_ABOVE_NORMAL))
								if (!SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_HIGHEST))
									SetThreadPriority(hSearchThread[nSearchThreadIndex], THREAD_PRIORITY_NORMAL);
				ResumeThread(hSearchThread[nSearchThreadIndex]);
				SetThreadPriorityBoost(hSearchThread[nSearchThreadIndex], !spi[nSearchThreadIndex].m_bFastMode);	// ϵͳ��̬�����߳����ȼ�ѡ��

				WaitForSingleObject(hSearchThread[nSearchThreadIndex], INFINITE);
				CloseHandle(hSearchThread[nSearchThreadIndex]);
				nSearchThreadIndex++;
			}
		}
		if (g_vecCurrentFindData.size())
		{
			lpDlg->InitList();
			lpDlg->NotifyStatusMsg(_T("���ڸ�����������б����Ժ�..."));
			lpDlg->UpdateList(g_vecCurrentFindData);
			ULONG nViewCount = 0, nFoundCount = 0;
			for (short i = 0; i < nSearchThreadIndex; i++)
			{
				nViewCount += spi[i].m_nViewCount;
				nFoundCount += spi[i].m_nFoundCount;
			}
			StringCchPrintf(szMsg, nMSG_SIZE - 1, _T("������ɣ���%d����Ŀ�й��ҵ���%d������"), nViewCount, nFoundCount);
			lpDlg->NotifyStatusMsg(szMsg);
		}
		else
		{
			lpDlg->NotifyStatusMsg(_T("δ�ҵ���"));
		}

		// �������ˣ�����״̬
		g_vecCurrentFindData.clear();
		std::vector<FIND_DATA> vecCurrentFindDataTemp;
		g_vecCurrentFindData.swap(vecCurrentFindDataTemp);
		g_hBrokenEvent ? ResetEvent(g_hBrokenEvent) : 0;
		g_hSearchEvent ? ResetEvent(g_hSearchEvent) : 0;

		if (WaitForSingleObject(g_hQuitEvent, 50) == WAIT_OBJECT_0)
		{
			// �����������֮���������˳��ź�
			OutputDebugString(_T("Quiting..."));
			break;
		}
	}

	return 0;
}