#include "stdafx.h"
#include "CommonModule.h"

// �쳣����ص�
int SEH_ExceptionAccessViolationFilter(LPEXCEPTION_POINTERS p_exinfo)
{
	TCHAR szErrMsg[BUFSIZ];
	switch (p_exinfo->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�洢�����쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("��������δ�����쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_BREAKPOINT:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�ж��쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_SINGLE_STEP:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�����ж��쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("����Խ���쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_STACK_CHECK:
	case EXCEPTION_FLT_UNDERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�����������쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("��0���쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INT_OVERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("��������쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("ҳ�����쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�Ƿ�ָ���쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_STACK_OVERFLOW:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("��ջ����쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	case EXCEPTION_INVALID_HANDLE:
		_stprintf_s(szErrMsg, BUFSIZ - 1, _T("��Ч�䲡�쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	default:
		if (p_exinfo->ExceptionRecord->ExceptionCode & (1 << 29))
			_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�û��Զ��������쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		else
			_stprintf_s(szErrMsg, BUFSIZ - 1, _T("�����쳣��������룺%x\n"), p_exinfo->ExceptionRecord->ExceptionCode);
		break;
	}

	OutputDebugString(szErrMsg);
	SetLastError(p_exinfo->ExceptionRecord->ExceptionCode);
	return 1;
}


// �������ܣ���char���ַ���ת����WCHAR
// ������LPWSTR ��Դ�ַ�����LPSTR ��Ŀ���ַ���
// ����ֵ��ת�����֮����Ŀ�껺�������Ƶ��ַ����ĸ���
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


// �������ܣ���char���ַ���ת����WCHAR
// ������LPSTR ��Դ�ַ�����LPWSTR ��Ŀ���ַ���
// ����ֵ��ת�����֮����Ŀ�껺�������Ƶ��ַ����ĸ���
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

// �������ܣ���ͨ�õ�UTF��ʽ������ת����GBK��ʽ
// ����ֵ������ת���ɹ����ַ���������0����ʧ��
UINT ConvertStr_Utf8ToGBK(__in_opt const char * const strUtf8, __out char * strGBK, __in const UINT nOutBufSize)
{
	if (strUtf8 == NULL || strGBK == NULL || nOutBufSize <= 0)	return 0;
	// �ȴ�UTF-8ת��ΪUnicode
	int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL, 0);
	WCHAR * wszGBK = new WCHAR[len + 1];
	memset(wszGBK, 0, len * sizeof(WCHAR)+2);
	MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, wszGBK, len);

	// �ٴ�Unicodeת��Ϊϵͳ����
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, strGBK, nOutBufSize - 1, NULL, NULL);

	delete[] wszGBK;
	return len;
}

// �������ܣ�����ָ���Ĵ�����룬��ȡ��Ӧ�Ĵ���������Ϣ
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

// �������ܣ�����һ����ʱ�ļ������ļ����ĳ��Ȳ��ܳ���MAX_PATH
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
