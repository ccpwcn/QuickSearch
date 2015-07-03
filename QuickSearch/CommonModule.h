#pragma once

#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

typedef struct _tagFindData
{
	TCHAR m_szFilename[MAX_PATH];
	TCHAR m_szLocation[MAX_PATH];
	DWORD m_dwFileSize;
	SYSTEMTIME	m_stFileDateAndTime;
}FIND_DATA, *LPFIND_DATA;

// �쳣����ص�
int SEH_ExceptionAccessViolationFilter(LPEXCEPTION_POINTERS p_exinfo);

// �������ܣ���char���ַ���ת����WCHAR
// ������LPWSTR ��Դ�ַ�����LPSTR ��Ŀ���ַ���
// ����ֵ��ת�����֮����Ŀ�껺�������Ƶ��ַ����ĸ���
UINT ConvertStr_U2A(__in LPCWSTR lpSourceStr, __out LPSTR pDestStr, __in const UINT nOutBufSize);

// �������ܣ���char���ַ���ת����WCHAR
// ������LPSTR ��Դ�ַ�����LPWSTR ��Ŀ���ַ���
// ����ֵ��ת�����֮����Ŀ�껺�������Ƶ��ַ����ĸ���
UINT ConvertStr_A2U(__in LPCSTR pSourceStr, __out LPWSTR lpDestStr, __in const UINT nOutBufSize);

// �������ܣ���ͨ�õ�UTF��ʽ������ת����GBK��ʽ
// ����ֵ������ת���ɹ����ַ���������0����ʧ��
UINT ConvertStr_Utf8ToGBK(__in_opt const char * const strUtf8, __out char * strGBK, __in const UINT nOutBufSize);

// �������ܣ�����ָ���Ĵ�����룬��ȡ��Ӧ�Ĵ���������Ϣ
void GetErrorReason(__in const DWORD dwErrCode, __out CString & strErrMsg);

// �������ܣ�����һ����ʱ�ļ������ļ����ĳ��Ȳ��ܳ���MAX_PATH
BOOL BuildTempFilename(__out LPTSTR lpszTempFilename, __in const int nOutBufSize = MAX_PATH);

