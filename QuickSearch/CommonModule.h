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

// 异常处理回调
int SEH_ExceptionAccessViolationFilter(LPEXCEPTION_POINTERS p_exinfo);

// 函数功能：将char型字符串转换成WCHAR
// 参数：LPWSTR 是源字符串，LPSTR 是目标字符串
// 返回值：转换完成之后向目标缓冲区复制的字符串的个数
UINT ConvertStr_U2A(__in LPCWSTR lpSourceStr, __out LPSTR pDestStr, __in const UINT nOutBufSize);

// 函数功能：将char型字符串转换成WCHAR
// 参数：LPSTR 是源字符串，LPWSTR 是目标字符串
// 返回值：转换完成之后向目标缓冲区复制的字符串的个数
UINT ConvertStr_A2U(__in LPCSTR pSourceStr, __out LPWSTR lpDestStr, __in const UINT nOutBufSize);

// 函数功能：将通用的UTF格式的数据转换成GBK格式
// 返回值：返回转换成功的字符数，返回0表明失败
UINT ConvertStr_Utf8ToGBK(__in_opt const char * const strUtf8, __out char * strGBK, __in const UINT nOutBufSize);

// 函数功能：根据指定的错误代码，获取对应的错误描述信息
void GetErrorReason(__in const DWORD dwErrCode, __out CString & strErrMsg);

// 函数功能：构造一个临时文件名，文件名的长度不能超过MAX_PATH
BOOL BuildTempFilename(__out LPTSTR lpszTempFilename, __in const int nOutBufSize = MAX_PATH);

