/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multi-main.c

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/src/multi-main.c,v 1.3 2007/11/26 13:54:44 iwai_yuma Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include <string.h>
#include <demo.h>
#include "Hio2If.h"
#include "multi.h"

//-----------------------------------------------------------------------------
// define

// DEMORFPrintf()�p���W
#define	POS_COMM_X		32
#define	POS_TITLE_Y		64
#define	POS_SENDDATA_Y	128
#define	POS_RECVDATA_Y	192
#define	POS_CHAN_Y		384
#define	POS_STAT_X		512
#define	POS_STAT_Y		96
#define	POS_V_Y			416
#define	POS_CONNECT_X	200
#define	POS_CONNECT_Y	416
#define	POS_SYNC_X		POS_STAT_X
#define	POS_SYNC_Y		POS_CONNECT_Y

//-----------------------------------------------------------------------------
// type definition

// PROTOCOL_USED�̎w��L���ɂ��AHIO2IF API���Ăяo���ׂ̌^
typedef HIO2IF_RESULT	(* MYREAD)(HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async);
typedef HIO2IF_RESULT	(* MYWRITE)(HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async);

//-----------------------------------------------------------------------------
// local symbol definiton

// Host I/O API�p�o�b�t�@
static MULTI_PACKET	sendBuffer ATTRIBUTE_ALIGN(32);
static MULTI_PACKET	recvBuffer ATTRIBUTE_ALIGN(32);

// �J���[�f�[�^
static const GXColor gxFont = { 0xFF, 0xFF, 0xFF, 0 }, gxBg = { 0, 0, 0, 0 };

// HIO2IF ID
static HIO2IF_ID	hioID = HIO2IF_INVALID_ID;

// �������A�񓯊�������
static BOOL	hioAsync = FALSE;

// �ڑ��֘A���
volatile const char*	pStatus = "NOT FIND";
#ifdef	PROTOCOL_USED
volatile BOOL	bReceived = FALSE;
volatile BOOL	bSendPossible = TRUE;
#else	// PROTOCOL_USED
#define	bReceived		TRUE
#define	bSendPossible	TRUE
#endif	// PROTOCOL_USED

// V�J�E���^
static u32 dwVCounter = 0;

#ifdef	PROTOCOL_USED
static const char*	strProtocol = "protocol used";
#else	// PROTOCOL_USED
static const char*	strProtocol = "non protocol";
#endif	// PROTOCOL_USED

static char	strBuffer[128];

//-----------------------------------------------------------------------------
// local function definiton

static void	myAppInit(void);
static void	myHioInit(void);
static void	myHioExit(void);
static void	myDispInfo(void);

static inline
void	myHalt(void)
{
	OSFatal(gxFont, gxBg, HIO2IFGetErrorMessage());
}

///////////////////////////////////////////////////////////////////////////////
//
// event callback
//
static 
void	myEventCallback(HIO2IF_ID id, HIO2IF_EVENT event)
{
	switch ( event )
	{
	case HIO2IF_EVENT_CONNECT:		// �ڑ��m��
#ifdef HIO2IF_DEBUG
		OSReport("EVENT CONNECT : id(%d)\n", id);
#endif
		pStatus = "CONNECT";
		break;
	case HIO2IF_EVENT_DISCONNECT:	// �ڑ�����
#ifdef HIO2IF_DEBUG
		OSReport("EVENT DISCONNECT : id(%d)", id);
#endif
		pStatus = "DISCONNECT";
		(void)HIO2IFClose(id);
		break;
#ifdef	PROTOCOL_USED
	case HIO2IF_EVENT_RECEIVED:		// �f�[�^��M
		bReceived = TRUE;
		break;
	case HIO2IF_EVENT_SEND_POSSIBLE:	// ���M�\���
		bSendPossible = TRUE;
		break;
#endif	// PROTOCOL_USED
	case HIO2IF_EVENT_READ_ASYNC_DONE:
	case HIO2IF_EVENT_WRITE_ASYNC_DONE:
		break;
	case HIO2IF_EVENT_INTERRUPT:
		myHalt();
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// double-main function
//

//-----------------------------------------------------------------------------
void	main(void)
{
	MYREAD	hioIfRead;
	MYWRITE	hioIfWrite;

	// ������
	myAppInit();

#ifdef	PROTOCOL_USED
	hioIfRead	= HIO2IFRead;
	hioIfWrite	= HIO2IFWrite;
#else	// PROTOCOL_USED
	hioIfRead	= HIO2IFReadFree;
	hioIfWrite	= HIO2IFWriteFree;
#endif	// PROTOCOL_USED

	// Host I/O �̏�����
	myHioInit();

	while ( 1 )
	{
		OSCalendarTime calender;
		HIO2IF_RESULT result;
		u32 stat = 0;

		// �ʐM��Ԃ�\��
		(void)HIO2IFReadStatus(hioID, &stat);
		(void)DEMORFPrintf(POS_STAT_X, POS_STAT_Y, 0, "ST:%04X", stat);

		// �J�����_�[���͌��ʂ�PC���M�p�o�b�t�@�ɐݒ�
		OSTicksToCalendarTime(OSGetTime(), &calender);
		sendBuffer.mon	= (u8)(calender.mon + 1);
		sendBuffer.mday	= (u8)calender.mday;
		sendBuffer.hour	= (u8)calender.hour;
		sendBuffer.min	= (u8)calender.min;
		sendBuffer.sec	= (u8)calender.sec;
		sendBuffer.msec	= (u16)calender.msec;

		// �R���g���[������
		DEMOPadRead();

		// A�{�^���������ꂽ���A�ڑ�
		// B�{�^���������ꂽ���A�ڑ�����
		// X�{�^���������ꂽ���ASYNC <-> ASYNC �؂�ւ�
		if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_A ) myHioInit();
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_B ) myHioExit();
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_X ) hioAsync=!hioAsync;

		// PC�Ɛڑ�������Ԃ̎��A��M�������s��
		if ( bReceived && HIO2IFIsConnected(hioID) )
		{
			// PC����̎�M����
			result = hioIfRead(hioID, MULTI_PC2NNGC_ADDR,
							   &recvBuffer, MULTI_BUFFER_SIZE, FALSE);
			if ( HIO2IF_FAILED(result) ) myHalt();
#ifdef	PROTOCOL_USED
			bReceived = FALSE;
#endif	// PROTOCOL_USED
		}

		// Y�{�^���������ꂽ�� & PC�Ɛڑ�������Ԃ̎��A���݂̃J�����_�[���
		// ��PC�֑��M
		if ( (DEMOPadGetButtonDown(0) & PAD_BUTTON_Y)
			 && bSendPossible && HIO2IFIsConnected(hioID) )
		{
			result = hioIfWrite(hioID, MULTI_NNGC2PC_ADDR,
								&sendBuffer, MULTI_BUFFER_SIZE, hioAsync);
			if ( HIO2IF_FAILED(result) ) myHalt();
#ifdef	PROTOCOL_USED
			bSendPossible = FALSE;
#endif	// PROTOCOL_USED
		}

		DEMOBeforeRender();

		// �\�����X�V
		myDispInfo();

		DEMODoneRender();

		dwVCounter++;

		// Sync
		HIO2IFSync();
	}

}

//----------------------------------------------------------------------------
static
void	myAppInit(void)
{
    GXRenderModeObj* rmp;

	DEMOInit(NULL);

	// DEMORFPrintf()�p������
	(void)DEMOInitROMFont();

    rmp = DEMOGetRenderModeObj();
    DEMOInitCaption(DM_FT_OPQ, (s16) rmp->fbWidth, (s16) rmp->efbHeight);
    DEMOSetFontType(DM_FT_OPQ);

	// �X�N���[��������
    GXSetCopyClear(gxBg, 0x00ffffff);
    GXCopyDisp(DEMOGetCurrentBuffer(), GX_TRUE);

	// �R���g���[��������
	DEMOPadInit();

	// Host I/O API�p�o�b�t�@�̏�����
	(void)memset(&sendBuffer, 0, sizeof(sendBuffer));
	(void)memset(&recvBuffer, 0, sizeof(recvBuffer));
	(void)strcpy(sendBuffer.string,"NNGC TIME");
}

//----------------------------------------------------------------------------
static
void	myHioInit(void)
{
	if ( HIO2IFIsConnected(hioID) ) return ;

	// Host I/O �̏�����
	if ( HIO2IF_FAILED(HIO2IFInit()) ) myHalt();

	// �`���l���I�[�v��
	if ( HIO2IFGetDeviceCount() > 0 )
	{
		if ( HIO2IF_FAILED(HIO2IFOpen(
							   HIO2IFGetDevice(0),
							   HIO2IF_MODE_RDWR,
							   myEventCallback,
							   &hioID)) )
			myHalt();
		pStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bReceived = FALSE;
		bSendPossible = TRUE;
#endif
	}
}

//----------------------------------------------------------------------------
static
void	myHioExit(void)
{
	if ( HIO2IFIsConnected(hioID) )
	{
		(void)HIO2IFClose(hioID);
		pStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bReceived = FALSE;
		bSendPossible = FALSE;
#endif
	}
}

//----------------------------------------------------------------------------
static
void	myDispInfo(void)
{
	static const char* devName[HIO2_CHAN_MAX + 2] =
		{ "UNKOWN", "EXI2USB0", "EXI2USB1", "MrEXI" };

	// �^�C�g��
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y,  0, "HIO2 DEMO - multi(%s)",
					   strProtocol);
	// ���M���
	MultiPacketToString(strBuffer, &sendBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_SENDDATA_Y, 0, "%s", strBuffer);

	// ��M���
	MultiPacketToString(strBuffer, &recvBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_RECVDATA_Y, 0, "%s", strBuffer);

	// �`���l����
	(void)DEMORFPrintf(POS_COMM_X, POS_CHAN_Y, 0,
					   "CHAN : %s, NNGC %s, PC%d",
					   pStatus, devName[HIO2IFGetDeviceType(hioID) + 1],
					   HIO2IFGetPcChan(hioID));

	// V�J�E���^�[
	(void)DEMORFPrintf(POS_COMM_X, POS_V_Y, 0, "V=%ld", dwVCounter);

	// �ڑ����
	(void)DEMORFPrintf(POS_CONNECT_X, POS_CONNECT_Y, 0,
					   HIO2IFIsConnected(hioID)
					   ? "PUSH B DISCONNECT" : "PUSH A CONNECT");

	// SYNC/ASYNC�̏��
	(void)DEMORFPrintf(POS_SYNC_X, POS_SYNC_Y, 0, hioAsync ? "ASYNC" : "SYNC");
}

// end of multi-main.c
