
// QuickSearchDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"
#include "CommonModule.h"
#include <iostream>
#include <vector>
#include "EditEx.h"

extern HANDLE g_hSearchEvent;		// 查找事件
extern HANDLE g_hQuitEvent;			// 退出事件
extern HANDLE g_hBrokenEvent;		// 中断事件

// CQuickSearchDlg 对话框
class CQuickSearchDlg : public CDialogEx
{
// 构造
public:
	CQuickSearchDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CQuickSearchDlg();

// 对话框数据
	enum { IDD = IDD_QUICKSEARCH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

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

// 实现
protected:
	HICON	m_hIcon;
	HANDLE	m_hSearchThread;		// 搜索线程句柄
	DWORD	m_dwSearchThreadID;		// 搜索线程ID
	CString m_strCurrentSearch;		// 当前搜索项
	CString m_strSearchStartPath;	// 搜索起始位置
	LARGE_INTEGER	m_liSeconds;	// 计时器
	CString m_strSettingFullname;	// 设置文件全路径

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
protected:
	// 用户输入控件变量
	CComboBox m_CInput;
	// 搜索按钮控件变量
	CButton m_CBtnSearch;
	// 结果列表控件变量
	CListCtrl m_CListResult;
	// 清除历史控件变量
	CButton m_CBtnClearHistory;
	// 重建索引控件变量
	CButton m_CBtnResetIndex;
	// 导出结果控件变量
	CButton m_CBtnExport;
	// 状态文本控件变量
	CEdit m_CTextStatus;
	// 消耗时间控件变量
	CStatic m_CTextTimer;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtnSearch();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnClearHistory();
protected:
	// 只查找第一个选项
	CButton m_CCBoxFindFirst;
	// 高速模式选项
	CButton m_CCBoxHighSpeedMode;
	// 用户中断搜索的控件变量
	CButton m_CBtnBroken;
public:
	afx_msg void OnBnClickedBtnCancel();
	afx_msg void OnLvnColumnclickListResult(NMHDR *pNMHDR, LRESULT *pResult);
protected:
	// 搜索起始位置
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
