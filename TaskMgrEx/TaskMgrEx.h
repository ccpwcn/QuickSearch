
// TaskMgrEx.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTaskMgrExApp: 
// �йش����ʵ�֣������ TaskMgrEx.cpp
//

class CTaskMgrExApp : public CWinApp
{
public:
	CTaskMgrExApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTaskMgrExApp theApp;