/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - dual
  File:     dual-main.c

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/src/dual-main.c,v 1.3 2006/08/16 08:13:07 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include <demo.h>
#include "Hio2If.h"
#include "dual.h"

//-----------------------------------------------------------------------------
// define

// DEMORFPrintf()�p���W
#define	POS_COMM_X		32
#define	POS_COL_X		48
#define	POS_TITLE_Y		64
#define	POS_R_Y			96	
#define	POS_G_Y			128
#define	POS_B_Y			160
#define	POS_RECV_Y		352
#define	POS_SEND_Y		384
#define	POS_V_Y			416
#define	POS_CONNECT_X	200
#define	POS_CONNECT_Y	416
#define	POS_STAT_X		512
#define	POS_RECV_STAT_Y	POS_R_Y
#define	POS_SEND_STAT_Y	POS_G_Y

#define	POS_SYNC_X		POS_STAT_X
#define	POS_SYNC_Y		POS_CONNECT_Y

//-----------------------------------------------------------------------------
// type definition

// PROTOCOL_USED�̎w��L���ɂ��AHIO2IF API���Ăяo���ׂ̌^
typedef HIO2IF_RESULT	(* MYREAD)(HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async);
typedef HIO2IF_RESULT	(* MYWRITE)(HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async);

//-----------------------------------------------------------------------------
// local symbol definiton

// Paint Box�\���ɎQ�ƁE�ݒ肷����
static u32	dwFbWidth, dwFbSize;
static GXColor gxColor = { 0, 0, 0, 0 };

// Host I/O API�p�o�b�t�@
static u8	rgbBuffer[DUAL_BUFFER_SIZE] ATTRIBUTE_ALIGN(32);

// �J���[�f�[�^
static const GXColor gxFont = { 0xFF, 0xFF, 0xFF, 0 }, gxBg = { 0, 0, 0, 0 };

// HIO2IF ID
static HIO2IF_ID	hioRecvID = HIO2IF_INVALID_ID,
					hioSendID = HIO2IF_INVALID_ID;

// �������A�񓯊�������
static BOOL	hioAsync = FALSE;

// �ڑ��֘A���
volatile const char*	pRecvStatus = "NOT FIND";
volatile const char*	pSendStatus = "NOT FIND";
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


//-----------------------------------------------------------------------------
// local function definiton

static void	myAppInit(void);
static void	myHioInit(void);
static void	myHioExit(void);
static u32	myMakeColor(void);
static void	myPaintBox(void);
static void	myDispInfo(void);

static inline
void	myHalt(void)
{
	OSFatal(gxFont, gxBg, HIO2IFGetErrorMessage());
}

static inline
BOOL	myIsConnect(void)
{
	return (HIO2IFIsConnected(hioRecvID) | HIO2IFIsConnected(hioSendID));
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
		OSReport("EVENT CONNECT : id(%d) hioRecvID(%d) hioSendID(%d)\n",
				 id, hioRecvID, hioSendID);
#endif
		if ( id == hioRecvID ) pRecvStatus = "CONNECT";
		else pSendStatus = "CONNECT";
		break;
	case HIO2IF_EVENT_DISCONNECT:	// �ڑ�����
#ifdef HIO2IF_DEBUG
		OSReport("EVENT DISCONNECT : id(%d) hioRecvID(%d) hioSendID(%d)\n",
				 id, hioRecvID, hioSendID);
#endif
		if ( id == hioRecvID ) pRecvStatus = "DISCONNECT";
		else pSendStatus = "DISCONNECT";
		(void)HIO2IFClose(id);
		gxColor.r = gxColor.g = gxColor.b = 0;
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
		gxColor.r = rgbBuffer[DUAL_DATA_IDX_RED];
		gxColor.g = rgbBuffer[DUAL_DATA_IDX_GREEN];
		gxColor.b = rgbBuffer[DUAL_DATA_IDX_BLUE];
		break;
	case HIO2IF_EVENT_WRITE_ASYNC_DONE:
		break;
	case HIO2IF_EVENT_INTERRUPT:
		myHalt();
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// dual-main function
//

//-----------------------------------------------------------------------------
void	main(void)
{
	MYREAD	hioIfRead;
	MYWRITE	hioIfWrite;

	// ������
	myAppInit();

	// �r���h���@�ɂ��ʐM���@��ύX
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
		HIO2IF_RESULT result;
		u32 recvStat = 0, sendStat = 0;

		// �ʐM��Ԃ�\��
		(void)HIO2IFReadStatus(hioRecvID, &recvStat);
		(void)HIO2IFReadStatus(hioSendID, &sendStat);
		(void)DEMORFPrintf(POS_STAT_X, POS_RECV_STAT_Y, 0, "R:%04X", recvStat);
		(void)DEMORFPrintf(POS_STAT_X, POS_SEND_STAT_Y, 0, "S:%04X", sendStat);

		// �R���g���[������
		DEMOPadRead();

		// A�{�^���������ꂽ���A�ڑ�
		// B�{�^���������ꂽ���A�ڑ�����
		// X�{�^���������ꂽ���ASYNC <-> ASYNC �؂�ւ�
		if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_A ) myHioInit();
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_B ) myHioExit();
		else if ( DEMOPadGetButtonDown(0) & PAD_BUTTON_X ) hioAsync=!hioAsync; 
		// PC�Ɛڑ�������Ԃ̎��A��M�������s��
		if ( bReceived && HIO2IFIsConnected(hioRecvID) )
		{
			// PC����̎�M����
			result = hioIfRead(hioRecvID, DUAL_PC2NNGC_ADDR,
							   rgbBuffer, DUAL_BUFFER_SIZE, hioAsync);
			if ( HIO2IF_FAILED(result) ) myHalt();
#ifdef	PROTOCOL_USED
			bReceived = FALSE;
#endif	// PROTOCOL_USED

			// �f�[�^����M���Ă����ꍇ�APaint Box�p�J���[�f�[�^�ɔ��f
			if ( HIO2IF_SUCCESS(result) && !hioAsync )
			{
				gxColor.r = rgbBuffer[DUAL_DATA_IDX_RED];
				gxColor.g = rgbBuffer[DUAL_DATA_IDX_GREEN];
				gxColor.b = rgbBuffer[DUAL_DATA_IDX_BLUE];
			}
		}

		// PC�Ɛڑ�������Ԃ̎��A���݂̃J���[�f�[�^��PC�֑��M
		if ( bSendPossible && HIO2IFIsConnected(hioSendID) )
		{
			rgbBuffer[DUAL_DATA_IDX_RED]	= gxColor.r;
			rgbBuffer[DUAL_DATA_IDX_GREEN]	= gxColor.g;
			rgbBuffer[DUAL_DATA_IDX_BLUE]	= gxColor.b;
			result = hioIfWrite(hioSendID, DUAL_NNGC2PC_ADDR,
								rgbBuffer, DUAL_BUFFER_SIZE, hioAsync);
			if ( HIO2IF_FAILED(result) ) myHalt();
#ifdef	PROTOCOL_USED
			bSendPossible = FALSE;
#endif	// PROTOCOL_USED
		}

		DEMOBeforeRender();

		// Paint Box �X�V
		myPaintBox();

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

	// Paint Box�\���p������
	dwFbWidth =
		(u32)(VIPadFrameBufferWidth(rmp->fbWidth) * VI_DISPLAY_PIX_SZ );
	dwFbSize	 = (u32)(dwFbWidth * rmp->xfbHeight);

	// �R���g���[��������
	DEMOPadInit();
}

//----------------------------------------------------------------------------
static
void	myHioInit(void)
{
	int count;

	if ( myIsConnect() ) return;

	// Host I/O �̏�����
	if ( HIO2IF_FAILED(HIO2IFInit()) ) myHalt();

	// �񋓂����f�o�C�X�����擾
	count = HIO2IFGetDeviceCount();

	// ��M�p�`���l���I�[�v��
	if ( count > 0 )
	{
		if ( HIO2IF_FAILED(HIO2IFOpen(
							   HIO2IFGetDevice(0),
							   HIO2IF_MODE_RDONLY,
							   myEventCallback,
							   &hioRecvID)) )
			myHalt();
		pRecvStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bReceived = FALSE;
#endif
	}

	// ���M�p�`���l���I�[�v��
	if ( count > 1 )
	{
		if ( HIO2IF_FAILED(HIO2IFOpen(
							   HIO2IFGetDevice(1),
							   HIO2IF_MODE_WRONLY,
							   myEventCallback,
							   &hioSendID)) )
			myHalt();
		pSendStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bSendPossible = TRUE;
#endif
	}
}

//----------------------------------------------------------------------------
static
void	myHioExit(void)
{
	if ( HIO2IFIsConnected(hioRecvID) )
	{
		(void)HIO2IFClose(hioRecvID);
		pRecvStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bReceived = FALSE;
#endif
	}

	if ( HIO2IFIsConnected(hioSendID) )
	{
		(void)HIO2IFClose(hioSendID);
		pSendStatus = "DISCONNECT";
#ifdef	PROTOCOL_USED
		bSendPossible = FALSE;
#endif
	}

	gxColor.r = gxColor.g = gxColor.b = 0;
}

//----------------------------------------------------------------------------
u32	myMakeColor(void)
{
#define CLAMP(x,l,h) ((x > h) ? h : ((x < l) ? l : x))

	u32  colY , colCr , colCb , colVal;
	double Y,Cr,Cb;

	Y  =  0.257 * gxColor.r + 0.504 * gxColor.g + 0.098
		* gxColor.b +  16.0 + 0.5;
	Cb = -0.148 * gxColor.r - 0.291 * gxColor.g + 0.439
		* gxColor.b + 128.0 + 0.5;
	Cr =  0.439 * gxColor.r - 0.368 * gxColor.g - 0.071
		* gxColor.b + 128.0 + 0.5;

    Y  = CLAMP(Y , 16, 235);
    Cb = CLAMP(Cb, 16, 240);
    Cr = CLAMP(Cr, 16, 240);

	colY  = (u32)Y;
	colCr = (u32)Cr;
	colCb = (u32)Cb;

	colVal = (colY << 24) | (colCb << 16) | (colY << 8) | colCr;

	return colVal;
}

//----------------------------------------------------------------------------
static
void	myPaintBox(void)
{
	u32 x=96, y=128, w=128, h=192, col = myMakeColor();
	u32	pixSize = (VI_DISPLAY_PIX_SZ << 1), lineSize = pixSize * w, i;
	u8* xfb = VIGetNextFrameBuffer();

	for (i=0; i<h; i++)
	{
		u8	*ptr, *sPtr;
		sPtr = xfb + (pixSize * x) + (dwFbWidth * (y + i));
		for (ptr=sPtr; ptr<(sPtr + lineSize); ptr+=pixSize)
			*(u32*)ptr = col;
	}

    DCStoreRange((void*)xfb, dwFbSize);
}

//----------------------------------------------------------------------------
static
void	myDispInfo(void)
{
	static const char* devName[HIO2_CHAN_MAX + 2] =
		{ "UNKOWN", "EXI2USB0", "EXI2USB1", "MrEXI" };

	// �^�C�g��
	(void)DEMORFPrintf(POS_COMM_X, POS_TITLE_Y,  0, "HIO2 DEMO - dual(%s)",
					   strProtocol);
	// �J���[
	(void)DEMORFPrintf(POS_COL_X, POS_R_Y, 0, " R:%02X(%03d)",
					   gxColor.r, gxColor.r);
	(void)DEMORFPrintf(POS_COL_X, POS_G_Y, 0, " G:%02X(%03d)",
					   gxColor.g, gxColor.g);
	(void)DEMORFPrintf(POS_COL_X, POS_B_Y, 0, " B:%02X(%03d)",
					   gxColor.b, gxColor.b);

	(void)DEMORFPrintf(POS_COMM_X, POS_RECV_Y, 0,
					   " RECV CHAN : %s, NDEV %s, PC%d",
					   pRecvStatus, devName[HIO2IFGetDeviceType(hioRecvID)+1],
					   HIO2IFGetPcChan(hioRecvID));

	(void)DEMORFPrintf(POS_COMM_X, POS_SEND_Y, 0,
					   " SEND CHAN : %s, NDEV %s, PC%d",
					   pSendStatus, devName[HIO2IFGetDeviceType(hioSendID)+1],
					   HIO2IFGetPcChan(hioSendID));

	// V�J�E���^�[
	(void)DEMORFPrintf(POS_COMM_X, POS_V_Y, 0, " V=%ld", dwVCounter);

	// �ڑ����
	(void)DEMORFPrintf(POS_CONNECT_X, POS_CONNECT_Y, 0,
					   myIsConnect() ?
					   "PUSH B DISCONNECT" : "PUSH A CONNECT");

	// SYNC/ASYNC�̏��
	(void)DEMORFPrintf(POS_SYNC_X, POS_SYNC_Y, 0, hioAsync ? "ASYNC" : "SYNC");
}

// end of dual-main.c
