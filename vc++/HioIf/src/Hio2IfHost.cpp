/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2IfHost.cpp

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/HioIf/src/Hio2IfHost.cpp,v 1.4 2007/11/26 13:55:55 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// Host I/O API wrapper interface

#include "stdafx.h"
#include "Hio2IfHost.h"
#include <stdarg.h>

///////////////////////////////////////////////////////////////////////////////
//
// callback�֐��Q�ƗpCHiosIf�I�u�W�F�N�g
//

static CHio2If*	l_pHio2If = NULL;


///////////////////////////////////////////////////////////////////////////////
//
// CStatusList->IndexOf�p��r�֐�
//

// HIO�n���h���̔�r
BOOL	Hio2IfCompHandle(LPHIO2IF_STATUS pItem, LPVOID pData)
{
	// pData == �n���h��
	return (pItem->m_hHIO == pData) ? TRUE : FALSE;
}

// PC���`���l���̔�r
BOOL	Hio2IfCompPcChan(LPHIO2IF_STATUS pItem, LPVOID pData)
{
#ifndef	HW0
	return (lstrcmp(pItem->m_pPathName, (LPCSTR)pData) == 0) ? TRUE : FALSE;
#else	// HW0
	return (pItem->m_pPathName == (HIO2DevicePath)pData) ? TRUE : FALSE;
#endif	// HW0
}

///////////////////////////////////////////////////////////////////////////////
//
// error strings
//

LPCSTR	CHio2If::m_lpszErrorStrings[HIO2IF_ERROR_MAX] =
{

#include "Hio2IfErr.str"

};

///////////////////////////////////////////////////////////////////////////////
//
// callback
//

// HIO2EnumDevices() - callback
static
BOOL	hio2EnumCallback(HIO2DevicePath pathName, void* param)
{
	CHio2If* pHio2If = static_cast<CHio2If *>(param);
	pHio2If->AddDevicePath(pathName);
	return TRUE;
}

// HIO2Open() - callback
static
void	hio2Callback(HIO2Handle h)
{
	static const HIO2IF_MODE	l_nOpenMode[] =
	{
		HIO2IF_MODE_WRONLY,	// ���M�̂�
		HIO2IF_MODE_RDONLY,	// ��M�̂�
		HIO2IF_MODE_RDWR		// ����M
	};

	// �}���`�X���b�h�p
	l_pHio2If->EnterCriticalSection();

	HIO2IF_EVENT event;
	int id, cmd;
	u32 mail = 0;

	// HIO�n���h������CHio2If�pID���擾
	id = l_pHio2If->GetIdOfHandle(h);

	l_pHio2If->GetDllIf().ReadMailbox(h, &mail);

	cmd = HIO2IF_GET_PACKET_CMD(mail);

	switch ( cmd )
	{
	case HIO2IF_CMD_OPEN_RDONLY:
	case HIO2IF_CMD_OPEN_WRONLY:
	case HIO2IF_CMD_OPEN_RDWR:
		// NNGC���I�[�v���ʒm���͂����ꍇ�A�ȉ��̐ݒ���s���ANNGC�ɒʒm
		// 1) PC�Ƃ̐ڑ��m��
		// 2) �I�[�v�����[�h�̐ݒ�
		//    Read Only�AWrite Only�̏ꍇ�́ANNGC�Ƒ΁i�t�j�̐ݒ�ɂȂ�
		// 3) NNGC EXI�`���l���ԍ�
		l_pHio2If->SetConnect(id, TRUE);
		l_pHio2If->SetOpenMode(id, l_nOpenMode[cmd -1]);
		l_pHio2If->SetDeviceType(id, (HIO2DeviceType)HIO2IF_GET_PACKET_CHAN(mail));
		l_pHio2If->GetDllIf().WriteMailbox(h,
            HIO2IF_SET_PACKET(
				l_pHio2If->GetPcChan(id),
				HIO2IF_CMD_OPEN_RESULT
			)
		);
		event = HIO2IF_EVENT_CONNECT;
		break;
	case HIO2IF_CMD_SEND:
		l_pHio2If->SetReceived(id, TRUE);
		event = HIO2IF_EVENT_RECEIVED;
		break;
	case HIO2IF_CMD_SEND_RESULT:
		l_pHio2If->SetSendPossible(id, TRUE);
		event = HIO2IF_EVENT_SEND_POSSIBLE;
		break;
	case HIO2IF_CMD_CLOSE:
		// ���ۂ̃N���[�Y�����́A�A�v���ōs������
		l_pHio2If->SetConnect(id, FALSE);
		event = HIO2IF_EVENT_DISCONNECT;
		break;
	default:
		event = HIO2IF_EVENT_UNKOWN;
		break;
	}

	// �}���`�X���b�h�p
	l_pHio2If->LeaveCriticalSection();

	l_pHio2If->CallEventCallback(id, event);
}

// HIOReadAsync() - callback
static
void	hio2ReadAsyncCallback(HIO2Handle h)
{
	// HIO�n���h������CHio2If�pID���擾
	int id = l_pHio2If->GetIdOfHandle(h);
	DWORD async = HIO2IF_ASYNC_READ_MASK(l_pHio2If->GetAsyncMode(id));

	// CHio2If::Read�ɂ��Async�w��̏ꍇ�A�ڑ������Read�����ʒm���s��
	// �iCHio2If::ReadFree�́A�ʒm�s�v�j
	if ( async & HIO2IF_ASYNC_READ )
	{
		// �}���`�X���b�h�p
		l_pHio2If->EnterCriticalSection();

		l_pHio2If->GetDllIf().WriteMailbox(h,
            HIO2IF_SET_PACKET(
				l_pHio2If->GetPcChan(id),
				HIO2IF_CMD_SEND_RESULT
			)
		);

		// �}���`�X���b�h�p
		l_pHio2If->LeaveCriticalSection();
	}

	l_pHio2If->SetAsyncMode(id, l_pHio2If->GetAsyncMode(id) & ~async);

	// �C�x���g�R�[���o�b�N�֐��Ăяo��
	l_pHio2If->CallEventCallback(id, HIO2IF_EVENT_READ_ASYNC_DONE);
}

// HIOWriteAsync() - callback
static
void	hio2WriteAsyncCallback(HIO2Handle h)
{
	// HIO�n���h������CHio2If�pID���擾
	int id = l_pHio2If->GetIdOfHandle(h);
	DWORD async = HIO2IF_ASYNC_WRITE_MASK(l_pHio2If->GetAsyncMode(id));

	// CHio2If::Write�ɂ��Async�w��̏ꍇ�A�ڑ������Write�ʒm���s��
	// �iCHio2If::WriteFree�́A�ʒm�s�v�j
	if ( async & HIO2IF_ASYNC_WRITE )
	{
		// �}���`�X���b�h�p
		l_pHio2If->EnterCriticalSection();

		l_pHio2If->GetDllIf().WriteMailbox(h,
            HIO2IF_SET_PACKET(
				l_pHio2If->GetPcChan(id),
				HIO2IF_CMD_SEND
			)
		);

		// �}���`�X���b�h�p
		l_pHio2If->LeaveCriticalSection();
	}

	l_pHio2If->SetAsyncMode(id, l_pHio2If->GetAsyncMode(id) & ~async);

	// �C�x���g�R�[���o�b�N�֐��Ăяo��
	l_pHio2If->CallEventCallback(id, HIO2IF_EVENT_WRITE_ASYNC_DONE);
}

///////////////////////////////////////////////////////////////////////////////
//
// CHio2If
//

//-----------------------------------------------------------------------------
CHio2If::CHio2If()
{
	m_szMessage[0]	= '\0';
	m_nLastError = HIO2IF_ERROR_NONE;
	m_bInitialized = FALSE;
	InitializeCriticalSection(&m_csCriticalSection); 
}

CHio2If::~CHio2If()
{
	Exit();
	DeleteCriticalSection(&m_csCriticalSection); 
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::Init()
{
	// �������ς݂̏ꍇ�AHIO���I��������
	if ( m_bInitialized ) m_cHio2Dll.Exit();
	else if ( !m_cHio2Dll.Create() ) return SetFatal(HIO2IF_FATAL_LOAD_DLL);

	InitInstance();

	// HIO������
	if ( !m_cHio2Dll.Init() )
		return SetFatal(HIO2IF_FATAL_INIT, m_cHio2Dll.GetLastError());

	// EXI�f�o�C�X������
	if ( m_cHio2Dll.EnumDevices(hio2EnumCallback, this) == -1 )
		return SetFatal(HIO2IF_FATAL_ENUMDEVICES, m_cHio2Dll.GetLastError());

	m_bInitialized = TRUE;

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::Open(HIO2DevicePath pathName, HIO2IF_EVENT_CALLBACK callback, HIO2IF_ID& id)
{
	LPHIO2IF_STATUS pStatus = NULL;
	int nIndex, nPcChan = 0;

#ifndef	HW0
	// �f�o�C�X�p�X���Ɋ܂܂�鐔�l���[��PC�`���l���ԍ��Ƃ���
	LPCSTR p = pathName;
	while ( *p != '\0' )
	{
		if ( (*p >= '0') && (*p <= '9') )
		{
			nPcChan *= 10;
			nPcChan += *p - '0';
		}
		p++;
	}
#else	// HW0
	nPcChan = pathName;
#endif	// HW0
	nPcChan &= HIO2IF_CMD_MASK;

	// �w��EXI�`���l�����g�p���̏ꍇ
	nIndex = m_cHioStatus.IndexOf((LPVOID)pathName, Hio2IfCompPcChan);
	if ( nIndex != -1 )
	{
		pStatus = m_cHioStatus[nIndex];
		if ( pStatus->m_hHIO != HIO2IF_INVALID_HANDLE_VALUE )
			return SetError(HIO2IF_ERROR_CHAN_ALREADY_OPENED, pathName);
	}

	HIO2Handle h = m_cHio2Dll.Open(pathName, hio2Callback, NULL, NULL);
	if ( h == HIO2_INVALID_HANDLE_VALUE )
		return SetFatal(HIO2IF_FATAL_OPEN, pathName, m_cHio2Dll.GetLastError());

	// �ȑO�ɃI�[�v�������`���l���i���o���Ă����`���l���j�̏ꍇ
	if ( nIndex != -1 )
		id = nIndex;
	else
	{
		pStatus = new HIO2IF_STATUS;
		id = m_cHioStatus.AddTail(pStatus);
	}

	// Host I/O interface�̏�����Ԑݒ�
#ifndef	HW0
	pStatus->m_pPathName = new TCHAR [lstrlen(pathName) + 1];
	lstrcpy((LPSTR)pStatus->m_pPathName, pathName);
#else	// HW0
	pStatus->m_pPathName = pathName;
#endif	// HW0
	pStatus->m_nPcChan		= nPcChan;
	pStatus->m_hHIO			= h;
	pStatus->m_fncCallback	= callback;

	return HIO2IF_RESULT_SUCCESS; 
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::Read(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];

	// Write only ���[�h�̏ꍇ
	if ( pStatus->m_nMode == HIO2IF_MODE_WRONLY )
		return SetError(HIO2IF_ERROR_WRITE_ONLY, pStatus->m_nPcChan);

	// PC�Ɛڑ�����Ă��Ȃ�
	if ( !pStatus->m_bConnect )
		return SetError(HIO2IF_ERROR_NOT_CONNECT, pStatus->m_nPcChan);

	// �f�[�^����M���Ă��Ȃ��ꍇ
	if ( !pStatus->m_bReceived )
		return SetError(HIO2IF_ERROR_NOT_RECV_DATA, pStatus->m_nPcChan);

	pStatus->m_bReceived = FALSE;

	// ������Read
	if ( !async )
	{
		if ( !m_cHio2Dll.Read(pStatus->m_hHIO, addr, buffer, size) )
			return SetFatal(HIO2IF_FATAL_READ,
							pStatus->m_nPcChan, m_cHio2Dll.GetLastError());

		// �ڑ������Read�����ʒm���s��
		m_cHio2Dll.WriteMailbox(pStatus->m_hHIO,
			HIO2IF_SET_PACKET(pStatus->m_nPcChan, HIO2IF_CMD_SEND_RESULT));
	}
	// �񓯊���Read
	else
	{
		pStatus->m_dwAsyncMode |= HIO2IF_ASYNC_READ;
		if ( !m_cHio2Dll.ReadAsync(pStatus->m_hHIO, addr, buffer, size, hio2ReadAsyncCallback) )
			return SetFatal(HIO2IF_FATAL_READ,
							pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::ReadFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];

	// Write only ���[�h�̏ꍇ
	if ( pStatus->m_nMode == HIO2IF_MODE_WRONLY )
		return SetError(HIO2IF_ERROR_WRITE_ONLY, pStatus->m_nPcChan);

	// PC�Ɛڑ�����Ă��Ȃ�
	if ( !pStatus->m_bConnect )
		return SetError(HIO2IF_ERROR_NOT_CONNECT, pStatus->m_nPcChan);

	// ������Read
	if ( !async )
	{
		if ( !m_cHio2Dll.Read(pStatus->m_hHIO, addr, buffer, size) )
			return SetFatal(HIO2IF_FATAL_READ,
							pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}
	// �񓯊���Read
	else
	{
		// �ȑO��ReadAync or WriteAsync���������Ă��Ȃ�
		if  ( pStatus->m_dwAsyncMode )
			return SetError(HIO2IF_ERROR_BUSY, pStatus->m_nPcChan);

		pStatus->m_dwAsyncMode |= HIO2IF_ASYNC_READ_FREE;
		if ( !m_cHio2Dll.ReadAsync(pStatus->m_hHIO, addr, buffer, size, hio2ReadAsyncCallback) )
			return SetFatal(HIO2IF_FATAL_READ,
							pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::Write(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];

	// Read only ���[�h�̏ꍇ
	if ( pStatus->m_nMode == HIO2IF_MODE_RDONLY )
		return SetError(HIO2IF_ERROR_READ_ONLY, pStatus->m_nPcChan);

	// PC�Ɛڑ�����Ă��Ȃ�
	if ( !pStatus->m_bConnect )
		return SetError(HIO2IF_ERROR_NOT_CONNECT, pStatus->m_nPcChan);

	// ���M�ł��Ȃ��ꍇ�i����悪�f�[�^����M�ł��Ȃ��ꍇ�j
	if ( !pStatus->m_bSendPossible )
		return SetError(HIO2IF_ERROR_CANNOT_SEND_DATA, pStatus->m_nPcChan);

	pStatus->m_bSendPossible = FALSE;

	// ������Write
	if ( !async )
	{
		if ( !m_cHio2Dll.Write(pStatus->m_hHIO, addr, buffer, size) )
			return SetFatal(HIO2IF_FATAL_WRITE,
				pStatus->m_nPcChan, m_cHio2Dll.GetLastError());

		// �ڑ������Write�ʒm���s��
		m_cHio2Dll.WriteMailbox(pStatus->m_hHIO,
			HIO2IF_SET_PACKET(pStatus->m_nPcChan, HIO2IF_CMD_SEND));
	}
	// �񓯊���Write
	else
	{
		pStatus->m_dwAsyncMode |= HIO2IF_ASYNC_WRITE;
		if ( !m_cHio2Dll.WriteAsync(pStatus->m_hHIO, addr, buffer, size, hio2WriteAsyncCallback) )
			return SetFatal(HIO2IF_FATAL_WRITE,
				pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::WriteFree(HIO2IF_ID id, DWORD addr, LPVOID buffer, int size, BOOL async)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];

	// Read only ���[�h�̏ꍇ
	if ( pStatus->m_nMode == HIO2IF_MODE_RDONLY )
		return SetError(HIO2IF_ERROR_READ_ONLY, pStatus->m_nPcChan);

	// PC�Ɛڑ�����Ă��Ȃ�
	if ( !pStatus->m_bConnect )
		return SetError(HIO2IF_ERROR_NOT_CONNECT, pStatus->m_nPcChan);

	// ������Write
	if ( !async )
	{
		if ( !m_cHio2Dll.Write(pStatus->m_hHIO, addr, buffer, size) )
			return SetFatal(HIO2IF_FATAL_WRITE,
							pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}
	// �񓯊���Write
	else
	{
		// �ȑO��ReadAync or WriteAsync���������Ă��Ȃ�
		if  ( pStatus->m_dwAsyncMode )
			return SetError(HIO2IF_ERROR_BUSY, pStatus->m_nPcChan);

		pStatus->m_dwAsyncMode |= HIO2IF_ASYNC_WRITE_FREE;
		if ( !m_cHio2Dll.WriteAsync(pStatus->m_hHIO, addr, buffer, size, hio2WriteAsyncCallback) )
			return SetFatal(HIO2IF_FATAL_WRITE,
				pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::ReadStatus(HIO2IF_ID id, DWORD& status)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];

	// PC�Ɛڑ�����Ă��Ȃ�
	if ( !pStatus->m_bConnect )
		return SetError(HIO2IF_ERROR_NOT_CONNECT, pStatus->m_nPcChan);

	return m_cHio2Dll.ReadStatus(pStatus->m_hHIO, (u32 *)&status)
		? HIO2IF_RESULT_SUCCESS
		: SetFatal(HIO2IF_FATAL_READSTATUS, 
			pStatus->m_nPcChan, m_cHio2Dll.GetLastError());
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::Close(HIO2IF_ID id)
{
	if ( !IsValidID(id) ) return SetError(HIO2IF_ERROR_INVALID_ID);

	LPHIO2IF_STATUS pStatus = m_cHioStatus[id];
	int chan = pStatus->m_nPcChan;

	// ����Ɛڑ����Ă���ꍇ�A�N���[�Y�ʒm���s��
	if ( pStatus->m_bConnect )
	{
		pStatus->m_bConnect	= FALSE;
		m_cHio2Dll.WriteMailbox(pStatus->m_hHIO,
			HIO2IF_SET_PACKET(chan, HIO2IF_CMD_CLOSE));
	}

	BOOL result = m_cHio2Dll.Close(pStatus->m_hHIO);

	// �ăI�[�v������\�����l�����A�폜���Ȃ��ŏ������̂ݍs��
	// (�`���l���ԍ��́A�c���Ă���)
//	pStatus->m_nPcChan			= HIO2IF_INVALID_ID;
	pStatus->m_nDevType			= HIO2_DEVICE_INVALID;
	pStatus->m_hHIO				= HIO2IF_INVALID_HANDLE_VALUE;
	pStatus->m_nMode			= HIO2IF_MODE_NONE;
	pStatus->m_bReceived		= FALSE;
	pStatus->m_bSendPossible	= TRUE;
	pStatus->m_fncCallback		= NULL;

	return result ? HIO2IF_RESULT_SUCCESS
		: SetFatal(HIO2IF_FATAL_CLOSE, chan, m_cHio2Dll.GetLastError());
}

//-----------------------------------------------------------------------------
void	CHio2If::Exit()
{
	if ( !m_bInitialized ) return ;

	// EXI�`���l�����I�[�v������Ă��āAPC�Ɛڑ����Ă���ꍇ�A�N���[�Y�ʒm���s��
	for (int i=0; i<m_cHioStatus.GetCount(); i++)
	{
		if ( m_cHioStatus[i]->m_hHIO != HIO2IF_INVALID_HANDLE_VALUE ) Close(i);
	}

	// HIO2Exit()�́A�I�[�v���ς݃`���l���������N���[�Y����
	m_cHio2Dll.Exit();
	m_cHio2Dll.Release();

	InitInstance();
	m_bInitialized = FALSE;
}

//-----------------------------------------------------------------------------
void	CHio2If::AddDevicePath(HIO2DevicePath pathName)
{
#ifndef	HW0
	LPSTR lpPath = new TCHAR [lstrlen(pathName) + 1];
	lstrcpy(lpPath, pathName);
	m_cDevices.push_back(lpPath);
#else	// HW0
	m_cDevices.push_back(pathName);
#endif	// HW0
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::SetFatal(HIO2IF_ERROR errID, ...)
{
	va_list argptr;

	va_start(argptr, errID);
	vsprintf_s( m_szMessage, sizeof( m_szMessage ), (char*)m_lpszErrorStrings[errID], argptr );
//	vsprintf( m_szMessage, (char*)m_lpszErrorStrings[errID], argptr );
	va_end(argptr);
	m_nLastError = errID;

	return HIO2IF_RESULT_FATAL;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	CHio2If::SetError(HIO2IF_ERROR errID, ...)
{
	va_list argptr;

	va_start(argptr, errID);
	vsprintf_s( m_szMessage, sizeof( m_szMessage ),(char*)m_lpszErrorStrings[errID], argptr );
//	vsprintf( m_szMessage, (char*)m_lpszErrorStrings[errID], argptr );
	va_end(argptr);
	m_nLastError = errID;

	return HIO2IF_RESULT_ERROR;
}

//-----------------------------------------------------------------------------
void	CHio2If::InitInstance()
{
#ifndef	HW0	// HW0
	for (int i=0; i<(int)m_cDevices.size(); i++) delete [] m_cDevices[i];
	m_cDevices.clear();
#else		// HW0
	m_cDevices.clear();
#endif
	m_cHioStatus.Clear();
}

///////////////////////////////////////////////////////////////////////////////
//
// create an release Host I/O interface for PC
//

CHio2If*	Hio2CreateInterface(void)
{
	if ( l_pHio2If == NULL ) l_pHio2If = new CHio2If;

	return l_pHio2If;
}

void	Hio2ReleaseInterface(CHio2If* pHioIf)
{
	ASSERT(l_pHio2If == pHioIf);

	delete static_cast<CHio2If *>(pHioIf);
	l_pHio2If = NULL;
}

// end of HioIfHost.cpp
