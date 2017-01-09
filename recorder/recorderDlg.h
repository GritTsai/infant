// recorderDlg.h : header file
//

#if !defined(AFX_RECORDERDLG_H__E6B0FC83_20A5_46BF_9627_60322218D9CB__INCLUDED_)
#define AFX_RECORDERDLG_H__E6B0FC83_20A5_46BF_9627_60322218D9CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRecorderDlg dialog

class CRecorderDlg : public CDialog
{
// Construction
public:
	CRecorderDlg(CWnd* pParent = NULL);	// standard constructor
	
	BOOL boMonThreadLive;
	BOOL boRecording;

	int iSecPast;
	int iTimer;

	CFile m_FileTXT;
	CFile m_FileDATA;
// Dialog Data
	//{{AFX_DATA(CRecorderDlg)
	enum { IDD = IDD_RECORDER_DIALOG };
	CButton	m_recordBtn;
	BOOL	m_txtdata;
	BOOL	m_bitdata;
	CString	m_timelen;
	CString	m_filepath;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRecorderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CRecorderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRecorder();
	afx_msg void OnTextBtn();
	afx_msg void OnDataBtn();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RECORDERDLG_H__E6B0FC83_20A5_46BF_9627_60322218D9CB__INCLUDED_)
