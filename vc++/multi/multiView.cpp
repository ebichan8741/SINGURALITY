/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multiView.cpp

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multiView.cpp,v 1.4 2007/11/26 13:56:47 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multiView.cpp : implementation of the CMultiView class
//

#include "stdafx.h"
#include "multi.h"

#include "multiDoc.h"
#include "multiView.h"
#include ".\multiview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMultiView

IMPLEMENT_DYNCREATE(CMultiView, CFormView)

BEGIN_MESSAGE_MAP(CMultiView, CFormView)
    ON_MESSAGE(WM_USER, OnMyMessage)
	ON_BN_CLICKED(IDC_BTN_SEND, OnBnClickedBtnSend)
	ON_BN_CLICKED(IDC_BTN_CONNECT, OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_RADIO_SYNC, OnBnClickedRadioSync)
	ON_BN_CLICKED(IDC_RADIO_ASYNC, OnBnClickedRadioAsync)
END_MESSAGE_MAP()

// CMultiView construction/destruction

CMultiView::CMultiView()
	: CFormView(CMultiView::IDD)
{
	// TODO: add construction code here

}

CMultiView::~CMultiView()
{
}

void CMultiView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BOOL CMultiView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CMultiView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	ResizeParentToFit();

	///////// for multiApp /////////
	CheckRadioButton(IDC_RADIO_SYNC, IDC_RADIO_ASYNC, IDC_RADIO_SYNC);
}


// CMultiView diagnostics

#ifdef _DEBUG
void CMultiView::AssertValid() const
{
	CFormView::AssertValid();
}

void CMultiView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CMultiDoc* CMultiView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMultiDoc)));
	return (CMultiDoc*)m_pDocument;
}
#endif //_DEBUG

///////// for multiApp /////////

// CMultiView message handlers

LRESULT	CMultiView::OnMyMessage(WPARAM wParam, LPARAM lParam)
{
	TCHAR szBuffer[128];

	switch ( wParam )
	{
	case EVENCT_RECEIVED:
		{
			int nIndex =
				GetApp()->m_cInfoList.IndexOf( (LPVOID)( GetDocument()->m_nHioIfID ), MultiCompID );
//			MultiPacketToString(
//				szBuffer,
//				sizeof( szBuffer ),
//				(MULTI_PACKET *)&(GetApp()->m_cInfoList.GetItem(nIndex)->m_stPacket)
//			);
			MultiPacketToString(
				szBuffer,
				(MULTI_PACKET *)&(GetApp()->m_cInfoList.GetItem(nIndex)->m_stPacket)
			);
			SetDlgItemText(IDC_RECV_DATA, szBuffer);
		}
		break;
	case EVENT_CONNECT:
		GetDocument()->UpdateTitle();
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(TRUE);
		break;
	case EVENT_DISCONNECT:
		GetDocument()->UpdateTitle();
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(FALSE);
		break;
	case EVENT_UPDATE_PC_TIME:
		{
			// PC時間
			MULTI_PACKET stPacket;
			GetApp()->CreatePcTime(&stPacket);
//			MultiPacketToString( szBuffer, sizeof( szBuffer ), &stPacket );
			MultiPacketToString( szBuffer, &stPacket );
			SetDlgItemText(IDC_PC_TIME, szBuffer);
			// 通信状態
			wsprintf(szBuffer,"ST:%04X",lParam);
			SetDlgItemText(IDC_STATUS, szBuffer);
			break;
		}
	case EVENT_WRITE_DONE:
		break;
	}
	return 0;
}

void CMultiView::OnBnClickedBtnSend()
{
	HIO2IF_RESULT result;

	GetApp()->CreatePcTime(&m_stPacket);
	int nIndex =
		GetApp()->m_cInfoList.IndexOf((LPVOID)GetDocument()->m_nHioIfID, MultiCompID);
	BOOL bSync = GetApp()->m_cInfoList.GetItem(nIndex)->m_bSync;

	GetApp()->m_pHioIf->EnterCriticalSection();
#ifdef	PROTOCOL_USED
	result = GetApp()->m_pHioIf->Write(
					GetDocument()->m_nHioIfID,
					MULTI_PC2NNGC_ADDR,
					(LPVOID)&m_stPacket,
					MULTI_BUFFER_SIZE,
					bSync);

#else	// PROTOCOL_USED
	result = GetApp()->m_pHioIf->WriteFree(
					GetDocument()->m_nHioIfID,
					MULTI_PC2NNGC_ADDR,
					(LPVOID)&m_stPacket,
					MULTI_BUFFER_SIZE,
					bSync);
#endif	// PROTOCOL_USED

	GetApp()->m_pHioIf->LeaveCriticalSection();

	if ( HIO2IF_FAILED(result) )
		MessageBox("Send Error", "ERROR", MB_OK);
}

void CMultiView::OnBnClickedBtnConnect()
{
	GetApp()->Connect(GetDocument()->m_nHioIfID);
}

void CMultiView::OnBnClickedBtnDisconnect()
{
	GetApp()->Disconnect(GetDocument()->m_nHioIfID);
}

void CMultiView::OnBnClickedRadioSync()
{
	int nIndex =
		GetApp()->m_cInfoList.IndexOf((LPVOID)GetDocument()->m_nHioIfID, MultiCompID);
	GetApp()->m_cInfoList.GetItem(nIndex)->m_bSync = FALSE;
}

void CMultiView::OnBnClickedRadioAsync()
{
	int nIndex =
		GetApp()->m_cInfoList.IndexOf((LPVOID)GetDocument()->m_nHioIfID, MultiCompID);
	GetApp()->m_cInfoList.GetItem(nIndex)->m_bSync = TRUE;
}


void CMultiView::OnDraw(CDC* /*pDC*/)
{
	CMultiApp* pApp = GetApp();
	CMultiDoc* pDoc = GetDocument();
	
	// NNGCから先に起動したケース
	if ( (pDoc->m_nHioIfID != HIO2IF_INVALID_ID)
		&& (pApp->m_pHioIf->IsConnected(pDoc->m_nHioIfID)) )
	{
		pDoc->UpdateTitle();
		GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_DISCONNECT)->EnableWindow(TRUE);
	}
}
