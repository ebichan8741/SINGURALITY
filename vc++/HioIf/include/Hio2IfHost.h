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
// CHio2If�̓��������Ɏg�p (private)
//

#define	HIO2IF_INVALID_HANDLE_VALUE	((LPVOID)-1)

// Host I/O interface�̏��
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

	HIO2DevicePath	m_pPathName;		// �f�o�C�X�p�X��
	int				m_nPcChan;			// PC���`���l��(CHio2If���[���I�ɍ쐬)
	HIO2DeviceType	m_nDevType;			// EXI�f�o�C�X�^�C�v
	LPVOID			m_hHIO;				// Host I/O API�n���h��
	HIO2IF_MODE		m_nMode;			// �I�[�v�����[�h
	volatile BOOL	m_bConnect;			// NNGC�Ƃ̐ڑ���
	volatile BOOL	m_bReceived;		// NNGC����̃f�[�^��M�ʒm
	volatile BOOL	m_bSendPossible;	// NNGC�֑��M�\�ȏ��
	volatile DWORD	m_dwAsyncMode;		// Async���[�h
	HIO2IF_EVENT_CALLBACK	m_fncCallback;	// �C�x���g��M���̃R�[���o�b�N�֐�
} HIO2IF_STATUS, *LPHIO2IF_STATUS;

// Host I/O interface�̏�ԃ��X�g�A��r�p�֐�
//
// �� �{class��index�́ACHio2If->Open()�Ŏ擾����HIO2IF_ID
//

typedef TMYList<LPHIO2IF_STATUS>	CStatusList;

// CtatusList.IndexOf()�Ɏw�肷���r�֐�
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

	// �f�o�C�X�p�X���Q�Ɨp���\�b�h
	int	GetDeviceCount()	const { return (int)m_cDevices.size();	}
	HIO2DevicePath	GetDevicePath(int nIndex)	const
	{
		ASSERT((nIndex >= 0) && (nIndex < (int)m_cDevices.size()));
		return m_cDevices[nIndex];
	}

	// HIO2 API���䃁�\�b�h
	HIO2IF_RESULT	Init();
	HIO2IF_RESULT	Open(HIO2DevicePath pathName, HIO2IF_EVENT_CALLBACK callback, HIO2IF_ID& id);
	HIO2IF_RESULT	Read(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	ReadFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	Write(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	WriteFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async);
	HIO2IF_RESULT	ReadStatus(HIO2IF_ID id, DWORD& status);
	HIO2IF_RESULT	Close(HIO2IF_ID id);
	void			Exit();

	// ��ԎQ�Ɨp���\�b�h
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

	// �}���`�X�X���b�h�A�v���p���\�b�h
	void	EnterCriticalSection()	{ ::EnterCriticalSection(&m_csCriticalSection); }
	void	LeaveCriticalSection()	{ ::LeaveCriticalSection(&m_csCriticalSection); }

	// �G���[���擾���\�b�h
	HIO2IF_ERROR	GetLastError()	const { return m_nLastError;	}
	LPCSTR	GetMessage()			const { return m_szMessage;		}

	//!!!!!!!!!! �A�v���P�[�V�����Ŏg�p���֎~���郁�\�b�h !!!!!!!!!!

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
	BOOL	m_bInitialized;					// ����������t���O

	deque<HIO2DevicePath>	m_cDevices;		// �f�o�C�X�p�X���
	CStatusList	m_cHioStatus;				// �X�e�[�^�X���

	HIO2IF_ERROR	m_nLastError;			// �G���[�R�[�h
	TCHAR	m_szMessage[128];				// �G���[���b�Z�[�W

	CRITICAL_SECTION	m_csCriticalSection;	// �}���`�X���b�h�A�v���p

	CHio2DllIf	m_cHio2Dll;	// hio2[D].dll�C���^�t�F�[�X

	static LPCSTR	m_lpszErrorStrings[HIO2IF_ERROR_MAX];	// �G���[���b�Z�[�W

	void	InitInstance();

	BOOL	IsValidID(HIO2IF_ID id)
	{
		return ((id >= 0) && (id < m_cHioStatus.GetCount())) ? TRUE : FALSE;
	};

	// (Fatal)Error���b�Z�[�W�ݒ�
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
