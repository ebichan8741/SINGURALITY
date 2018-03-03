/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multi.cpp

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multi.cpp,v 1.4 2007/11/26 13:56:47 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multi.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "multi.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "multiDoc.h"
#include "multiView.h"
#include ".\multi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////
//前フレームのデータ
//////////////////////////////////////////////
MULTI_PACKET packetDB[2] = {0};


///////// for multiApp /////////

// m_cInfoList.IndexOf()に指定する比較関数
BOOL	MultiCompID(CMultiApp::LPAPPINFO pItem, LPVOID pData)
{
	return (pItem->m_nHioIfID == (HIO2IF_ID)pData) ? TRUE : FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// local function and thread function definition
//

// Get active view
static
CMultiView*	GetActiveView(HIO2IF_ID nID)
{
	POSITION pos;
	CDocTemplate* pDocTemplate;
	CMultiDoc* pDoc;

	// Document Template
	pos = GetApp()->GetFirstDocTemplatePosition();
	if ( pos == NULL ) return NULL;
	pDocTemplate = GetApp()->GetNextDocTemplate(pos);

	// Document
	pos = pDocTemplate->GetFirstDocPosition();
	if ( pos == NULL ) return NULL;
	while ( pos != NULL )
	{
		pDoc = static_cast<CMultiDoc *>(pDocTemplate->GetNextDoc(pos));
		if ( pDoc->m_nHioIfID == nID )
		{
			POSITION viewpos = pDoc->GetFirstViewPosition();
			if ( viewpos != NULL )
				return static_cast<CMultiView *>(pDoc->GetNextView(viewpos));
		}
	}
	return NULL;
}

// CHioIf::HIO2IF_EVENT_CALLBACK
static
void	HioIfEventCallback(HIO2IF_ID id, HIO2IF_EVENT event)
{
	CMultiView* pView;
	CMultiApp* pApp = GetApp();
	int nIndex =
        GetApp()->m_cInfoList.IndexOf((LPVOID)id, MultiCompID);
	CMultiApp::LPAPPINFO pInfo = pApp->m_cInfoList.GetItem(nIndex);

	switch ( event )
	{
	case HIO2IF_EVENT_CONNECT:		// 接続確立
		{
			BYTE clearBuf[MULTI_BUFFER_SIZE * 2];

			// Viewへメッセージ
			if ( (pView=GetActiveView(id)) != NULL )
				pView->PostMessage(WM_USER, CMultiView::EVENT_CONNECT, 0);

			// 領域をクリア
			ZeroMemory(clearBuf, sizeof(clearBuf));
			pApp->m_pHioIf->WriteFree(id, MULTI_PC2NNGC_ADDR, clearBuf, sizeof(clearBuf), FALSE);
			break;
		}
	case HIO2IF_EVENT_DISCONNECT:	// 接続解除
		pApp->m_pHioIf->Close(id);

		// Viewへメッセージ
		if ( (pView=GetActiveView(id)) != NULL )
			pView->PostMessage(WM_USER, CMultiView::EVENT_DISCONNECT, 0);
		break;
	case HIO2IF_EVENT_RECEIVED:		// データ受信
		{
			HIO2IF_RESULT result = pApp->m_pHioIf->Read(
									id,
									MULTI_NNGC2PC_ADDR,
									(LPVOID)&(pInfo->m_stPacket),
									MULTI_BUFFER_SIZE,
									pInfo->m_bSync);

			if ( HIO2IF_SUCCESS(result) && !pInfo->m_bSync )
			{
				if ( (pView=GetActiveView(id)) != NULL )
					pView->PostMessage(WM_USER, CMultiView::EVENCT_RECEIVED, 0);
			}
			break;
		}

	case HIO2IF_EVENT_READ_ASYNC_DONE:
		if ( (pView=GetActiveView(id)) != NULL )
			pView->PostMessage(WM_USER, CMultiView::EVENCT_RECEIVED, 0);
		break;

	case HIO2IF_EVENT_WRITE_ASYNC_DONE:
		if ( (pView=GetActiveView(id)) != NULL )
			pView->PostMessage(WM_USER, CMultiView::EVENT_WRITE_DONE, 0);
		break;

	case HIO2IF_EVENT_SEND_POSSIBLE:	// 送信可能状態
	default:
		break;
	}
}

// ReadFreeスレッド
static
DWORD WINAPI	PollingThread( LPVOID pHandle )
{
	CMultiApp* pApp = static_cast<CMultiApp *>(pHandle);

	pApp->m_bThreadBreak = FALSE;
	while ( !pApp->m_bThreadBreak )
	{
		for (int nCntID=0; nCntID<pApp->m_cInfoList.GetCount(); nCntID++ )
		{
			CMultiView* pView;
			DWORD dwStatus = 0;

			CMultiApp::LPAPPINFO pInfo = pApp->m_cInfoList.GetItem(nCntID);

			// 通信状態の取得
			pApp->m_pHioIf->ReadStatus(pInfo->m_nHioIfID, dwStatus);

			// 表示PC時間、通信状態の更新
			if ( (pView=GetActiveView(pInfo->m_nHioIfID)) != NULL )
				pView->PostMessage(WM_USER, CMultiView::EVENT_UPDATE_PC_TIME, dwStatus);

#ifndef	PROTOCOL_USED
			// 接続解除状態の時は、処理しない
			if ( !pApp->m_pHioIf->IsConnected(pInfo->m_nHioIfID) ) continue;

			////////////////////////////////////////////////////////////////////////////////////////////////////
			//データ受信
			////////////////////////////////////////////////////////////////////////////////////////////////////
			HIO2IF_RESULT result = pApp->m_pHioIf->ReadFree(
										pInfo->m_nHioIfID,
										MULTI_NNGC2PC_ADDR,
										(LPVOID)&(packetDB[nCntID]),
										MULTI_BUFFER_SIZE,
										pInfo->m_bSync);
			pView = GetActiveView(pInfo->m_nHioIfID);

			///////////////////////////////////////////////////////////////////////////////////////////////////
			//データコピー
			///////////////////////////////////////////////////////////////////////////////////////////////////
			//if(pInfo->m_stPacket.uNoPlayer == 2)
			//{
			//	for(int nCntXYZ = 0;nCntXYZ < 3;nCntXYZ++)
			//	{
			//		for(int nCnt = 0;nCnt < 6;nCnt++)
			//		{
			//			packetDB.uPosEnemy[nCntXYZ][nCnt] = pInfo->m_stPacket.uPosEnemy[nCntXYZ][nCnt];
			//		}
			//	}
			//	
			//	packetDB.uNoPlayer = pInfo->m_stPacket.uNoPlayer;
			//}
			//else if(pInfo->m_stPacket.uNoPlayer == 1)
			//{
			//	for(int nCntXYZ = 0;nCntXYZ < 3;nCntXYZ++)
			//	{
			//		for(int nCnt = 0;nCnt < 6;nCnt++)
			//		{
			//			packetDB.uPosPlayer[nCntXYZ][nCnt] = pInfo->m_stPacket.uPosPlayer[nCntXYZ][nCnt];
			//		}
			//	}

			//	packetDB.uNoPlayer = pInfo->m_stPacket.uNoPlayer;
			//}

			//if(CheckDataChange(packet))
			//{
			//	bSend = true;
			//}

			if ( HIO2IF_SUCCESS(result) && (pView != NULL) )
				pView->PostMessage(WM_USER, CMultiView::EVENCT_RECEIVED, 0);
#endif
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////
		//データ発信
		///////////////////////////////////////////////////////////////////////////////////////////////////
		for (int nCntID=0; nCntID<pApp->m_cInfoList.GetCount(); nCntID++ )
		{
			HIO2IF_RESULT result;
			CMultiView* pView;
			DWORD dwStatus = 0;
			CMultiApp::LPAPPINFO pInfo = pApp->m_cInfoList.GetItem(nCntID);

			// 通信状態の取得
			pApp->m_pHioIf->ReadStatus(pInfo->m_nHioIfID, dwStatus);

			// 表示PC時間、通信状態の更新
			if ( (pView=GetActiveView(pInfo->m_nHioIfID)) != NULL )
				pView->PostMessage(WM_USER, CMultiView::EVENT_UPDATE_PC_TIME, dwStatus);

			GetApp()->CreatePcTime(&packetDB[(nCntID + 1) % 2]);

			//pInfo->m_stPacket.uPosPlayerXOperator = packetDB.uPosPlayerXOperator;
			//pInfo->m_stPacket.uPosPlayerYOperator = packetDB.uPosPlayerYOperator;
			//pInfo->m_stPacket.uPosPlayerX = packetDB.uPosPlayerX;
			//pInfo->m_stPacket.uPosPlayerY = packetDB.uPosPlayerY;
			//pInfo->m_stPacket.nPlayer = packetDB.nPlayer;
			//pInfo->m_stPacket.uPosEnemyXOperator = packetDB.uPosEnemyXOperator;
			//pInfo->m_stPacket.uPosEnemyYOperator = packetDB.uPosEnemyYOperator;
			//pInfo->m_stPacket.uPosEnemyX = packetDB.uPosEnemyX;
			//pInfo->m_stPacket.uPosEnemyY = packetDB.uPosEnemyY;

			int nIndex =
				GetApp()->m_cInfoList.IndexOf((LPVOID)pInfo->m_nHioIfID, MultiCompID);
			BOOL bSync = GetApp()->m_cInfoList.GetItem(nIndex)->m_bSync;

			GetApp()->m_pHioIf->EnterCriticalSection();
			result = GetApp()->m_pHioIf->WriteFree(
							pInfo->m_nHioIfID,
							MULTI_PC2NNGC_ADDR,
							(LPVOID)&packetDB[(nCntID + 1) % 2],
							MULTI_BUFFER_SIZE,
							bSync);

			GetApp()->m_pHioIf->LeaveCriticalSection();
		}

		// 約1V毎にRead
		Sleep(1000 / 60);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// CMultiApp - Host I/O API interface
//

//-----------------------------------------------------------------------------
BOOL	CMultiApp::HioIfInit()
{
	// 初期化
	if ( HIO2IF_FAILED(m_pHioIf->Init()) ) return FALSE;

	// 検出したEXIチャネル数をオープン
	for (int i=0; i<m_pHioIf->GetDeviceCount(); i++)
	{
		HIO2IF_ID id;
		HIO2DevicePath pPath = m_pHioIf->GetDevicePath(i);
		if ( HIO2IF_FAILED(m_pHioIf->Open(pPath, HioIfEventCallback, id)) )
			return FALSE;

		LPAPPINFO pInfo = new APPINFO;
#ifndef	HW0
		pInfo->m_pDevice = new TCHAR [lstrlen(pPath) + 1];
		lstrcpy((LPSTR)pInfo->m_pDevice, pPath);
#else	// HW0
		pInfo->m_pDevice = pPath;
#endif	// HW0
		pInfo->m_nHioIfID = id;
		m_cInfoList.AddTail(pInfo);
	}

	// 表示PC時間の更新 & CHioIf->ReadFree()を実行する為のスレッドを作成
	DWORD dwTmp;

	m_bThreadBreak = TRUE;
	m_hThread = ::CreateThread(NULL, 0, PollingThread, (LPVOID)this, 0, &dwTmp);

	// スレッドが起動するまで待つ
	// PollingThread関数内でm_bThreadBreakにFALSEを設定）
	while ( m_bThreadBreak ) ;

	// プロトコルを使用した通信を行う場合は、HIO2IF_EVENT_CALLBACKでCHioIf->Read
	// を呼ぶ

	return TRUE;
}

//-----------------------------------------------------------------------------
void	CMultiApp::CreateInfo()
{
	POSITION pos = GetFirstDocTemplatePosition();
	ASSERT(pos != NULL);
	CMultiDocTemplate* pDocTemplate =
		static_cast<CMultiDocTemplate *>(GetNextDocTemplate(pos));

	for (int i=0; i<m_cInfoList.GetCount(); i++)
	{
		LPAPPINFO pInfo = m_cInfoList.GetItem(i);
		CMultiDoc* pDoc =
			static_cast<CMultiDoc *>(pDocTemplate->OpenDocumentFile(NULL, TRUE));
		pDoc->m_nHioIfID = pInfo->m_nHioIfID;
		pDoc->UpdateTitle();
	}
}


//-----------------------------------------------------------------------------
BOOL	CMultiApp::HioIfExit()
{
	// PollingThreadが有効な場合はスレッドを終了
	StopPollingThread();

	m_pHioIf->EnterCriticalSection();
	m_pHioIf->Exit();	// HIO終了
	m_pHioIf->LeaveCriticalSection();

	m_cInfoList.Clear();

	return TRUE;
}

//-----------------------------------------------------------------------------
void	CMultiApp::StopPollingThread()
{
	// PollingThreadが有効な場合はスレッドを終了
	if ( m_hThread != INVALID_HANDLE_VALUE )
	{
        // EXI スレッドを停止
		m_bThreadBreak = TRUE;
		WaitForSingleObject(m_hThread,1000*30);
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
	}
}

void	CMultiApp::Connect(HIO2IF_ID id)
{
	m_pHioIf->EnterCriticalSection();

	int nIndex = m_cInfoList.IndexOf((LPVOID)id, MultiCompID);
	LPAPPINFO pInfo = m_cInfoList.GetItem(nIndex);

	if ( HIO2IF_FAILED(m_pHioIf->Open(pInfo->m_pDevice,	HioIfEventCallback, id)) )
	{
		MessageBox(NULL, "reopen failed", "ERROR", MB_OK);
	}

	m_pHioIf->LeaveCriticalSection();
}

void	CMultiApp::Disconnect(HIO2IF_ID id)
{
	m_pHioIf->EnterCriticalSection();
	m_pHioIf->Close(id);
	m_pHioIf->LeaveCriticalSection();

	CMultiView* pView = GetActiveView(id);
	if ( pView != NULL )
		pView->PostMessage(WM_USER, CMultiView::EVENT_DISCONNECT, 0);
}

void	CMultiApp::CreatePcTime(MULTI_PACKET* pPacket)
{
	struct tm *pLocalTime;
	__time64_t lTime;
//	errno_t err;

	_time64(&lTime);
//	err = _localtime64_s( &LocalTime, &lTime );
	//pLocalTime = _localtime64(&lTime);

	//pPacket->mon	= pLocalTime->tm_mon + 1;
	//pPacket->mday	= pLocalTime->tm_mday;
	//pPacket->hour	= pLocalTime->tm_hour;
	//pPacket->min	= pLocalTime->tm_min;
	//pPacket->sec	= pLocalTime->tm_sec;
	//pPacket->msec	= 0;
//    strcpy_s( pPacket->string, sizeof( pPacket->string ), "PC TIME" );
//    strcpy( pPacket->string, "PC TIME" );
}

///////////////////////////////////////////////////////////////////////////////
//
// CMultiApp
//

BEGIN_MESSAGE_MAP(CMultiApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// CMultiApp construction

CMultiApp::CMultiApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CMultiApp object

CMultiApp theApp;

// CMultiApp initialization

BOOL CMultiApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	////////// for multiApp //////////
	m_pHioIf = Hio2CreateInterface();
	if ( !HioIfInit() )
	{
		MessageBox(NULL, m_pHioIf->GetMessage(), "ERROR", (MB_OK | MB_ICONWARNING));
		m_cInfoList.Clear();
		return FALSE;
	}

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_multiTYPE,
		RUNTIME_CLASS(CMultiDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CMultiView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;
	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
#if 0
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
#endif
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	////////// for multiApp //////////
	// 各EXIチャネル毎のウインドウを作成
	CreateInfo();

	return TRUE;
}

// CMultiApp message handlers


int CMultiApp::ExitInstance()
{
	////////// for multiApp //////////
	HioIfExit();
	Hio2ReleaseInterface(m_pHioIf);

	return CWinApp::ExitInstance();
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CMultiApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}