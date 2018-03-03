/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multi.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multi.h,v 1.4 2007/11/26 13:56:47 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multi.h : main header file for the multi application
//
#pragma once
#pragma warning(disable: 4311)
#pragma warning(disable: 4312)
#pragma warning(disable: 4996)

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

//#define	PROTOCOL_USED

////////// for multiApp //////////

#include <win32/mytl.h>		// NNGC SDK : $(ROOT)/include/win32
#include "Hio2IfHost.h"
#include "../../include/multi.h"

// CMultiApp:
// See multi.cpp for the implementation of this class
//

class CMultiApp : public CWinApp
{
////////// for multiApp //////////
public:
	typedef struct _APPINFO
	{
		_APPINFO()
		{
			m_nHioIfID = HIO2IF_INVALID_ID;
			m_bSync = FALSE;
			ZeroMemory((LPVOID)&m_stPacket, sizeof(m_stPacket));
#ifndef	HW0
			m_pDevice = NULL;
#else	// HW0
			m_pDevice = -1;
#endif	// HW0
		}

		~_APPINFO()
		{
#ifndef	HW0
			if ( m_pDevice != NULL ) delete [] m_pDevice;
#endif	// HW0
		}

		HIO2DevicePath	m_pDevice;
		HIO2IF_ID	m_nHioIfID;
		BOOL	m_bSync;
		volatile MULTI_PACKET	m_stPacket;
	}	APPINFO, *LPAPPINFO;

	CHio2If*	m_pHioIf;
	volatile HANDLE	m_hThread;
	volatile BOOL	m_bThreadBreak;
	TMYList<LPAPPINFO>	m_cInfoList;

	BOOL	HioIfInit();
	void	CreateInfo();
	BOOL	HioIfExit();
	void	StopPollingThread();
	LPCSTR	GetStatus(HIO2IF_ID nID)
	{
		static LPCSTR l_lpszDeviceName[HIO2_CHAN_MAX + 2] =
		{ "UNKNOWN", "EXI2USB0", "EXI2USB1", "MrEXI" };

		m_cStrStatus.Format("CHAN : %s, PC%d, NNGC %s",
			(m_pHioIf->IsConnected(nID) ? "CONNECT" : "DISCONNECT"),
			m_pHioIf->GetPcChan(nID),
			l_lpszDeviceName[m_pHioIf->GetDeviceType(nID) + 1]
		);
		return (LPCSTR)m_cStrStatus;
	};
	void	Connect(HIO2IF_ID id);
	void	Disconnect(HIO2IF_ID id);
	void	CreatePcTime(MULTI_PACKET* pPacket);

private:
	CString m_cStrStatus;

public:
	CMultiApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CMultiApp theApp;

static inline
CMultiApp*	GetApp()
{
	return ((CMultiApp *)AfxGetApp());
}

// m_cInfoList.IndexOf()Ç…éwíËÇ∑ÇÈî‰ärä÷êî
BOOL	MultiCompID(CMultiApp::LPAPPINFO pItem, LPVOID pData);


int GetPos(void);