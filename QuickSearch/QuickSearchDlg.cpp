
// QuickSearchDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "QuickSearch.h"
#include "QuickSearchDlg.h"
#include "afxdialogex.h"
#include "SearchModule.h"
#include <locale>
#include <ShlObj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define USER_CONFIG_FILE	_T("setting.ini")

#define SEARCH_TIMER_ID		0x0001

HANDLE g_hSearchEvent = NULL;
HANDLE g_hQuitEvent = NULL;
HANDLE g_hBrokenEvent = NULL;

int g_nSortColumn;	// 记录点击的列
bool g_bMethod;		// 记录比较方法

#define SECOND_OF_DAY	86400

#define COL_COUNT	5
enum COL{ colNo, colFilename, colFullPath, colSize, colModifyTime };
LPCTSTR lpColumn[COL_COUNT] = { _T("序号"), _T("文件名"), _T("位置"), _T("大小"), _T("修改时间") };

#define DEFAULT_SEARCH_LOCATION_PROMPT	_T("双击此处选择搜索位置，输入'0'全盘搜索")

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CQuickSearchDlg 对话框



CQuickSearchDlg::CQuickSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CQuickSearchDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hSearchThread = NULL;
	m_liSeconds.QuadPart = 0;
	
	TCHAR szPath[MAX_PATH] = { 0 };
	SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, FALSE);
	if (_tcslen(szPath) > 0)
	{
		StringCchCat(szPath, MAX_PATH - 1, _T("\\QuickSearch"));
		CreateDirectory(szPath, NULL);
		StringCchCat(szPath, MAX_PATH - 1, _T("\\"));
		StringCchCat(szPath, MAX_PATH - 1, USER_CONFIG_FILE);
		m_strSettingFullname = szPath;
	}

}

CQuickSearchDlg::~CQuickSearchDlg()
{
	SetEvent(g_hSearchEvent);
	SetEvent(g_hQuitEvent);

	g_hSearchEvent ? CloseHandle(g_hSearchEvent) : 0;
	g_hQuitEvent ? CloseHandle(g_hQuitEvent) : 0;
	m_hSearchThread ? WaitForSingleObject(m_hSearchThread, INFINITE ) : 0;
}

void CQuickSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_INPUT, m_CInput);
	DDX_Control(pDX, IDC_BTN_SEARCH, m_CBtnSearch);
	DDX_Control(pDX, IDC_LIST_RESULT, m_CListResult);
	DDX_Control(pDX, IDC_BTN_CLEAR_HISTORY, m_CBtnClearHistory);
	DDX_Control(pDX, IDC_BTN_RESETINDEX, m_CBtnResetIndex);
	DDX_Control(pDX, IDC_BTN_EXPORT, m_CBtnExport);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_CTextStatus);
	DDX_Control(pDX, IDC_STATIC_TIMER, m_CTextTimer);
	DDX_Control(pDX, IDC_CHECK_FINDFIRST, m_CCBoxFindFirst);
	DDX_Control(pDX, IDC_CHECK_HIGH_SPEED_MODE, m_CCBoxHighSpeedMode);
	DDX_Control(pDX, IDC_BTN_CANCEL, m_CBtnBroken);
	DDX_Control(pDX, IDC_EDIT_SEARCH_LOCATION, m_CTextSearchLocation);
}

BEGIN_MESSAGE_MAP(CQuickSearchDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_SEARCH, &CQuickSearchDlg::OnBnClickedBtnSearch)
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_CLEAR_HISTORY, &CQuickSearchDlg::OnBnClickedBtnClearHistory)
	ON_BN_CLICKED(IDC_BTN_CANCEL, &CQuickSearchDlg::OnBnClickedBtnCancel)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_RESULT, &CQuickSearchDlg::OnLvnColumnclickListResult)
	ON_EN_CHANGE(IDC_EDIT_SEARCH_LOCATION, &CQuickSearchDlg::OnEnChangeEditSearchLocation)
	ON_EN_SETFOCUS(IDC_EDIT_SEARCH_LOCATION, &CQuickSearchDlg::OnEnSetfocusEditSearchLocation)
	ON_EN_KILLFOCUS(IDC_EDIT_SEARCH_LOCATION, &CQuickSearchDlg::OnEnKillfocusEditSearchLocation)
	ON_WM_CREATE()
	ON_CBN_KILLFOCUS(IDC_COMBO_INPUT, &CQuickSearchDlg::OnCbnKillfocusComboInput)
	ON_CBN_CLOSEUP(IDC_COMBO_INPUT, &CQuickSearchDlg::OnCbnCloseupComboInput)
	ON_CBN_SELCHANGE(IDC_COMBO_INPUT, &CQuickSearchDlg::OnCbnSelchangeComboInput)
	ON_NOTIFY(LVN_HOTTRACK, IDC_LIST_RESULT, &CQuickSearchDlg::OnLvnHotTrackListResult)
END_MESSAGE_MAP()



// CQuickSearchDlg 消息处理程序

BOOL CQuickSearchDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	// 所有线程都使用同样的区域配置
	_configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
	// 设置当前进程的区域配置为中文中国
	_tsetlocale(LC_ALL, /*_T("chinese_china")*/_T("Chinese_People's Republic of China"));

	g_hSearchEvent = CreateEvent(
		NULL,
		TRUE,	// 手动重置它的信号状态
		FALSE,	// 初始化时无信号状态
		NULL);
	g_hQuitEvent = CreateEvent(
		NULL,
		TRUE,	// 手动重置它的信号状态
		FALSE,	// 初始化时无信号状态
		NULL);
	g_hBrokenEvent = CreateEvent(
		NULL,
		TRUE,	// 手动重置它的信号状态
		FALSE,	// 初始化时无信号状态
		NULL);
	if (g_hSearchEvent == NULL || g_hQuitEvent == NULL || g_hBrokenEvent == NULL)
	{
		DWORD dwRet = GetLastError();
		CString strErrMsg;
		strErrMsg.Format(_T("Create event error: %d"), dwRet);
		AfxMessageBox(strErrMsg);

		g_hSearchEvent ? CloseHandle(g_hSearchEvent) : 0;
		g_hQuitEvent ? CloseHandle(g_hQuitEvent) : 0;

		exit(dwRet);
	}
	// 创建查询主线程
	m_hSearchThread = CreateThread(
		NULL,         // 使用默认的安全描述符
		0,            // 使用默认的栈大小
		(LPTHREAD_START_ROUTINE)SearchWorkThreadMgrProc,
		(LPVOID)this,
		0,						// 使用默认的创建标记
		&m_dwSearchThreadID);	// 取得线程ID

	if (m_hSearchThread == NULL)
	{
		DWORD dwRet = GetLastError();
		CString strErrMsg;
		strErrMsg.Format(_T("Create thread error: %d"), dwRet);
		AfxMessageBox(strErrMsg);

		g_hSearchEvent ? CloseHandle(g_hSearchEvent) : 0;
		g_hQuitEvent ? CloseHandle(g_hQuitEvent) : 0;

		exit(dwRet);
	}

	m_CTextSearchLocation.SetWindowText(DEFAULT_SEARCH_LOCATION_PROMPT);

	// 创建表格视图
	COLORREF color = RGB(250, 100, 100);
	m_CListResult.SetOutlineColor(color);
	DWORD dwStyle = m_CListResult.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;		//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;			//网格线（只适用与report风格的listctrl）
	// dwStyle |= LVS_EX_CHECKBOXES;			//item前生成checkbox控件
	m_CListResult.SetExtendedStyle(dwStyle);	//设置扩展风格

	// 创建列表框架
	m_CListResult.InsertColumn(colNo, lpColumn[colNo], LVCFMT_CENTER, 50);
	m_CListResult.InsertColumn(colFilename, lpColumn[colFilename], LVCFMT_LEFT, 210);
	m_CListResult.InsertColumn(colFullPath, lpColumn[colFullPath], LVCFMT_LEFT, 560);
	m_CListResult.InsertColumn(colSize, lpColumn[colSize], LVCFMT_RIGHT, 80);
	m_CListResult.InsertColumn(colModifyTime, lpColumn[colModifyTime], LVCFMT_CENTER, 150);

	ReadHistory();
	m_CInput.SetFocus();
	SetTimer(SEARCH_TIMER_ID, 1000, NULL);

	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CQuickSearchDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CQuickSearchDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		const int cTitleTextFontSize = 260;
		CFont cfTitleFont;

		cfTitleFont.CreatePointFont(cTitleTextFontSize, _T("Arial Black"));
		if (GetDlgItem(IDC_STATIC_TITLE))
			GetDlgItem(IDC_STATIC_TITLE)->SetFont(&cfTitleFont);	//设置字体
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CQuickSearchDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CQuickSearchDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (AfxMessageBox(_T("确定要退出程序么？"), MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK)
	{
		SaveHistory();
		SetEvent(g_hBrokenEvent);
		SetEvent(g_hSearchEvent);
		SetEvent(g_hQuitEvent);
		KillTimer(SEARCH_TIMER_ID);
		CDialogEx::OnClose();
	}
}


BOOL CQuickSearchDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:			// Esc按键事件  
			return TRUE;
		case VK_RETURN:			// Enter按键事件  
			return TRUE;
		default:
			break;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CQuickSearchDlg::NotifyStatusMsg(__in CString strMsg)
{
	m_CTextStatus.SetWindowText(strMsg);
}

void CQuickSearchDlg::OnBnClickedBtnSearch()
{
	// TODO:  在此添加控件通知处理程序代码
	if (WaitForSingleObject(g_hSearchEvent, 10) == WAIT_OBJECT_0)
	{
		AfxMessageBox(_T("前一个搜索任务还没有完成，无法启动新的搜索任务！"));
		return;
	}
	m_CInput.GetWindowText(m_strCurrentSearch);
	if (m_strCurrentSearch.GetLength() <= 0)
	{
		AfxMessageBox(_T("没有输入任何内容，无法启动搜索！"));
		return;
	}
	TCHAR szUserSearchLocation[BUFSIZ] = { 0 };
	m_CTextSearchLocation.GetWindowText(szUserSearchLocation, BUFSIZ - 1);
	if (_tcslen(szUserSearchLocation) <= 0 || (_tcsicmp(szUserSearchLocation, _T("0")) && !PathFileExists(szUserSearchLocation)))
	{
		AfxMessageBox(_T("您的输入无效，请检查输入是否正确。\n提示：您指定的搜索目录位置必须存在并且有效！"));
		return;
	}

	SetEvent(g_hSearchEvent);
	
	// 更新搜索历史，如果搜索关键词已经存在，删除了重新添加，使它保持在最上面
	CString strCBText;
	for (int i = 0; i < m_CInput.GetCount(); i++)
	{
		m_CInput.GetLBText(i, strCBText);
		if (strCBText.GetLength() > 0 && strCBText == m_strCurrentSearch)
		{
			m_CInput.DeleteString(i);
			break;
		}
	}
	m_CInput.InsertString(0, m_strCurrentSearch);
	m_CInput.SetCurSel(0);
	
	m_liSeconds.QuadPart++;
}


void CQuickSearchDlg::InitList()
{
	m_CListResult.DeleteAllItems();
}

CString & CQuickSearchDlg::FormatFriendlySizeInfo(__in ULONG nSize, __in BOOL bBigSizeFormat /*= TRUE*/)
{
	static CString strOut;
	const UINT nUint = bBigSizeFormat ? 1024 : 1000;
	if (nSize >= 0 && nSize < nUint)	// < 1KB
		strOut.Format(_T("%.2lf%s"), (double)nSize, bBigSizeFormat ? _T("b") : _T("B"));
	else if (nSize >= nUint && nSize < nUint * nUint)  // < 1MB
		strOut.Format(_T("%.2lf%s"), (double)nSize / nUint, bBigSizeFormat ? _T("KiB") : _T("KB"));
	else if (nSize >= nUint * nUint && nSize < nUint * nUint * nUint)  // < 1GB
		strOut.Format(_T("%.2lf%s"), (double)nSize / nUint / nUint, bBigSizeFormat ? _T("MiB") : _T("MB"));
	else
		strOut.Format(_T("%.2lf%s"), (double)nSize / nUint / nUint / nUint, bBigSizeFormat ? _T("GiB") : _T("GB"));

	return strOut;
}
void CQuickSearchDlg::UpdateList(std::vector<FIND_DATA> & vecDataInfo)
{
	if (vecDataInfo.size() <= 0)		return;

	for (std::vector<FIND_DATA>::iterator it = vecDataInfo.begin(); it != vecDataInfo.end(); it++)
	{
		int nRow = 0;
		int nLineIndex = m_CListResult.GetItemCount();	     //行数

		LV_ITEM lvItem;
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = nLineIndex;
		lvItem.iSubItem = 0;
		TCHAR szTemp[MAX_PATH];
		StringCchPrintf(szTemp, MAX_PATH - 1, _T("%d"), it - vecDataInfo.begin() + 1);
		lvItem.pszText = szTemp;   //第一列
		//在最后一行插入记录值，序号已经顺便插入了
		m_CListResult.InsertItem(&lvItem);
		nRow++;

		// 插入文件名
		StringCchPrintf(szTemp, MAX_PATH - 1, _T("%s"), it->m_szFilename);
		m_CListResult.SetItemText(nLineIndex, nRow, szTemp);
		nRow++;

		// 插入文件位置
		StringCchPrintf(szTemp, MAX_PATH - 1, _T("%s"), it->m_szLocation);
		m_CListResult.SetItemText(nLineIndex, nRow, szTemp);
		nRow++;

		// 插入文件大小
		m_CListResult.SetItemText(nLineIndex, nRow, FormatFriendlySizeInfo(it->m_dwFileSize));
		nRow++;

		//插入文件日期信息
		StringCchPrintf(szTemp, 
			MAX_PATH - 1, 
			_T("%04d-%02d-%02d %02d:%02d:%02d"), 
			it->m_stFileDateAndTime.wYear,
			it->m_stFileDateAndTime.wMonth,
			it->m_stFileDateAndTime.wDay,
			it->m_stFileDateAndTime.wHour,
			it->m_stFileDateAndTime.wMinute,
			it->m_stFileDateAndTime.wSecond);
		m_CListResult.SetItemText(nLineIndex, nRow, szTemp);
		nRow++;
	}
	m_CListResult.SetFocus();
}

void CQuickSearchDlg::AddList(FIND_DATA & fd)
{
	int nRow = 0;
	int nLineCount = m_CListResult.GetItemCount();	     //行数

	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nLineCount;
	lvItem.iSubItem = 0;
	TCHAR szTemp[MAX_PATH];
	StringCchPrintf(szTemp, MAX_PATH - 1, _T("%d"), nLineCount);
	lvItem.pszText = szTemp;   //第一列
	//在最后一行插入记录值，序号已经顺便插入了
	m_CListResult.InsertItem(&lvItem);
	nRow++;

	// 插入文件名
	StringCchPrintf(szTemp, MAX_PATH - 1, _T("%s"), fd.m_szFilename);
	m_CListResult.SetItemText(nLineCount, nRow, szTemp);
	nRow++;

	// 插入文件位置
	StringCchPrintf(szTemp, MAX_PATH - 1, _T("%s"), fd.m_szLocation);
	m_CListResult.SetItemText(nLineCount, nRow, szTemp);
	nRow++;

	// 插入文件大小
	m_CListResult.SetItemText(nLineCount, nRow, FormatFriendlySizeInfo(fd.m_dwFileSize));
	nRow++;

	//插入文件日期信息
	StringCchPrintf(szTemp,
		MAX_PATH - 1,
		_T("%04d-%02d-%02d %02d:%02d:%02d"),
		fd.m_stFileDateAndTime.wYear,
		fd.m_stFileDateAndTime.wMonth,
		fd.m_stFileDateAndTime.wDay,
		fd.m_stFileDateAndTime.wHour,
		fd.m_stFileDateAndTime.wMinute,
		fd.m_stFileDateAndTime.wSecond);
	m_CListResult.SetItemText(nLineCount, nRow, szTemp);
	nRow++;

	m_CListResult.EnsureVisible(m_CListResult.GetItemCount() - 1, FALSE);
	
	m_CListResult.SetItemState(
		m_CListResult.GetItemCount() - 1,
		LVIS_ACTIVATING | LVIS_FOCUSED | LVIS_SELECTED,
		LVIS_SELECTED | LVIS_FOCUSED);
}

HBRUSH CQuickSearchDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_TITLE)//在这里判断你的控件ID，做相应的处理
		pDC->SetTextColor(RGB(255, 60, 60));
	else if (pWnd->GetDlgCtrlID() == IDC_EDIT_SEARCH_LOCATION)//在这里判断你的控件ID，做相应的处理
	{
		CString strText;
		m_CTextSearchLocation.GetWindowText(strText);
		if (strText == DEFAULT_SEARCH_LOCATION_PROMPT)
			pDC->SetTextColor(RGB(190, 190, 190));
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_TIMER)//在这里判断你的控件ID，做相应的处理
		pDC->SetTextColor(RGB(29, 94, 23));
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}


void CQuickSearchDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CString strMsg;

	switch (nIDEvent)
	{
		case SEARCH_TIMER_ID:
		{
			if (WaitForSingleObject(g_hSearchEvent, 1) == WAIT_OBJECT_0)
			{
				if (m_liSeconds.QuadPart)
				{
					strMsg.Format(_T("%02I64d:%02I64d:%02I64d"), m_liSeconds.QuadPart / 3600, m_liSeconds.QuadPart / 60, m_liSeconds.QuadPart % 60);
					m_CTextTimer.SetWindowText(strMsg);
				}
				m_liSeconds.QuadPart++;
			}
			else
			{
				// 如果任务已经完成，计时器归零
				m_liSeconds.QuadPart = 0;
			}
		}
		default:
			break;
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CQuickSearchDlg::OnBnClickedBtnClearHistory()
{
	// TODO:  在此添加控件通知处理程序代码
	if (AfxMessageBox(_T("历史记录清除无法恢复，确定要这么做吗？"), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	m_CTextTimer.SetWindowText(_T(""));
	TCHAR buffer[65] = { 0 };
	for (int i = 0; i < m_CInput.GetCount(); i++)
	{
		_itot_s(i, buffer, 64, 10);
		WritePrivateProfileString(_T("input"), buffer, NULL, m_strSettingFullname);
	}
	m_CInput.ResetContent();
	InitList();
	m_CTextStatus.SetWindowText(_T("清除历史记录完成！"));
}


CString & CQuickSearchDlg::GetSearchString()
{
	return m_strCurrentSearch;
}

void CQuickSearchDlg::ReadHistory()
{
	TCHAR szAllAppName[8192] = { 0 };		//所有AppName
	DWORD dwKeyNameSize;					//对应每个AppName的所有KeyName的总长度
	DWORD dwAppNameSize = GetPrivateProfileString(NULL, NULL, NULL, szAllAppName, 8191, m_strSettingFullname);
	for (TCHAR * lpszCurrentAppName = szAllAppName; *lpszCurrentAppName; lpszCurrentAppName += _tcslen(lpszCurrentAppName) + 1)
	{
		TCHAR szAllKeyName[8192] = { 0 };		//所有AppName
		dwKeyNameSize = GetPrivateProfileString(lpszCurrentAppName, NULL, NULL, szAllKeyName, 8191, m_strSettingFullname);

		if (_tcsicmp(lpszCurrentAppName, _T("input")) == 0)
		{
			TCHAR szCurrentKeyValue[MAX_PATH] = { 0 };
			for (TCHAR * lpszCurrentKeyName = szAllKeyName; *lpszCurrentKeyName; lpszCurrentKeyName += _tcslen(lpszCurrentKeyName) + 1)
			{
				GetPrivateProfileString(lpszCurrentAppName, lpszCurrentKeyName, NULL, szCurrentKeyValue, MAX_PATH - 1, m_strSettingFullname);
				m_CInput.AddString(szCurrentKeyValue);
			}
		}
		else if (_tcsicmp(lpszCurrentAppName, _T("checkbox")) == 0)
		{
			TCHAR szCurrentKeyValue[MAX_PATH] = { 0 };
			for (TCHAR * lpszCurrentKeyName = szAllKeyName; *lpszCurrentKeyName; lpszCurrentKeyName += _tcslen(lpszCurrentKeyName) + 1)
			{
				GetPrivateProfileString(lpszCurrentAppName, lpszCurrentKeyName, NULL, szCurrentKeyValue, MAX_PATH - 1, m_strSettingFullname);
				int state = _tstoi(szCurrentKeyValue) == 0 ? BST_UNCHECKED : BST_CHECKED;
				if (_tcsicmp(lpszCurrentKeyName, _T("OnlyFindFirst")) == 0)
				{
					m_CCBoxFindFirst.SetCheck(state);
				}
				else if (_tcsicmp(lpszCurrentKeyName, _T("HighSpeedMode")) == 0)
				{
					m_CCBoxHighSpeedMode.SetCheck(state);
				}

			}
		}
	}
}

void CQuickSearchDlg::SaveHistory()
{
	CString strKeyName, strCBText;
	for (int i = 0; i < m_CInput.GetCount(); i++)
	{
		m_CInput.GetLBText(i, strCBText);
		if (strCBText.GetLength() > 0)
		{
			strKeyName.Format(_T("%d"), i);
			WritePrivateProfileString(_T("input"), strKeyName, strCBText, m_strSettingFullname);
		}
	}

	CString strCheckBox;
	CButton * pCheckBoxFindFirst = (CButton *)GetDlgItem(IDC_CHECK_FINDFIRST);
	int state = pCheckBoxFindFirst->GetCheck();
	strCheckBox.Format(_T("%d"), state);
	WritePrivateProfileString(_T("checkbox"), _T("OnlyFindFirst"), strCheckBox, m_strSettingFullname);

	CButton * pCheckBoxHighSpeedMode = (CButton *)GetDlgItem(IDC_CHECK_HIGH_SPEED_MODE);
	state = pCheckBoxHighSpeedMode->GetCheck();
	strCheckBox.Format(_T("%d"), state);
	WritePrivateProfileString(_T("checkbox"), _T("HighSpeedMode"), strCheckBox, m_strSettingFullname);
}

void CQuickSearchDlg::OnBnClickedBtnCancel()
{
	// TODO:  在此添加控件通知处理程序代码
	if (WaitForSingleObject(g_hSearchEvent, 10) == WAIT_OBJECT_0)
	{
		NotifyStatusMsg(_T("正在停止..."));
		SetEvent(g_hBrokenEvent); 
	}
	else
	{
		AfxMessageBox(_T("没有检测到正在运行的搜索任务，没有可以进行的停止操作。"));
	}
}

BOOL CQuickSearchDlg::GetFastMode()
{
	CButton * pCheckBoxHighSpeedMode = (CButton *)GetDlgItem(IDC_CHECK_HIGH_SPEED_MODE);
	int state = pCheckBoxHighSpeedMode->GetCheck();
	return state;
}

// 排序比较函数
static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// 从参数中提取所需比较lc的两行数据

	// int row1 = (int)lParam1;
	// int row2 = (int)lParam2;

	CListCtrl * lc = (CListCtrl *)lParamSort;

	// CString lp1 = lc->GetItemText(row1, g_nSortColumn);
	// CString lp2 = lc->GetItemText(row2, g_nSortColumn);
	static TCHAR szStr1[8192] = { 0 }, szStr2[8192] = { 0 };
	StringCchCopy(szStr1, 8191, reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam1, g_nSortColumn));
	StringCchCopy(szStr2, 8191, reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam2, g_nSortColumn));

	// 比较，对不同的列，不同比较，注意记录前一次排序方向，下一次要相反排序

	if (g_nSortColumn == 0)
	{
		// int型比较
		if (g_bMethod)
			return _tstoi(szStr1) - _tstoi(szStr2);
		else
			return _tstoi(szStr2) - _tstoi(szStr1);
	}
	else
	{
		// 文字型比较
		/*if (g_bMethod)
			return lp1.CompareNoCase(lp2);
		else
			return lp2.CompareNoCase(lp1);
			*/
		if (g_bMethod)
			return _tcsicmp(szStr1, szStr2);
		else
			return _tcsicmp(szStr2, szStr1);
			
		/*if (g_bMethod)
			return _tcsicmp(
			reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam1, g_nSortColumn),
			reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam2, g_nSortColumn));
		else
			return _tcsicmp(
			reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam2, g_nSortColumn),
			reinterpret_cast<CListCtrl *>(lParamSort)->GetItemText((int)lParam1, g_nSortColumn));
			*/
	}

	return 0;
}

void CQuickSearchDlg::OnLvnColumnclickListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	g_nSortColumn = pNMLV->iSubItem;//点击的列

	int count = m_CListResult.GetItemCount();
	static short nCallTimes = 0;
	if (nCallTimes == 0)
	{
		if (count >= 5000 && count < 10000)		// 5K到10K
		{
			if (AfxMessageBox(_T("排序可能需要花费几秒钟时间，您确定要排序吗？"), MB_YESNO) != IDYES)
				return;
		}
		else if (count >= 10000 && count < 100000)	// 10K到100K
		{
			if (AfxMessageBox(_T("排序可能需要花费一点点时间，您确定要排序吗？"), MB_YESNO) != IDYES)
				return;
		}
		else if (count >= 100000)	// 100K以上
		{
			if (AfxMessageBox(_T("当前数据量比较大，排序可能需要花费较长时间，您确定要排序吗？"), MB_YESNO) != IDYES)
				return;
		}
	}
	nCallTimes = 1;

	CString strMsg;
	for (int i = 0; i < count; i++)
	{
		strMsg.Format(_T("排序准备[%d/%d]..."), i, count);
		NotifyStatusMsg(strMsg);
		m_CListResult.SetItemData(i, i); // 每行的比较关键字，此处为列序号（点击的列号），可以设置为其他 比较函数的第一二个参数
	}
	NotifyStatusMsg(_T("正在排序..."));
	m_CListResult.SortItems(MyCompareProc, (DWORD_PTR)&m_CListResult);//排序 第二个参数是比较函数的第三个参数
	g_bMethod = !g_bMethod;
	NotifyStatusMsg(_T("排序完成！"));
	*pResult = 0;
}


void CQuickSearchDlg::OnEnChangeEditSearchLocation()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	
	// TODO:  在此添加控件通知处理程序代码
}


void CQuickSearchDlg::OnEnSetfocusEditSearchLocation()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strSearchLocation;
	m_CTextSearchLocation.GetWindowText(strSearchLocation);
	if (strSearchLocation == DEFAULT_SEARCH_LOCATION_PROMPT)
		m_CTextSearchLocation.SetWindowText(_T(""));
}


void CQuickSearchDlg::OnEnKillfocusEditSearchLocation()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strSearchLocation;
	m_CTextSearchLocation.GetWindowText(strSearchLocation);
	if (strSearchLocation == _T(""))
		m_CTextSearchLocation.SetWindowText(DEFAULT_SEARCH_LOCATION_PROMPT);
}


int CQuickSearchDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	return 0;
}

CString & CQuickSearchDlg::GetSearchLocation()
{
	m_CTextSearchLocation.GetWindowText(m_strSearchStartPath);
	if (m_strSearchStartPath.GetLength() <= 0)
	{
		return m_strSearchStartPath;
	}
	else if ( m_strSearchStartPath[m_strSearchStartPath.GetLength() - 1] == _T('\\'))
	{
		m_strSearchStartPath.Delete(m_strSearchStartPath.GetLength() - 1);
		return m_strSearchStartPath;
	}
	else
	{
		return m_strSearchStartPath;
	}
}

void CQuickSearchDlg::OnCbnKillfocusComboInput()
{
	// TODO:  在此添加控件通知处理程序代码
	m_CInput.GetWindowText(m_strCurrentSearch);
}


void CQuickSearchDlg::OnCbnCloseupComboInput()
{
	// TODO:  在此添加控件通知处理程序代码
	m_CInput.GetWindowText(m_strCurrentSearch);
}


void CQuickSearchDlg::OnCbnSelchangeComboInput()
{
	// TODO:  在此添加控件通知处理程序代码
	int nIndex = m_CInput.GetCurSel();
	if (nIndex != LB_ERR)
		m_CInput.GetWindowText(m_strCurrentSearch);
}


void CQuickSearchDlg::OnLvnHotTrackListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	m_CListResult.SetFocus();
	*pResult = 0;
}
