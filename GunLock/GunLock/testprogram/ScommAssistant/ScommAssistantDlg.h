// ScommAssistantDlg.h : header file
//
//{{AFX_INCLUDES()
#include "mscomm.h"
//}}AFX_INCLUDES

#if !defined(AFX_SCOMMASSISTANTDLG_H__0F614942_8991_4921_9579_296FC800A8DD__INCLUDED_)
#define AFX_SCOMMASSISTANTDLG_H__0F614942_8991_4921_9579_296FC800A8DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CScommAssistantDlg dialog

class CScommAssistantDlg : public CDialog
{
// Construction
public:
	CScommAssistantDlg(CWnd* pParent = NULL);	// standard constructor
	int m_flag;   //标识是否是打开程序初始化，m_flag=0说明是刚打开程序初始化
	int m_flag_stopdisplay;   //m_flag_stopdisplay=0时正常显示,==1时正常显示
   	float m_fWidthMul,m_fHeightMul;

	int String2Hex(CString str, CByteArray &senddata);
	char ConvertHexChar(char ch); 
	BOOL m_bAutoSend;
	BOOL change_flag;

	void ReSize(int nID);
	


// Dialog Data
	//{{AFX_DATA(CScommAssistantDlg)
	enum { IDD = IDD_SCOMMASSISTANT_DIALOG };
	CButton	m_BtnOpenLock;
	CButton	m_StopDisplay;
	CButton	m_ctrlHexDiaplay;
	CButton	m_SerialPort_CloseStart;
	CComboBox	m_Bate;
	CComboBox	m_Stop;
	CComboBox	m_Data;
	CComboBox	m_Checkout;
	CComboBox	m_SerialPort;
	CMSComm	m_ctrlComm;
	CString	m_strRXData;
	CString	m_strTXData;
	BOOL	m_ctrlHexSend;
	CString	m_Period;
	CString	m_Path;
	BYTE	m_strEdata;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScommAssistantDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CScommAssistantDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnComm();
	afx_msg void OnSelchangeComboSerialport();
	afx_msg void OnSelchangeComboBate();
	afx_msg void OnSelchangeComboCheckout();
	afx_msg void OnSelchangeComboData();
	afx_msg void OnSelchangeComboStop();
	afx_msg void OnButtonCloseSerialport();
	afx_msg void OnButtonManualsend();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnCheckAutosend();
	afx_msg void OnButtonClearRx();
	afx_msg void OnButtonStopDispal();
	afx_msg void OnButtonCloseprogramme();
	afx_msg void OnButtonAfresh();
	afx_msg void OnButtonChangpath();
	afx_msg void OnButtonSavedata();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBtnOpenLock();
	afx_msg void OnBtnEditLockAdress();
	afx_msg void OnChangeEditLockAdress();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CBrush m_brush;
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCOMMASSISTANTDLG_H__0F614942_8991_4921_9579_296FC800A8DD__INCLUDED_)
