// ScommAssistant.h : main header file for the SCOMMASSISTANT application
//

#if !defined(AFX_SCOMMASSISTANT_H__B03A2120_4969_496A_9185_FB91E3BEF009__INCLUDED_)
#define AFX_SCOMMASSISTANT_H__B03A2120_4969_496A_9185_FB91E3BEF009__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CScommAssistantApp:
// See ScommAssistant.cpp for the implementation of this class
//

class CScommAssistantApp : public CWinApp
{
public:
	CScommAssistantApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScommAssistantApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CScommAssistantApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCOMMASSISTANT_H__B03A2120_4969_496A_9185_FB91E3BEF009__INCLUDED_)
