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
#include "multi.h"	// ���M�f�[�^�̐����ɗ��p
#include "simple.h"

//-----------------------------------------------------------------------------
// define

// DEMORFPrintf()�p���W
#define	POS_COMM_X		32
#define	POS_TITLE_Y		64
#define	POS_SEND_Y		96
#define	POS_SENDDATA_Y	128
#define	POS_RECVDATA_Y	192
#define	POS_MES_Y		416
#define	POS_CONNECT_X	480

//-----------------------------------------------------------------------------
// local symbol definiton

// HIO2 API�p�n���h��
static HIO2Handle	hHIO2;

// HIO2 API�p�o�b�t�@
static MULTI_PACKET	sendBuffer ATTRIBUTE_ALIGN(32);
static MULTI_PACKET	recvBuffer ATTRIBUTE_ALIGN(32);

// �J���[�f�[�^
static const GXColor gxFont = { 0xFF, 0xFF, 0xFF, 0 }, gxBg = { 0, 0, 0, 0 };

// �G���[���b�Z�[�W
static char	strError[128] = { '\0' };

// EXI�f�o�C�X
static 	HIO2DeviceType exiDev = HIO2_DEVICE_INVALID;

// PC�Ƃ̐ڑ���
static BOOL	bConnect = FALSE;

// �I���t���O
static BOOL	bExit = FALSE;

// ���M�t���O
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
	// �ŏ��Ɍ��o�����f�o�C�X���g�p
	exiDev = type;
	return FALSE;
}

// receive : 
//
// PC����̃��[���{�b�N�X�������݂����荞�݂Ō��m����ꍇ�́A�R�[���o�b�N�֐�
// �Ƃ��Ďg�p
// 
// PC����̃��[���{�b�N�X�������݂��X�e�[�^�X�Ō��m����ꍇ�́A�T�u���[�`��
// �Ƃ��Ďg�p
//
static
void	myReceiveCallback( HIO2Handle h )
{
	u32 mail;

	if ( !HIO2ReadMailbox(h, &mail) ) myHalt(SIMPLE_ERR_HIO2_READ_MAILBOX);

	switch ( mail )
	{
	case SIMPLE_MAIL_OPEN_RESULT:	// �ڑ����肩��I�[�v�������ʒm
		bConnect = TRUE;
		break;
	case SIMPLE_MAIL_RECV:	// �ڑ����肩��f�[�^����M
		if ( !HIO2Read(h, SIMPLE_PC2NNGC_ADDR, &recvBuffer,
					   sizeof(recvBuffer)) )
			myHalt(SIMPLE_ERR_HIO2_READ);
		DCInvalidateRange(&recvBuffer, sizeof(recvBuffer));
		break;
	case SIMPLE_MAIL_EXIT:	// �ڑ����肩��I���ʒm
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

	// ������
	myAppInit();

	// HIO2 API�̏�����
	myHIO2Init();

	while ( !bExit )
	{
		OSCalendarTime calender;
		BOOL sendPossible = FALSE;

		// �\���X�V
		DEMOBeforeRender();
		myUpdateDisp();
		DEMODoneRender();	

		// �R���g���[������
		DEMOPadRead();

		// �J�����_�[���͌��ʂ�PC���M�p�o�b�t�@�ɐݒ�
		OSTicksToCalendarTime(OSGetTime(), &calender);
		sendBuffer.mon	= (u8)(calender.mon + 1);
		sendBuffer.mday	= (u8)calender.mday;
		sendBuffer.hour	= (u8)calender.hour;
		sendBuffer.min	= (u8)calender.min;
		sendBuffer.sec	= (u8)calender.sec;
		sendBuffer.msec	= (u16)calender.msec;
		{
			u32 status;
			// HIO2�X�e�[�^�X���擾
			if ( !HIO2ReadStatus(hHIO2, &status) )
				myHalt(SIMPLE_ERR_HIO2_READ_STATUS);
			sendPossible = !(status & HIO2_STATUS_RX);
#ifdef	POLLING
			if ( status & HIO2_STATUS_TX )
				myReceiveCallback(hHIO2);
#endif	// POLLING
		}

		// ���[�����M�s��Ԃ̎��́A�������Ȃ�
		if ( !sendPossible ) continue;

		// PC�Ɩ��ڑ� & ���[�����M�\��Ԃ̎��APC�ɐڑ��ʒm���s��
		if ( !bConnect )
		{
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_OPEN) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
			continue;
		}

		// A�{�^���������A���M�E��M��؂�ւ�
		if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_A )
		{
			bSend = !bSend;
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_CHANGE) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
		}
		// SATRT�{�^���������APC�ɏI���ʒm���s���ďI��
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_START )
		{
			if ( !HIO2WriteMailbox(hHIO2, SIMPLE_MAIL_EXIT) )
				myHalt(SIMPLE_ERR_HIO2_WRITE_MAILBOX);
				bExit = TRUE;
		}
		// ���M��Ԃ̎��APC�Ƀf�[�^�𑗐M�i60V�P�ʁj
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

	// HIO2�̏I��
	myHIO2Exit();
}

//----------------------------------------------------------------------------
static
void	myAppInit( void )
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

	(void)strcpy(sendBuffer.string, "NDEV TIME");
	(void)strcpy(recvBuffer.string, "PC TIME");
}

//----------------------------------------------------------------------------
static
void	myUpdateDisp( void )
{
	char dispBuf[128];

	// �^�C�g��
#ifndef	POLLING
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y, 0,
					   "HIO2 DEMO - simple (EXI INT)");
#else	// POLLING
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y, 0,
					   "HIO2 DEMO - simple (polling)");
#endif	// POLLING

	// �ڑ����
	(void)DEMORFPrintf(POS_CONNECT_X, POS_TITLE_Y, 0,
					   bConnect ? "CONNECT" : "DISCONNECT");

	// ���M�E��M���
	(void)DEMORFPrintf(POS_CONNECT_X, POS_SEND_Y, 0, bSend ? "SEND" : "RECV");

	// ���M���
	MultiPacketToString(dispBuf, &sendBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_SENDDATA_Y, 0, dispBuf);

	// ��M���
	MultiPacketToString(dispBuf, &recvBuffer);
	(void)DEMORFPrintf(POS_COMM_X, POS_RECVDATA_Y, 0, "%s", dispBuf);
	
	// ���b�Z�[�W
	(void)DEMORFPrintf(POS_COMM_X, POS_MES_Y, 0,
					   "push START button to exit");
}

//----------------------------------------------------------------------------
static
void	myHIO2Init( void )
{
	// ������
	if ( !HIO2Init() ) myHalt(SIMPLE_ERR_HIO2_INIT);

	// �f�o�C�X�̗�
	if ( !HIO2EnumDevices(myEnumCallback) ) myHalt(SIMPLE_ERR_HIO2_ENUMDEVICES);

	// �f�o�C�X��������Ȃ������ꍇ
	if ( exiDev == HIO2_DEVICE_INVALID ) myHalt(SIMPLE_ERR_EXI2USB_NOT_FIND);

#ifndef	POLLING
	// �f�o�C�X�I�[�v��
	if ( (hHIO2=HIO2Open(exiDev, myReceiveCallback, myDisconnectCallback))
		== HIO2_INVALID_HANDLE_VALUE )
		myHalt(SIMPLE_ERR_HIO2_OPEN);
#else	// POLLING
	// �f�o�C�X�I�[�v��
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
