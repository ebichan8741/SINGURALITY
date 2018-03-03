/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     MainFrm.cpp

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/MainFrm.cpp,v 1.3 2006/03/15 06:31:26 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "multi.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	///////// for multiApp /////////
#ifdef	PROTOCOL_USED
	LPCSTR	lpszProtocol = "protocol used";
#else	// PROTOCOL_USED
	LPCSTR	lpszProtocol = "non protocol";
#endif	// PROTOCOL_USED
	CString cAppTitle, cTitle;
	cAppTitle.LoadString(AFX_IDS_APP_TITLE);
	cTitle.Format("HIO2 %s ( %s )",
		(LPCSTR)cAppTitle, lpszProtocol);
	SetWindowText((LPCSTR)cTitle);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	///////// for multiApp /////////
	cs.style &= ~FWS_ADDTOTITLE;

	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

