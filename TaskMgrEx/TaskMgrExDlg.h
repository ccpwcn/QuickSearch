
// TaskMgrExDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "EasySize.h"

// CTaskMgrExDlg 对话框
class CTaskMgrExDlg : public CDialogEx
{
	DECLARE_EASYSIZE
// 构造
public:
	CTaskMgrExDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TASKMGREX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON	m_hIcon;
	HWND	m_hWnd;
	CRect	m_rect;		// 控件大小

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
protected:
	// 输出列表
	CListCtrl m_CList;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};

