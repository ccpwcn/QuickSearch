#include "stdafx.h"
#include "CommonModule.h"

// 异常处理回调
int SEH_ExceptionAccessViolationFilter(LPEXCEPTION_POINTERS p_exinfo)
{
	TCHAR szErrMsg[BUFSIZ];
	switch (p_exinfo->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("存储保护异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("数据类型未对齐异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_BREAKPOINT:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("中断异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_SINGLE_STEP:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("单步中断异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("数组越界异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_STACK_CHECK:
	case EXCEPTION_FLT_UNDERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("浮点数计算异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("被0除异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INT_OVERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("数据溢出异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("页错误异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("非法指令异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_STACK_OVERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("堆栈溢出异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INVALID_HANDLE:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("无效句病异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	default:
		if (p_exinfo->ExceptionRecord->ExceptionCode & (1 << 29))
			_stprintf_s(szErrMsg, BUFSIZ - 1, _T("用户自定义的软件异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		else
			_stprintf_s(szErrMsg, BUFSIZ - 1, _T("其它异常，错误代码：%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	}

	OutputDebugString(szErrMsg);
	SetLastError(p_exinfo->ExceptionRecord->ExceptionCode);
	return 1;
}


// 函数功能：将char型字符串转换成WCHAR
// 参数：LPWSTR 是源字符串，LPSTR 是目标字符串
// 返回值：转换完成之后向目标缓冲区复制的字符串的个数
UINT ConvertStr_U2A(__in LPCWSTR lpSourceStr, __out LPSTR pDestStr, __in const UINT nOutBufSize)
{
	if (lpSourceStr == NULL || pDestStr == NULL || nOutBufSize <= 0)		return 0;

	WideCharToMultiByte(
		CP_ACP,
		NULL,
		lpSourceStr,
		-1,
		pDestStr,
		nOutBufSize - 1,
		NULL,
		NULL);

	return strlen(pDestStr);
}


// 函数功能：将char型字符串转换成WCHAR
// 参数：LPSTR 是源字符串，LPWSTR 是目标字符串
// 返回值：转换完成之后向目标缓冲区复制的字符串的个数
UINT ConvertStr_A2U(__in LPCSTR pSourceStr, __out LPWSTR lpDestStr, __in const UINT nOutBufSize)
{
	if (pSourceStr == NULL || lpDestStr == NULL || nOutBufSize <= 0)		return 0;

	MultiByteToWideChar(
		CP_ACP,
		NULL,
		pSourceStr,
		-1,
		lpDestStr,
		nOutBufSize - 1);

	return wcslen(lpDestStr);
}

// 函数功能：将通用的UTF格式的数据转换成GBK格式
// 返回值：返回转换成功的字符数，返回0表明失败
UINT ConvertStr_Utf8ToGBK(__in_opt const char * const strUtf8, __out char * strGBK, __in const UINT nOutBufSize)
{
	if (strUtf8 == NULL || strGBK == NULL || nOutBufSize <= 0)	return 0;
	// 先从UTF-8转换为Unicode
	int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL, 0);
	WCHAR * wszGBK = new WCHAR[len + 1];
	memset(wszGBK, 0, len * sizeof(WCHAR)+2);
	MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, wszGBK, len);

	// 再从Unicode转换为系统语言
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, strGBK, nOutBufSize - 1, NULL, NULL);

	delete[] wszGBK;
	return len;
}

// 函数功能：根据指定的错误代码，获取对应的错误描述信息
void GetErrorReason(__in const DWORD dwErrCode, __out CString & strErrMsg)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf = NULL;
	LPVOID lpDisplayBuf = NULL;;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(_tcslen((LPCTSTR)lpMsgBuf) + 40)*sizeof(TCHAR));
	if (lpDisplayBuf)
	{
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf),
			_T("%s"),
			lpMsgBuf);
		strErrMsg.Format(_T("%s"), lpDisplayBuf);
	}

	lpMsgBuf ? LocalFree(lpMsgBuf) : 0;
	lpDisplayBuf ? LocalFree(lpDisplayBuf) : 0;
}

// 函数功能：构造一个临时文件名，文件名的长度不能超过MAX_PATH
BOOL BuildTempFilename( __out LPTSTR lpszTempFilename, __in const int nOutBufSize )
{
	if (lpszTempFilename == NULL)		return FALSE;

	BOOL bRet = FALSE;
	DWORD dwRet = 0;

	// Get the temp path.
	TCHAR szTempPath[MAX_PATH] = { 0 };
	DWORD dwRetVal = GetTempPath(
		MAX_PATH - 1,		// length of the buffer
		szTempPath);		// buffer for path 
	dwRet = GetLastError();
	if (dwRetVal > MAX_PATH || (dwRetVal == 0))
	{
		return FALSE;
	}

	UINT uRetVal = GetTempFileName(
		szTempPath,			// directory for tmp files
		_T("lg"),			// temp file name prefix 
		0,					// create unique name 
		lpszTempFilename);		// buffer for name 
	dwRet = GetLastError();
	if (uRetVal == 0)
	{
		return FALSE;
	}

	return TRUE;
}
