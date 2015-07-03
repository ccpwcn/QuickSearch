
// QuickSearchDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"
#include "CommonModule.h"
#include <iostream>
#include <vector>
#include "EditEx.h"

extern HANDLE g_hSearchEvent;		// �����¼�
extern HANDLE g_hQuitEvent;			// �˳��¼�
extern HANDLE g_hBrokenEvent;		// �ж��¼�

// CQuickSearchDlg �Ի���
class CQuickSearchDlg : public CDialogEx
{
// ����
public:
	CQuickSearchDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CQuickSearchDlg();

// �Ի�������
	enum { IDD = IDD_QUICKSEARCH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

	CString & FormatFriendlySizeInfo(__in ULONG nSize, __in BOOL bBigSizeFormat = TRUE);

public:
	void NotifyStatusMsg(__in CString strMsg);
	void InitList();
	void UpdateList(std::vector<FIND_DATA> & vecDataInfo);
	void AddList(FIND_DATA & fd);
	void SaveHistory();
	void ReadHistory();
	CString & GetSearchString();
	BOOL GetFastMode();
	CString & GetSearchLocation();

// ʵ��
protected:
	HICON	m_hIcon;
	HANDLE	m_hSearchThread;		// �����߳̾��
	DWORD	m_dwSearchThreadID;		// �����߳�ID
	CString m_strCurrentSearch;		// ��ǰ������
	CString m_strSearchStartPath;	// ������ʼλ��
	LARGE_INTEGER	m_liSeconds;	// ��ʱ��
	CString m_strSettingFullname;	// �����ļ�ȫ·��

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
protected:
	// �û�����ؼ�����
	CComboBox m_CInput;
	// ������ť�ؼ�����
	CButton m_CBtnSearch;
	// ����б�ؼ�����
	CListCtrl m_CListResult;
	// �����ʷ�ؼ�����
	CButton m_CBtnClearHistory;
	// �ؽ������ؼ�����
	CButton m_CBtnResetIndex;
	// ��������ؼ�����
	CButton m_CBtnExport;
	// ״̬�ı��ؼ�����
	CEdit m_CTextStatus;
	// ����ʱ��ؼ�����
	CStatic m_CTextTimer;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtnSearch();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnClearHistory();
protected:
	// ֻ���ҵ�һ��ѡ��
	CButton m_CCBoxFindFirst;
	// ����ģʽѡ��
	CButton m_CCBoxHighSpeedMode;
	// �û��ж������Ŀؼ�����
	CButton m_CBtnBroken;
public:
	afx_msg void OnBnClickedBtnCancel();
	afx_msg void OnLvnColumnclickListResult(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	// ������ʼλ��
	CEditEx m_CTextSearchLocation;
public:
	afx_msg void OnEnChangeEditSearchLocation();
	afx_msg void OnEnSetfocusEditSearchLocation();
	afx_msg void OnEnKillfocusEditSearchLocation();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnCbnKillfocusComboInput();
	afx_msg void OnCbnCloseupComboInput();
	afx_msg void OnCbnSelchangeComboInput();
	afx_msg void OnLvnHotTrackListResult(NMHDR *pNMHDR, LRESULT *pResult);
};
