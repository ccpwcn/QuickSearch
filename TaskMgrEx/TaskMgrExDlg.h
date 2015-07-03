
// TaskMgrExDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "EasySize.h"

// CTaskMgrExDlg �Ի���
class CTaskMgrExDlg : public CDialogEx
{
	DECLARE_EASYSIZE
// ����
public:
	CTaskMgrExDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TASKMGREX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON	m_hIcon;
	HWND	m_hWnd;
	CRect	m_rect;		// �ؼ���С

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
protected:
	// ����б�
	CListCtrl m_CList;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};

