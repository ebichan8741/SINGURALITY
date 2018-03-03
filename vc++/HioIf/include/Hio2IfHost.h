/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2IfHost.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/HioIf/include/Hio2IfHost.h,v 1.4 2007/11/26 13:55:39 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// HIO2 API wrapper interface

#pragma	once
#pragma warning(disable: 4311)
#pragma warning(disable: 4312)

#include <revolution/hio2.h>
#include <win32/mytl.h>	// NNGC SDK : $(ROOT)/include/win32
#include "Hio2If.h"		// ../../HioIf/include
#include "Hio2DllIf.h"

///////////////////////////////////////////////////////////////////////////////
//
// CHio2Ifの内部処理に使用 (private)
//

#define	HIO2IF_INVALID_HANDLE_VALUE	((LPVOID)-1)

// Host I/O interfaceの状態
typedef struct _HIO2IF_STATUS
{
	_HIO2IF_STATUS()
	{
		m_nPcChan = HIO2IF_INVALID_ID;
		m_nDevType = HIO2_DEVICE_INVALID;
		m_hHIO	= HIO2IF_INVALID_HANDLE_VALUE;
		m_nMode	= HIO2IF_MODE_NONE;
		m_bConnect	= FALSE;
		m_bReceived	= FALSE;
		m_bSendPossible = TRUE;
		m_dwAsyncMode = 0;
		m_fncCallback = NULL;
#ifndef	HW0
		m_pPathName = NULL;
#else	// HW0
		m_pPathName = -1;
#endif	// HW0
	}

	~_HIO2IF_STATUS()
	{
#ifndef	HW0
		if ( m_pPathName != NULL ) delete [] m_pPathName;
#endif	// HW0
	}

	HIO2DevicePath	m_pPathName;		// デバイスパス名
	int				m_nPcChan;			// PC側チャネル(CHio2Ifが擬似的に作成)
	HIO2DeviceType	m_nDevType;			// EXIデバイスタイプ
	LPVOID			m_hHIO;				// Host I/O APIハンドル
	HIO2IF_MODE		m_nMode;			// オープンモード
	volatile BOOL	m_bConnect;			// NNGCとの接続状況
	volatile BOOL	m_bReceived;		// NNGCからのデータ受信通知
	volatile BOOL	m_bSendPossible;	// NNGCへ送信可能な状態
	volatile DWORD	m_dwAsyncMode;		// Asyncモード
	HIO2IF_EVENT_CALLBACK	m_fncCallback;	// イベント受信時のコールバック関数
} HIO2IF_STATUS, *LPHIO2IF_STATUS;

// Host I/O interfaceの状態リスト、比較用関数
//
// ※ 本classのindexは、CHio2If->Open()で取得したHIO2IF_ID
//

typedef TMYList<LPHIO2IF_STATUS>	CStatusList;

// CtatusList.IndexOf()に指定する比較関数
BOOL	Hio2IfCompHandle(LPHIO2IF_STATUS pItem, LPVOID pData);
BOOL	Hio2IfCompPcChan(LPHIO2IF_STATUS pItem, LPVOID pData);

///////////////////////////////////////////////////////////////////////////////
//
// CHio2If
//

// Host I/O interface for PC

class CHio2If
{
public:
	CHio2If();
	~CHio2If();

	// デバイスパス情報参照用メソッド
	int	GetDeviceCount()	const { return (int)m_cDevices.size();	}
	HIO2DevicePath	GetDevicePath(int nIndex)	const
	{
		ASSERT((nIndex >= 0) && (nIndex < (int)m_cDevices.size()));
		return m_cDevices[nIndex];
	}

	// HIO2 API制御メソッド
	HIO2IF_RESULT	Init();
	HIO2IF_RESULT	Open(HIO2DevicePath pathName, HIO2IF_EVENT_CALLBACK callback, HIO2IF_ID& id);
	HIO2IF_RESULT	Read(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	ReadFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	Write(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	WriteFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	ReadStatus(HIO2IF_ID id, DWORD& status);
	HIO2IF_RESULT	Close(HIO2IF_ID id);
	void			Exit();

	// 状態参照用メソッド
	BOOL	IsConnected(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_bConnect : FALSE;
	}
	BOOL	IsReceived(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_bReceived : FALSE;
	}
	BOOL	IsSendPossible(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_bSendPossible : FALSE;
	}
	HIO2IF_MODE	GetOpenMode(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_nMode : HIO2IF_MODE_NONE;
	}
	int	GetPcChan(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_nPcChan : HIO2IF_INVALID_ID;
	}
	HIO2DeviceType	GetDeviceType(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_nDevType : HIO2_DEVICE_INVALID;
	}

	// マルチススレッドアプリ用メソッド
	void	EnterCriticalSection()	{ ::EnterCriticalSection(&m_csCriticalSection); }
	void	LeaveCriticalSection()	{ ::LeaveCriticalSection(&m_csCriticalSection); }

	// エラー情報取得メソッド
	HIO2IF_ERROR	GetLastError()	const { return m_nLastError;	}
	LPCSTR	GetMessage()			const { return m_szMessage;		}

	//!!!!!!!!!! アプリケーションで使用を禁止するメソッド !!!!!!!!!!

	CHio2DllIf&	GetDllIf()	{ return m_cHio2Dll;	};
	void	SetDeviceType(HIO2IF_ID id, HIO2DeviceType type)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_nDevType = type;
	};
	void	SetOpenMode(HIO2IF_ID id, HIO2IF_MODE mode)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_nMode = mode;
	};
	void	SetConnect(HIO2IF_ID id, BOOL bStatus)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_bConnect = bStatus;
	};
	void	SetReceived(HIO2IF_ID id, BOOL bStatus)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_bReceived = bStatus;
	};
	void	SetSendPossible(HIO2IF_ID id, BOOL bStatus)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_bSendPossible = bStatus;
	};
	DWORD	GetAsyncMode(HIO2IF_ID id)
	{
		return IsValidID(id) ? m_cHioStatus[id]->m_dwAsyncMode : 0;
	};
	void	SetAsyncMode(HIO2IF_ID id, DWORD mode)
	{
		if ( IsValidID(id) ) m_cHioStatus[id]->m_dwAsyncMode = mode;
	}
	void	CallEventCallback(HIO2IF_ID id, HIO2IF_EVENT event)
	{
		if ( (IsValidID(id)) && (m_cHioStatus[id]->m_fncCallback != NULL) )
			m_cHioStatus[id]->m_fncCallback(id, event);
	}
	int	GetIdOfHandle(LPVOID h)
	{
		return m_cHioStatus.IndexOf(h,Hio2IfCompHandle);
	}
	void	AddDevicePath(HIO2DevicePath pathName);

protected:
	BOOL	m_bInitialized;					// 初期化判定フラグ

	deque<HIO2DevicePath>	m_cDevices;		// デバイスパス情報
	CStatusList	m_cHioStatus;				// ステータス情報

	HIO2IF_ERROR	m_nLastError;			// エラーコード
	TCHAR	m_szMessage[128];				// エラーメッセージ

	CRITICAL_SECTION	m_csCriticalSection;	// マルチスレッドアプリ用

	CHio2DllIf	m_cHio2Dll;	// hio2[D].dllインタフェース

	static LPCSTR	m_lpszErrorStrings[HIO2IF_ERROR_MAX];	// エラーメッセージ

	void	InitInstance();

	BOOL	IsValidID(HIO2IF_ID id)
	{
		return ((id >= 0) && (id < m_cHioStatus.GetCount())) ? TRUE : FALSE;
	};

	// (Fatal)Errorメッセージ設定
	HIO2IF_RESULT	SetFatal(HIO2IF_ERROR errID, ...);
	HIO2IF_RESULT	SetError(HIO2IF_ERROR errID, ...);
};

//-----------------------------------------------------------------------------
// function prototypes

// create Host I/O interface for PC
CHio2If*	Hio2CreateInterface(void);

// release Host I/O interface for PC
void	Hio2ReleaseInterface(CHio2If* pHioIf);

// end of Hio2IfHost.h
