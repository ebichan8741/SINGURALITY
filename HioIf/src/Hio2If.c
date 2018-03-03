/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2If.c

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/HioIf/src/Hio2If.c,v 1.2 2006/03/09 12:28:43 yasuh-to Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include <revolution.h>
#include "Hio2If.h"

//#define	HIO2IF_DEBUG

// device information
static s32	hio2DevCount = 0;
static HIO2DeviceType	hio2Devices[HIO2_CHAN_MAX + 1] =
{
	HIO2_DEVICE_INVALID,
	HIO2_DEVICE_INVALID,
	HIO2_DEVICE_INVALID
};

// error
static HIO2IF_ERROR	hio2LastError = HIO2IF_ERROR_NONE;
static char	hio2Message[128] = { '\0' };

// status
static HIO2IF_STATUS	hio2Status[HIO2_CHAN_MAX] = 
{
	{
		HIO2_DEVICE_INVALID,
		HIO2IF_INVALID_ID,
		HIO2_INVALID_HANDLE_VALUE,
		HIO2IF_MODE_NONE,
		FALSE,
		FALSE,
		FALSE,
		HIO2IF_ASYNC_NONE,
		NULL,
		0,
		NULL
	},
	{
		HIO2_DEVICE_INVALID,
		HIO2IF_INVALID_ID,
		HIO2_INVALID_HANDLE_VALUE,
		HIO2IF_MODE_NONE,
		FALSE,
		FALSE,
		FALSE,
		HIO2IF_ASYNC_NONE,
		NULL,
		0,
		NULL
	},
};

// packet command for Open
static const u8	hio2PacketCmd[] = 
{
	HIO2IF_CMD_OPEN_RDONLY,
	HIO2IF_CMD_OPEN_WRONLY,
	HIO2IF_CMD_OPEN_RDWR,
};

// initialize flag
static BOOL	hio2Initialized = FALSE;

// error strings
static const char*	hio2ErrorStrings[HIO2IF_ERROR_MAX] =
{

#include "Hio2IfErr.str"

};

///////////////////////////////////////////////////////////////////////////////
//
// inline function definition
//

// HIO2ハンドルからHIO2IF_IDを取得
static inline
s32	hio2GetIdOfHandle(HIO2Handle h)
{
	return (hio2Status[0].hHIO == h) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// callback definition
//

// HIO2EnumDevice() - callback
static
BOOL	hio2EnumCallback( HIO2DeviceType type )
{
#ifdef HIO2IF_DEBUG
	OSReport("Device = %d\n", type);
#endif
	hio2Devices[hio2DevCount++] = type;
	return TRUE;
}

// callback for receive mail
static
void	hio2ReceiveCallback( HIO2Handle h )
{
	s32 id;
	HIO2IF_EVENT event;
	u32 mail = 0;

	(void)HIO2ReadMailbox(h, &mail);
	id = hio2GetIdOfHandle(h);

	switch ( HIO2IF_GET_PACKET_CMD(mail) )
	{
	case HIO2IF_CMD_OPEN_RESULT:
		hio2Status[id].nPc		= HIO2IF_GET_PACKET_CHAN(mail);
		hio2Status[id].bConnect = TRUE;
		event = HIO2IF_EVENT_CONNECT;
		break;
	case HIO2IF_CMD_SEND:
		hio2Status[id].bReceived = TRUE;
		event = HIO2IF_EVENT_RECEIVED;
		break;
	case HIO2IF_CMD_SEND_RESULT:
		hio2Status[id].bSendPossible = TRUE;
		event = HIO2IF_EVENT_SEND_POSSIBLE;
		break;
	case HIO2IF_CMD_CLOSE:
		// 実際のクローズ処理は、アプリで行うこと
		hio2Status[id].bConnect = FALSE;
		event = HIO2IF_EVENT_DISCONNECT;
		break;
	default:
		event = HIO2IF_EVENT_UNKOWN;
	}

	// イベントコールバック関数呼び出し
	if ( hio2Status[id].fncCallback != NULL )
		hio2Status[id].fncCallback(id, event);
}

// callback for HIO2ReadAsync()
static
void	hio2ReadAsyncCallback( HIO2Handle h )
{
	s32 id = hio2GetIdOfHandle(h);
	u32 async = HIO2IF_ASYNC_READ_MASK(hio2Status[id].dwAsyncMode);

	DCInvalidateRange((void *)hio2Status[id].pReadAsyncPtr,
					  hio2Status[id].dwReadAsyncSize);

	// ReadによるAsync指定の場合、接続相手にRead完了通知を行う
	// 、ReadFreeは、通知不要
	if ( async & HIO2IF_ASYNC_READ )
	{
		(void)HIO2WriteMailbox(
			hio2Status[id].hHIO,
			HIO2IF_SET_PACKET(hio2Status[id].nType, HIO2IF_CMD_SEND_RESULT));
	}

	hio2Status[id].dwAsyncMode &= ~async;

	// イベントコールバック関数呼び出し
	if (hio2Status[id].fncCallback != NULL )
		hio2Status[id].fncCallback(id, HIO2IF_EVENT_READ_ASYNC_DONE);
}

// callback for HIO2WriteAsync()
static
void	hio2WriteAsyncCallback( HIO2Handle h )
{
	s32 id = hio2GetIdOfHandle(h);
	u32 async = HIO2IF_ASYNC_WRITE_MASK(hio2Status[id].dwAsyncMode);

	// WriteによるAsync指定の場合、接続相手にWrite通知を行う
	// 、WriteFreeは、通知不要）
	if ( async & HIO2IF_ASYNC_WRITE )
	{
		(void)HIO2WriteMailbox(
			hio2Status[id].hHIO,
			HIO2IF_SET_PACKET(hio2Status[id].nType, HIO2IF_CMD_SEND));
	}

	hio2Status[id].dwAsyncMode &= ~async;

	// イベントコールバック関数呼び出し
	if ( hio2Status[id].fncCallback != NULL )
		hio2Status[id].fncCallback(id, HIO2IF_EVENT_WRITE_ASYNC_DONE);
}

// callback for disconnect
static
void	hio2DisconnectCallback( HIO2Handle h )
{
	s32 id = hio2GetIdOfHandle(h);

	(void)sprintf(hio2Message, "INTERRUPT, EXI device(%d)",
				  hio2Status[id].nType);

	// イベントコールバック関数呼び出し
	if ( hio2Status[id].fncCallback != NULL )
		hio2Status[id].fncCallback(id, HIO2IF_EVENT_INTERRUPT);

	hio2Status[id].nType			= HIO2_DEVICE_INVALID;
	hio2Status[id].nPc				= HIO2IF_INVALID_ID;
	hio2Status[id].hHIO				= HIO2_INVALID_HANDLE_VALUE;
	hio2Status[id].nMode			= HIO2IF_MODE_NONE;
	hio2Status[id].bConnect			= FALSE;
	hio2Status[id].bReceived		= FALSE;
	hio2Status[id].bSendPossible	= FALSE;
	hio2Status[id].fncCallback		= NULL;
}

//-----------------------------------------------------------------------------
// local function definiton

static
HIO2IF_RESULT	hio2SetFatal( HIO2IF_ERROR errID, ... )
{
	va_list argptr;

	va_start(argptr, errID);
	(void)vsprintf(hio2Message, (char*)hio2ErrorStrings[errID], argptr);
	va_end(argptr);

	hio2LastError = errID;

	return HIO2IF_RESULT_FATAL;
}

static
HIO2IF_RESULT	hio2SetError( HIO2IF_ERROR errID, ... )
{
	va_list argptr;

	va_start(argptr, errID);
	(void)vsprintf(hio2Message, (char*)hio2ErrorStrings[errID], argptr);
	va_end(argptr);

	hio2LastError = errID;

	return HIO2IF_RESULT_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// Host I/O interface for NNGC
//

//-----------------------------------------------------------------------------
s32	HIO2IFGetDeviceCount( void )
{
	return hio2DevCount;
}

//-----------------------------------------------------------------------------
HIO2DeviceType	HIO2IFGetDevice( s32 index )
{
	ASSERT((index >= 0) && (index < (HIO2_CHAN_MAX + 1)));
	return hio2Devices[index];
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFInit( void )
{
	// 初期化済みの場合、HIO2を終了させる
	if ( hio2Initialized ) HIO2Exit();

	hio2DevCount = 0;
	hio2Devices[0] = hio2Devices[1] = 
	hio2Status[0].nType = hio2Status[1].nType = HIO2_DEVICE_INVALID;

	// HIO2初期化
	if ( !HIO2Init() )
		return hio2SetFatal(HIO2IF_FATAL_INIT, HIO2GetLastError());

	// EXIデバイスを検索
	if ( !HIO2EnumDevices(hio2EnumCallback) )
		return hio2SetFatal(HIO2IF_FATAL_ENUMDEVICES, HIO2GetLastError());

	hio2Initialized = TRUE;

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFOpen( HIO2DeviceType type, HIO2IF_MODE mode,
							HIO2IF_EVENT_CALLBACK callback, HIO2IF_ID* id )
{
	*id = HIO2IF_INVALID_ID;

	// EXIチャネルが検出されていない場合
	if ( type == HIO2_DEVICE_INVALID )
		return hio2SetError(HIO2IF_ERROR_CHAN_NOT_FIND, type);

	// 指定EXIチャネルが使用中の場合
	if ( (type == hio2Status[0].nType) || (type == hio2Status[1].nType) )
		return hio2SetError(HIO2IF_ERROR_CHAN_ALREADY_OPENED, type);

	// 空きIDの検索
	*id = (hio2Status[0].nType == HIO2IF_INVALID_ID) ? 0 : 1;

	hio2Status[*id].hHIO =
		HIO2Open(type, hio2ReceiveCallback, hio2DisconnectCallback);
	if ( hio2Status[*id].hHIO == HIO2_INVALID_HANDLE_VALUE )
		return hio2SetFatal(HIO2IF_FATAL_OPEN, type, HIO2GetLastError());

	hio2Status[*id].nType			= type;
	hio2Status[*id].nMode			= mode;
	hio2Status[*id].bConnect		= FALSE;
	hio2Status[*id].bReceived		= FALSE;
	hio2Status[*id].bSendPossible	= TRUE;
	hio2Status[*id].fncCallback		= callback;

	// オープン通知
	(void)HIO2WriteMailbox(hio2Status[*id].hHIO,
						   HIO2IF_SET_PACKET(type, hio2PacketCmd[mode]));

	return HIO2IF_RESULT_SUCCESS; 
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFRead( HIO2IF_ID id, u32 addr, void* buffer, s32 size,
							BOOL async )
{
	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	// Write only モードの場合
	if ( hio2Status[id].nMode == HIO2IF_MODE_WRONLY )
		return hio2SetError(HIO2IF_ERROR_WRITE_ONLY, hio2Status[id].nType);

	// PCと接続されていない
	if ( !hio2Status[id].bConnect )
		return hio2SetError(HIO2IF_ERROR_NOT_CONNECT, hio2Status[id].nType);

	// データを受信していない場合
	if ( !hio2Status[id].bReceived )
		return hio2SetError(HIO2IF_ERROR_NOT_RECV_DATA, hio2Status[id].nType);

	hio2Status[id].bReceived = FALSE;

	// 同期式Read
	if ( !async )
	{
		if ( !HIO2Read(hio2Status[id].hHIO, addr, buffer, size) )
			return hio2SetFatal(HIO2IF_FATAL_READ,
								hio2Status[id].nType, HIO2GetLastError());

		DCInvalidateRange(buffer, (u32)size);

		// 接続相手にRead完了通知を行う
		(void)HIO2WriteMailbox(
			hio2Status[id].hHIO,
			HIO2IF_SET_PACKET(hio2Status[id].nType, HIO2IF_CMD_SEND_RESULT));
	}
	// 非同期式Read
	else
	{
		hio2Status[id].dwAsyncMode		|= HIO2IF_ASYNC_READ;
		hio2Status[id].pReadAsyncPtr	= buffer;
		hio2Status[id].dwReadAsyncSize	= (u32)size;

		if ( !HIO2ReadAsync(hio2Status[id].hHIO, addr, buffer, size,
							hio2ReadAsyncCallback) )
			return hio2SetFatal(HIO2IF_FATAL_READ,
								hio2Status[id].nType, HIO2GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFReadFree( HIO2IF_ID id, u32 addr, void* buffer, s32 size,
								BOOL async )
{
	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	// Write only モードの場合
	if ( hio2Status[id].nMode == HIO2IF_MODE_WRONLY )
		return hio2SetError(HIO2IF_ERROR_WRITE_ONLY, hio2Status[id].nType);

	// PCと接続されていない
	if ( !hio2Status[id].bConnect )
		return hio2SetError(HIO2IF_ERROR_NOT_CONNECT, hio2Status[id].nType);

	// 同期式Read
	if ( !async )
	{
		if ( !HIO2Read(hio2Status[id].hHIO, addr, buffer, size) )
			return hio2SetFatal(HIO2IF_FATAL_READ,
								hio2Status[id].nType, HIO2GetLastError());
		
		DCInvalidateRange(buffer, (u32)size);
	}
	// 非同期式Read
	else
	{
		// 以前のReadAync or WriteAsyncが完了していない
		if  ( hio2Status[id].dwAsyncMode )
			return hio2SetError(HIO2IF_ERROR_BUSY, hio2Status[id].nType);

		hio2Status[id].dwAsyncMode		|= HIO2IF_ASYNC_READ_FREE;
		hio2Status[id].pReadAsyncPtr	= buffer;
		hio2Status[id].dwReadAsyncSize	= (u32)size;

		if ( !HIO2ReadAsync(hio2Status[id].hHIO, addr, buffer, size,
							hio2ReadAsyncCallback) )
			return hio2SetFatal(HIO2IF_FATAL_READ,
								hio2Status[id].nType, HIO2GetLastError());

	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFWrite( HIO2IF_ID id, u32 addr, void* buffer, s32 size,
							 BOOL async )
{
	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	// Read only モードの場合
	if ( hio2Status[id].nMode == HIO2IF_MODE_RDONLY )
		return hio2SetError(HIO2IF_ERROR_READ_ONLY, hio2Status[id].nType);

	// PCと接続されていない
	if ( !hio2Status[id].bConnect )
		return hio2SetError(HIO2IF_ERROR_NOT_CONNECT, hio2Status[id].nType);

	// 送信できない場合（相手先がデータを受信できない場合）
	if ( !hio2Status[id].bSendPossible )
	{
		return hio2SetError(HIO2IF_ERROR_CANNOT_SEND_DATA, 
							hio2Status[id].nType);
	}

	hio2Status[id].bSendPossible = FALSE;

	DCFlushRange(buffer, (u32)size);

	// 同期式Write
	if ( !async )
	{
		if ( !HIO2Write(hio2Status[id].hHIO, addr, buffer, size) )
			return hio2SetFatal(HIO2IF_FATAL_WRITE,
								hio2Status[id].nType, HIO2GetLastError());
	
		// 接続相手にWrite通知を行う
		(void)HIO2WriteMailbox(
			hio2Status[id].hHIO,
			HIO2IF_SET_PACKET(hio2Status[id].nType, HIO2IF_CMD_SEND));
	}
	// 非同期式Write
	else
	{
		hio2Status[id].dwAsyncMode |= HIO2IF_ASYNC_WRITE;
		if ( !HIO2WriteAsync(hio2Status[id].hHIO, addr, buffer, size,
							 hio2WriteAsyncCallback) )
			return hio2SetFatal(HIO2IF_FATAL_WRITE,
								hio2Status[id].nType, HIO2GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFWriteFree( HIO2IF_ID id, u32 addr, void* buffer,
								 s32 size, BOOL async )
{
	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	// Read only モードの場合
	if ( hio2Status[id].nMode == HIO2IF_MODE_RDONLY )
		return hio2SetError(HIO2IF_ERROR_READ_ONLY, hio2Status[id].nType);

	// PCと接続されていない
	if ( !hio2Status[id].bConnect )
		return hio2SetError(HIO2IF_ERROR_NOT_CONNECT, hio2Status[id].nType);

	DCFlushRange(buffer, (u32)size);

	// 同期式Write
	if ( !async )
	{
		if ( !HIO2Write(hio2Status[id].hHIO, addr, buffer, size) )
			return hio2SetFatal(HIO2IF_FATAL_WRITE,
								hio2Status[id].nType, HIO2GetLastError());
	}
	// 非同期式Write
	else
	{
		// 以前のReadAync or WriteAsyncが完了していない
		if  ( hio2Status[id].dwAsyncMode )
			return hio2SetError(HIO2IF_ERROR_BUSY, hio2Status[id].nType);

		hio2Status[id].dwAsyncMode |= HIO2IF_ASYNC_WRITE_FREE;
		if ( !HIO2WriteAsync(hio2Status[id].hHIO, addr, buffer, size,
							 hio2WriteAsyncCallback) )
			return hio2SetFatal(HIO2IF_FATAL_WRITE,
								hio2Status[id].nType, HIO2GetLastError());
	}

	return HIO2IF_RESULT_SUCCESS;
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFReadStatus( HIO2IF_ID id, u32* status )
{
	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	// PCと接続されていない
	if ( !hio2Status[id].bConnect )
		return hio2SetError(HIO2IF_ERROR_NOT_CONNECT, hio2Status[id].nType);

	return HIO2ReadStatus(hio2Status[id].hHIO, status)
		? HIO2IF_RESULT_SUCCESS
		: hio2SetFatal(HIO2IF_FATAL_READSTATUS, hio2Status[id].nType,
					   HIO2GetLastError());
}

//-----------------------------------------------------------------------------
HIO2IF_RESULT	HIO2IFClose( HIO2IF_ID id )
{
	BOOL result;
	s32 chan;

	if ( id == HIO2IF_INVALID_ID ) return hio2SetError(HIO2IF_ERROR_INVALID_ID);
	chan = hio2Status[id].nType;

	// 相手と接続している場合、クローズ通知を行う
	if ( hio2Status[id].bConnect )
	{
		(void)HIO2WriteMailbox(hio2Status[id].hHIO,
							   HIO2IF_SET_PACKET(chan, HIO2IF_CMD_CLOSE));
	}
	
	result = HIO2Close(hio2Status[id].hHIO);

	hio2Status[id].nType			= (HIO2DeviceType)HIO2IF_INVALID_ID;
	hio2Status[id].nPc				= HIO2IF_INVALID_ID;
	hio2Status[id].hHIO				= HIO2_INVALID_HANDLE_VALUE;
	hio2Status[id].nMode			= HIO2IF_MODE_NONE;
	hio2Status[id].bConnect			= FALSE;
	hio2Status[id].bReceived		= FALSE;
	hio2Status[id].bSendPossible	= FALSE;
	hio2Status[id].fncCallback		= NULL;

	return result ? HIO2IF_RESULT_SUCCESS
		: hio2SetFatal(HIO2IF_FATAL_CLOSE, chan, HIO2GetLastError());
}

//-----------------------------------------------------------------------------
void	HIO2IFSync( void )
{
	// EXIチャネルがオープンされていて、PCと接続できていない場合、オープン通知
	// を行う
	if ( (hio2Status[0].hHIO != HIO2_INVALID_HANDLE_VALUE)
		 && (!hio2Status[0].bConnect) )
	{
		(void)HIO2WriteMailbox(
			hio2Status[0].hHIO,
			HIO2IF_SET_PACKET(hio2Status[0].nType,
							 hio2PacketCmd[hio2Status[0].nMode]));
	}

	if ( (hio2Status[1].hHIO != HIO2_INVALID_HANDLE_VALUE)
		 && (!hio2Status[1].bConnect) )
	{
		// オープン通知
		(void)HIO2WriteMailbox(
			hio2Status[1].hHIO,
			HIO2IF_SET_PACKET(hio2Status[1].nType,
							  hio2PacketCmd[hio2Status[1].nMode]));
	}
}

//-----------------------------------------------------------------------------
void	HIO2IFExit( void )
{
	// EXIチャネルがオープンされていて、PCと接続している場合、クローズ通知を行う
	if ( (hio2Status[0].hHIO != HIO2_INVALID_HANDLE_VALUE)
		 && (hio2Status[0].bConnect) )
	{
		(void)HIO2IFClose(0);
	}

	if ( (hio2Status[1].hHIO != HIO2_INVALID_HANDLE_VALUE)
		 && (hio2Status[1].bConnect) )
	{
		(void)HIO2IFClose(1);
	}

	// HIO2Exit()は、オープン済みチャネルを強制クローズする
	HIO2Exit();

	hio2Initialized = FALSE;
}

//-----------------------------------------------------------------------------
BOOL	HIO2IFIsConnected( HIO2IF_ID id )
{
	return  (id != HIO2IF_INVALID_ID) ? hio2Status[id].bConnect : FALSE;
}

//-----------------------------------------------------------------------------
BOOL	HIO2IFIsReceived( HIO2IF_ID id )
{
	return  (id != HIO2IF_INVALID_ID) ? hio2Status[id].bReceived : FALSE;
}

//-----------------------------------------------------------------------------
BOOL	HIO2IFIsSendPossible( HIO2IF_ID id )
{
	return  (id != HIO2IF_INVALID_ID) ? hio2Status[id].bSendPossible : FALSE;
}

//-----------------------------------------------------------------------------
HIO2DeviceType	HIO2IFGetDeviceType( HIO2IF_ID id )
{
	return (id != HIO2IF_INVALID_ID)
		? hio2Status[id].nType : HIO2_DEVICE_INVALID;
}

//-----------------------------------------------------------------------------
s32	HIO2IFGetPcChan( HIO2IF_ID id )
{
	return (id != HIO2IF_INVALID_ID) ? hio2Status[id].nPc : -1;
}

//-----------------------------------------------------------------------------
HIO2IF_ERROR	HIO2IFGetLastError( void )
{
	return hio2LastError;
}

//-----------------------------------------------------------------------------
const char*		HIO2IFGetErrorMessage( void )
{
	return hio2Message;
}

// end of Hio2If.c
