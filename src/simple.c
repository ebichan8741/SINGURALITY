/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - simple
  File:     simple.c

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/src/simple.c,v 1.4 2007/11/26 13:54:44 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include <string.h>
#include <demo.h>
#include <revolution/hio2.h>
#include "multi.h"	// 送信データの生成に流用
#include "simple.h"

//-----------------------------------------------------------------------------
// define

// DEMORFPrintf()用座標
#define	POS_COMM_X		32
#define	POS_TITLE_Y		64
#define	POS_SEND_Y		96
#define	POS_SENDDATA_Y	128
#define	POS_RECVDATA_Y	192
#define	POS_MES_Y		416
#define	POS_CONNECT_X	480

//-----------------------------------------------------------------------------
// local symbol definiton

// HIO2 API用ハンドル
static HIO2Handle	hHIO2;

// HIO2 API用バッファ
static MULTI_PACKET	sendBuffer ATTRIBUTE_ALIGN(32);
static MULTI_PACKET	recvBuffer ATTRIBUTE_ALIGN(32);

// カラーデータ
static const GXColor gxFont = { 0xFF, 0xFF, 0xFF, 0 }, gxBg = { 0, 0, 0, 0 };

// エラーメッセージ
static char	strError[128] = { '\0' };

// EXIデバイス
static 	HIO2DeviceType exiDev = HIO2_DEVICE_INVALID;

// PCとの接続状況
static BOOL	bConnect = FALSE;

// 終了フラグ
static BOOL	bExit = FALSE;

// 送信フラグ
static BOOL	bSend = TRUE;

//-----------------------------------------------------------------------------
// local function declared

static void	myAppInit( void );
static void	myUpdateDisp( void );
static void	myHIO2Init( void );
static void	myHIO2Exit( void );

static inline
void	myHalt( SIMPLE_ERROR errno )
{
	(void)sprintf(strError, simpleErrorMessage[errno], HIO2GetLastError());

        OSFatal(gxFont, gxBg, strError);
}

///////////////////////////////////////////////////////////////////////////////
//
// callback
//

// enum devices
static
BOOL	myEnumCallback( HIO2DeviceType type )
{
	// 最初に検出したデバイスを使用
	exiDev = type;
	return FALSE;
}

// receive : 
//
// PCからのメールボックス書き込みを割り込みで検知する場合は、コールバック関数
// として使用
// 
// PCからのメールボックス書き込みをステータスで検知する場合は、サブルーチン
// として使用
//
static
void	myReceiveCallback( HIO2Handle h )
{
	u32 mail;

	if ( !HIO2ReadMailbox(h, &mail) ) myHalt(SIMPLE_ERR_HIO2_READ_MAILBOX);

	switch ( mail )
	{
	case SIMPLE_MAIL_OPEN_RESULT:	// 接続相手からオープン完了通知
		bConnect = TRUE;
		break;
	case SIMPLE_MAIL_RECV:	// 接続相手からデータを受信
		if ( !HIO2Read(h, SIMPLE_PC2NNGC_ADDR, &recvBuffer,
					   sizeof(recvBuffer)) )
			myHalt(SIMPLE_ERR_HIO2_READ);
		DCInvalidateRange(&recvBuffer, sizeof(recvBuffer));
		break;
	case SIMPLE_MAIL_EXIT:	// 接続相手から終了通知
		bExit = TRUE; break;
	}
}

// disconnect
static
void	myDisconnectCallback( HIO2Handle h )
{
#pragma unused(h)
	myHalt(SIMPLE_ERR_EXI2USB_DISCONNECT);
}

///////////////////////////////////////////////////////////////////////////////
//
// simple function
//

//-----------------------------------------------------------------------------
void	main( void )
{
	int v = 0;

	// 初期化
	myAppInit();

	// HIO2 APIの初期化
	myHIO2Init();

	while ( !bExit )
	{
		OSCalendarTime calender;
		BOOL sendPossible = FALSE;

		// 表示更新
		DEMOBeforeRender();
		myUpdateDisp();
		DEMODoneRender();	

		// コントローラ入力
		DEMOPadRead();

		// カレンダー入力結果をPC送信用バッファに設定
		OSTicksToCalendarTime(OSGetTime(), &calender);
		sendBuffer.mon	= (u8)(calender.mon + 1);
		sendBuffer.mday	= (u8)calender.mday;
		sendBuffer.hour	= (u8)calender.hour;
		sendBuffer.min	= (u8)calender.min;
		sendBuffer.sec	= (u8)calender.sec;
		sendBuffer.msec	= (u16)calender.msec;
		{
			u32 status;
			// HIO2ステータスを取得
			if ( !HIO2ReadStatus(hHIO2, &status) )
				myHalt(SIMPLE_ERR_HIO2_READ_STATUS);
			sendPossible = !(status & HIO2_STATUS_RX);
#ifdef	POLLING
			if ( status & HIO2_STATUS_TX )
				myReceiveCallback(hHIO2);
#endif	// POLLING
		}

		// メール送信不可状態の時は、何もしない
		if ( !sendPossible ) continue;

		// PCと未接続 & メール送信可能状態の時、PCに接続通知を行う
		if ( !bConnect )
		{
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_OPEN) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
			continue;
		}

		// Aボタン押下時、送信・受信を切り替え
		if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_A )
		{
			bSend = !bSend;
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_CHANGE) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
		}
		// SATRTボタン押下時、PCに終了通知を行って終了
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_START )
		{
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_EXIT) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
				bExit = TRUE;
		}
		// 送信状態の時、PCにデータを送信（60V単位）
		else if ( bSend && (++v == 60) )
		{
			DCFlushRange(&sendBuffer, sizeof(sendBuffer));
			if ( !HIO2Write(hHIO2, SIMPLE_NNGC2PC_ADDR,	&sendBuffer,
							sizeof(sendBuffer)) )
				myHalt(SIMPLE_ERR_HIO2_WRITE);
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_RECV) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
			v = 0;
		}
	}

	// HIO2の終了
	myHIO2Exit();
}

//----------------------------------------------------------------------------
static
void	myAppInit( void )
{
    GXRenderModeObj* rmp;

	DEMOInit(NULL);

	// DEMORFPrintf()用初期化
	(void)DEMOInitROMFont();

    rmp = DEMOGetRenderModeObj();
    DEMOInitCaption(DM_FT_OPQ, (s16) rmp->fbWidth, (s16) rmp->efbHeight);
    DEMOSetFontType(DM_FT_OPQ);

	// スクリーン初期化
    GXSetCopyClear(gxBg, 0x00ffffff);
    GXCopyDisp(DEMOGetCurrentBuffer(), GX_TRUE);

	// コントローラ初期化
	DEMOPadInit();

	(void)strcpy(sendBuffer.string, "NDEV TIME");
	(void)strcpy(recvBuffer.string, "PC TIME");
}

//----------------------------------------------------------------------------
static
void	myUpdateDisp( void )
{
	char dispBuf[128];

	// タイトル
#ifndef	POLLING
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y, 0,
					   "HIO2 DEMO - simple (EXI INT)");
#else	// POLLING
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y, 0,
					   "HIO2 DEMO - simple (polling)");
#endif	// POLLING

	// 接続状態
	(void)DEMORFPrintf(POS_CONNECT_X, POS_TITLE_Y, 0,
					   bConnect ? "CONNECT" : "DISCONNECT");

	// 送信・受信状態
	(void)DEMORFPrintf(POS_CONNECT_X, POS_SEND_Y, 0, bSend ? "SEND" : "RECV");

	// 送信情報
	MultiPacketToString(dispBuf, &sendBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_SENDDATA_Y, 0, dispBuf);

	// 受信情報
	MultiPacketToString(dispBuf, &recvBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_RECVDATA_Y, 0, "%s", dispBuf);
	
	// メッセージ
	(void)DEMORFPrintf(POS_COMM_X, POS_MES_Y, 0,
					   "push START button to exit");
}

//----------------------------------------------------------------------------
static
void	myHIO2Init( void )
{
	// 初期化
	if ( !HIO2Init() ) myHalt(SIMPLE_ERR_HIO2_INIT);

	// デバイスの列挙
	if ( !HIO2EnumDevices(myEnumCallback) ) myHalt(SIMPLE_ERR_HIO2_ENUMDEVICES);

	// デバイスが見つからなかった場合
	if ( exiDev == HIO2_DEVICE_INVALID ) myHalt(SIMPLE_ERR_EXI2USB_NOT_FIND);

#ifndef	POLLING
	// デバイスオープン
	if ( (hHIO2=HIO2Open(exiDev, myReceiveCallback, myDisconnectCallback))
		== HIO2_INVALID_HANDLE_VALUE )
		myHalt(SIMPLE_ERR_HIO2_OPEN);
#else	// POLLING
	// デバイスオープン
	if ( (hHIO2=HIO2Open(exiDev, NULL, myDisconnectCallback))
		== HIO2_INVALID_HANDLE_VALUE )
		myHalt(SIMPLE_ERR_HIO2_OPEN);
#endif	// POLLING
}

//----------------------------------------------------------------------------
static
void	myHIO2Exit( void )
{
	(void)HIO2Close(hHIO2);
	HIO2Exit();

	DEMOBeforeRender();

	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y, 0, "HIO2 DEMO - simple done.");

	DEMODoneRender();
}

// end of simple.c
