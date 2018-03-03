/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2DllIf.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/HioIf/include/Hio2DllIf.h,v 1.3 2006/03/15 06:31:25 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// DLL wrapper class for hio2[D].dll

#pragma once

#include <revolution/hio2.h>

class CHio2DllIf
{
public:
	CHio2DllIf()	{ m_hDll = NULL; };
	~CHio2DllIf()	{ Release(); };

	BOOL	Create();
	void	Release();

	BOOL		Init			( void );
	s32			EnumDevices		( HIO2EnumCallback callback, void* param );
	HIO2Handle	Open			( HIO2DevicePath pathName, HIO2ReceiveCallback callback, HIO2NotifyCallback notify, void* param);
	BOOL		Close			( HIO2Handle h );
	BOOL		ReadMailbox		( HIO2Handle h, u32* mail );
	BOOL		WriteMailbox	( HIO2Handle h, u32  mail );
	BOOL		Read			( HIO2Handle h, u32 addr, void* buffer, s32 size );
	BOOL		Write			( HIO2Handle h, u32 addr, const void* buffer, s32 size );
	BOOL		ReadAsync		( HIO2Handle h, u32 addr, void* buffer, s32 size, HIO2DmaCallback callback );
	BOOL		WriteAsync		( HIO2Handle h, u32 addr, const void* buffer, s32 size, HIO2DmaCallback callback );
	BOOL		ReadStatus		( HIO2Handle h, u32* status );
	void		Exit			( void );
	HIO2Error	GetLastError	( void );

protected:
	HIO2EnumDevicesType		m_fncEnumDevices;
	HIO2InitType			m_fncInit;
	HIO2OpenType			m_fncOpen;
	HIO2CloseType			m_fncClose;
	HIO2ReadMailboxType		m_fncReadMailbox;
	HIO2WriteMailboxType	m_fncWriteMailbox;
	HIO2ReadType			m_fncRead;
	HIO2WriteType			m_fncWrite;
	HIO2ReadAsyncType		m_fncReadAsync;
	HIO2WriteAsyncType		m_fncWriteAsync;
	HIO2ReadStatusType		m_fncReadStatus;
	HIO2ExitType			m_fncExit;
	HIO2GetLastErrorType	m_fncGetLastError;

	HINSTANCE	m_hDll;
};

// end of Hio2DllIf.h
