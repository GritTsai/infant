// recorderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "recorder.h"
#include "recorderDlg.h"
#include "MonitorFunction.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecorderDlg dialog

CRecorderDlg::CRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecorderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRecorderDlg)
	m_txtdata = FALSE;
	m_bitdata = FALSE;
	m_timelen = _T("录音时间长度:");
	m_filepath = _T("./audio/");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	boMonThreadLive = TRUE;
	boRecording = FALSE;
}

void CRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRecorderDlg)
	DDX_Control(pDX, IDC_BUTTON1, m_recordBtn);
	DDX_Check(pDX, IDC_CHECK1, m_txtdata);
	DDX_Check(pDX, IDC_CHECK2, m_bitdata);
	DDX_Text(pDX, IDC_TIMELEN, m_timelen);
	DDX_Text(pDX, IDC_EDIT1, m_filepath);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRecorderDlg, CDialog)
	//{{AFX_MSG_MAP(CRecorderDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnRecorder)
	ON_BN_CLICKED(IDC_CHECK1, OnTextBtn)
	ON_BN_CLICKED(IDC_CHECK2, OnDataBtn)
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecorderDlg message handlers

BOOL CRecorderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	HANDLE hThread1 = CreateThread(NULL, 0, CMonitorFunction::SoundHandleThread, (LPVOID)this, 0, NULL);
	CloseHandle(hThread1);
	HANDLE hThread2 = CreateThread(NULL, 0, CMonitorFunction::SoundCollectThread, (LPVOID)this, 0, NULL);
	CloseHandle(hThread2);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRecorderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRecorderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRecorderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CRecorderDlg::OnRecorder() 
{
	// TODO: Add your control notification handler code here
	CString str;
	char filename[128];
	CFileException exp;
	CFileFind m_FileFind;

	UpdateData(true);

	if(boRecording)
	{
		KillTimer(this->iTimer);
		m_recordBtn.SetWindowText(_T("开始"));
		boRecording = FALSE;
		::Sleep(500);
		if(m_txtdata)
		{
			this->m_FileTXT.Close();
		}
		if(m_bitdata)
		{
			this->m_FileDATA.Close();
		}
		return;
	}

	if(!m_txtdata && !m_bitdata)
	{
		AfxMessageBox(_T("请至少选择一种录音格式!"));
		return;
	}

	if(GetFileAttributes(m_filepath) == -1)
	{
		//AfxMessageBox(_T("目标路径文件夹不存在, 开始创建!"));
		if(!CreateDirectory(m_filepath, NULL))
		{
			str = _T("路径目录不存在,且无法正常创建!");
			str += m_filepath;
			AfxMessageBox(str);
			return;
		}
	}

	//创建文件,如果失败,提示并且更新界面配置
	CTime time = CTime::GetCurrentTime();
	memset(filename, 0, sizeof(filename));

	if(m_txtdata)
	{
		_snprintf(filename, sizeof(filename), "%s\\%d%02d%02dT%02d%02d%02d.txt", this->m_filepath.GetBuffer(0), time.GetYear(), 
				time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond());
		TRACE(_T("Open create file %s\r\n"), filename);
		if(!this->m_FileTXT.Open(filename,CFile::modeCreate|CFile::modeReadWrite, &exp))
		{
			str.Format("Can't open file %s, cause %s\n", filename, exp.m_cause);
			AfxMessageBox(str);
			this->m_txtdata = FALSE;
			UpdateData(FALSE);
		}
		else
		{
			str.Format("%s", filename);
			//AfxMessageBox(str);
		}
	}

	if(m_bitdata)
	{
		_snprintf(filename, sizeof(filename), "%s\\%d%02d%02dT%02d%02d%02d.dat", this->m_filepath.GetBuffer(0), time.GetYear(), 
				time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond());
		if(!this->m_FileDATA.Open(filename, CFile::modeCreate|CFile::modeReadWrite, &exp))
		{
			str.Format("Can't open file %s, cause %s\n", filename, exp.m_cause);
			AfxMessageBox(str);
			this->m_bitdata = FALSE;
			UpdateData(FALSE);
		}
		else
		{
			str.Format("%s", filename);
			//AfxMessageBox(str);
		}
	}

	GetDlgItemText(IDC_BUTTON1,str);
	if(str == "开始")
	{
		m_recordBtn.SetWindowText(_T("停止"));
		boRecording = TRUE;
		this->iTimer = SetTimer(3, 1000, NULL);
		iSecPast = 0;
	}
	else
	{
		m_recordBtn.SetWindowText(_T("开始"));
		boRecording = FALSE;
		KillTimer(this->iTimer);
	}

	return;
}

void CRecorderDlg::OnTextBtn() 
{
	// TODO: Add your control notification handler code here
	if(boRecording)
	{
		AfxMessageBox("正在录音!");
		UpdateData(FALSE);
		return;
	}
	UpdateData(true);
}

void CRecorderDlg::OnDataBtn() 
{
	// TODO: Add your control notification handler code here
	if(boRecording)
	{
		AfxMessageBox("正在录音!");
		UpdateData(FALSE);
		return;
	}
	UpdateData(true);
}

void CRecorderDlg::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	if(boRecording)
	{
		AfxMessageBox(_T("请先停止录音, 然后再退出!"));
		return;
	}
	boMonThreadLive = FALSE;
	::Sleep(500);
	CDialog::OnClose();
}

void CRecorderDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	iSecPast++;
	m_timelen.Format("录音时间长度:%d秒", iSecPast);
	UpdateData(FALSE);
	CDialog::OnTimer(nIDEvent);
}
