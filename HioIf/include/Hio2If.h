/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2If.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/HioIf/include/Hio2If.h,v 1.2 2006/03/09 12:28:43 yasuh-to Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// HIO2 API wrapper interface

#ifndef	__HIO2IF_H__
#define	__HIO2IF_H__

#include <revolution/hio2.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// define

#define	HIO2IF_INVALID_ID	-1

// packet command (private)
//  -> packet = ((EXI chan(8bit) << 8) | cmd(8bit))
enum HIO2IF_CMD
{
	HIO2IF_CMD_OPEN_RDONLY = 1,	// NNGC ->  PC
	HIO2IF_CMD_OPEN_WRONLY,		// NNGC ->  PC
	HIO2IF_CMD_OPEN_RDWR,		// NNGC ->  PC
	HIO2IF_CMD_OPEN_RESULT,		// NNGC <-  PC
	HIO2IF_CMD_SEND,			// NNGC <-> PC
	HIO2IF_CMD_SEND_RESULT,		// NNGC <-> PC
	HIO2IF_CMD_CLOSE,			// NNGC <-> PC

	HIO2IF_CMD_MASK = 0xFF
};

// async mode (private)
enum HIO2IF_ASYNC
{
	HIO2IF_ASYNC_NONE		= 0x00000000,
	HIO2IF_ASYNC_READ		= 0x00000001,
	HIO2IF_ASYNC_READ_FREE	= 0x00000002,
	HIO2IF_ASYNC_WRITE		= 0x00000004,
	HIO2IF_ASYNC_WRITE_FREE	= 0x00000008
};

//-----------------------------------------------------------------------------
// type definiton

// ID
typedef int	HIO2IF_ID;

// open mode
typedef enum
{
	HIO2IF_MODE_NONE = -1,
	HIO2IF_MODE_RDONLY,		// ��M�̂�
	HIO2IF_MODE_WRONLY,		// ���M�̂�
	HIO2IF_MODE_RDWR		// ����M
} HIO2IF_MODE;

// interface result
typedef enum
{
	HIO2IF_RESULT_FATAL = -1,	// HIO2 API�ŃG���[������
	HIO2IF_RESULT_SUCCESS,		// ����
	HIO2IF_RESULT_ERROR			// �G���[�i���[�h���Ⴄ���j
} HIO2IF_RESULT;

// event
typedef enum
{
	HIO2IF_EVENT_CONNECT,			// �ڑ�����
	HIO2IF_EVENT_DISCONNECT,		// �ڑ�����
	HIO2IF_EVENT_RECEIVED,			// �f�[�^��M
	HIO2IF_EVENT_SEND_POSSIBLE,		// ���M�\��ԁi�ڑ����肪�f�[�^����M�j
	HIO2IF_EVENT_READ_ASYNC_DONE,	// Read Async���I��
	HIO2IF_EVENT_WRITE_ASYNC_DONE,	// Write Async���I��
	HIO2IF_EVENT_INTERRUPT,			// �����}�����荞�ݔ����iNNGC only�j
	HIO2IF_EVENT_UNKOWN
} HIO2IF_EVENT;

// event callback
typedef void	(* HIO2IF_EVENT_CALLBACK)(HIO2IF_ID id, HIO2IF_EVENT event);

// error code
typedef enum
{
	// error
	HIO2IF_ERROR_NONE = 0,
	HIO2IF_ERROR_CHAN_NOT_FIND,
	HIO2IF_ERROR_CHAN_ALREADY_OPENED,
	HIO2IF_ERROR_NOT_CONNECT,
	HIO2IF_ERROR_WRITE_ONLY,
	HIO2IF_ERROR_READ_ONLY,
	HIO2IF_ERROR_NOT_RECV_DATA,
	HIO2IF_ERROR_CANNOT_SEND_DATA,
	HIO2IF_ERROR_BUSY,
	HIO2IF_ERROR_INVALID_ID,

	// fatal error
	HIO2IF_FATAL_ENUMDEVICES,
	HIO2IF_FATAL_INIT,
	HIO2IF_FATAL_OPEN,
	HIO2IF_FATAL_CLOSE,
	HIO2IF_FATAL_READ,
	HIO2IF_FATAL_WRITE,
	HIO2IF_FATAL_READSTATUS,
	HIO2IF_FATAL_LOAD_DLL,	// PC only

	HIO2IF_ERROR_MAX
} HIO2IF_ERROR;

// Host I/O interface

#ifndef	WIN32

// status (private)
typedef struct
{
	HIO2DeviceType	nType;					// HIO2�f�o�C�X�^�C�v
	s32				nPc;					// �i�[���jUSB�@��ԍ�
	HIO2Handle		hHIO;					// HIO2 API�n���h��
	HIO2IF_MODE		nMode;					// �I�[�v�����[�h
	volatile BOOL	bConnect;				// PC�Ƃ̐ڑ���
	volatile BOOL	bReceived;				// PC����̃f�[�^��M�ʒm
	volatile BOOL	bSendPossible;			// PC�֑��M�\�ȏ��
	volatile u32	dwAsyncMode;			// Async���[�h
	volatile void*	pReadAsyncPtr;			// Read Async�Ɏw�肵���o�b�t�@
	volatile u32	dwReadAsyncSize;		// Read Async�Ɏw�肵���T�C�Y
	HIO2IF_EVENT_CALLBACK	fncCallback;	// �C�x���g��M���̃R�[���o�b�N�֐�
} HIO2IF_STATUS;

// Host I/O interface for NNGC (function prototypes)

// EXI�`���l�����Q�Ɨp�֐�
s32				HIO2IFGetDeviceCount	( void );
HIO2DeviceType	HIO2IFGetDevice			( s32 index );

// HIO2 API����֐�
HIO2IF_RESULT	HIO2IFInit				( void );
HIO2IF_RESULT	HIO2IFOpen				( HIO2DeviceType type, HIO2IF_MODE mode, HIO2IF_EVENT_CALLBACK callback, HIO2IF_ID* id );
HIO2IF_RESULT	HIO2IFRead				( HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async );
HIO2IF_RESULT	HIO2IFReadFree			( HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async );
HIO2IF_RESULT	HIO2IFWrite				( HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async );
HIO2IF_RESULT	HIO2IFWriteFree			( HIO2IF_ID id, u32 addr, void* buffer, s32 size, BOOL async );
HIO2IF_RESULT	HIO2IFReadStatus		( HIO2IF_ID id, u32* status );
HIO2IF_RESULT	HIO2IFClose				( HIO2IF_ID id );
void			HIO2IFSync				( void );
void			HIO2IFExit				( void );

// ��ԎQ�Ɨp�֐�
BOOL			HIO2IFIsConnected		( HIO2IF_ID id );
BOOL			HIO2IFIsReceived		( HIO2IF_ID id );
BOOL			HIO2IFIsSendPossible	( HIO2IF_ID id );
HIO2DeviceType	HIO2IFGetDeviceType		( HIO2IF_ID id );
s32				HIO2IFGetPcChan			( HIO2IF_ID id );

// �G���[���擾�p�֐�
HIO2IF_ERROR	HIO2IFGetLastError		( void );
const char*		HIO2IFGetErrorMessage	( void );

#endif	// WIN32

//-----------------------------------------------------------------------------
// macro definiton

// packet (private)
#define	HIO2IF_SET_PACKET(data, cmd)	((u32)(((data) << 8) | (cmd)))
#define	HIO2IF_GET_PACKET_CMD(packet)	((packet) & HIO2IF_CMD_MASK)
#define	HIO2IF_GET_PACKET_CHAN(packet)	(s32)(((packet) >> 8) & HIO2IF_CMD_MASK)

// fatal error check 
#define	HIO2IF_FAILED(result)	\
	(((result) == HIO2IF_RESULT_FATAL) ? TRUE : FALSE)

#define	HIO2IF_SUCCESS(result)	\
	(((result) == HIO2IF_RESULT_SUCCESS) ? TRUE : FALSE)

// async mode check (private)
#define	HIO2IF_ASYNC_READ_MASK(mode)	\
	((mode) & (HIO2IF_ASYNC_READ | HIO2IF_ASYNC_READ_FREE))
#define	HIO2IF_ASYNC_WRITE_MASK(mode)	\
	((mode) & (HIO2IF_ASYNC_WRITE | HIO2IF_ASYNC_WRITE_FREE))

#ifdef __cplusplus
}
#endif
#endif	// __HIO2IF_H__
