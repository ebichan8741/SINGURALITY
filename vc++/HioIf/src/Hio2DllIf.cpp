/*---------------------------------------------------------------------------*
  Project:  Host I/O Interface for HIO2
  File:     Hio2DllIf.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/HioIf/src/Hio2DllIf.cpp,v 1.3 2006/03/15 06:31:26 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "Hio2DllIf.h"

///////////////////////////////////////////////////////////////////////////////
//
// CHio2DllIf
//

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::Create()
{
#ifdef	_DEBUG
	LPCSTR	lpszLibFileName	= "hio2D.dll";
#else
	LPCSTR	lpszLibFileName	= "hio2.dll";
#endif
	// èâä˙âªçœÇ›
	if ( m_hDll != NULL ) return TRUE;
	
	m_hDll = LoadLibrary(lpszLibFileName);
	if ( m_hDll == NULL ) return FALSE;

	m_fncEnumDevices	= (HIO2EnumDevicesType)GetProcAddress(m_hDll, HIO2EnumDevicesStr);
	m_fncInit			= (HIO2InitType)GetProcAddress(m_hDll, HIO2InitStr);
	m_fncOpen			= (HIO2OpenType)GetProcAddress(m_hDll, HIO2OpenStr);
	m_fncClose			= (HIO2CloseType)GetProcAddress(m_hDll, HIO2CloseStr);
	m_fncReadMailbox	= (HIO2ReadMailboxType)GetProcAddress(m_hDll, HIO2ReadMailboxStr);
	m_fncWriteMailbox	= (HIO2WriteMailboxType)GetProcAddress(m_hDll, HIO2WriteMailboxStr);
	m_fncRead			= (HIO2ReadType)GetProcAddress(m_hDll, HIO2ReadStr);
	m_fncWrite			= (HIO2WriteType)GetProcAddress(m_hDll, HIO2WriteStr);
	m_fncReadAsync		= (HIO2ReadAsyncType)GetProcAddress(m_hDll, HIO2ReadAsyncStr);
	m_fncWriteAsync		= (HIO2WriteAsyncType)GetProcAddress(m_hDll, HIO2WriteAsyncStr);
	m_fncReadStatus		= (HIO2ReadStatusType)GetProcAddress(m_hDll, HIO2ReadStatusStr);
	m_fncExit			= (HIO2ExitType)GetProcAddress(m_hDll, HIO2ExitStr);
	m_fncGetLastError	= (HIO2GetLastErrorType)GetProcAddress(m_hDll, HIO2GetLastErrorStr);

	if ( (m_fncEnumDevices == NULL)
		|| (m_fncInit == NULL)
		|| (m_fncOpen == NULL)
		|| (m_fncClose == NULL)
		|| (m_fncReadMailbox == NULL)
		|| (m_fncWriteMailbox == NULL)
		|| (m_fncRead == NULL)
		|| (m_fncWrite == NULL)
		|| (m_fncReadAsync == NULL)
		|| (m_fncWriteAsync == NULL)
		|| (m_fncReadStatus == NULL)
		|| (m_fncExit == NULL)
		|| (m_fncGetLastError == NULL) )
	{
		FreeLibrary(m_hDll);
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
void	CHio2DllIf::Release()
{
	if ( m_hDll != NULL )
	{
		FreeLibrary(m_hDll);
		m_hDll = NULL;
	}
}

//-----------------------------------------------------------------------------
s32	CHio2DllIf::EnumDevices( HIO2EnumCallback callback, void* param )
{
	ASSERT(m_hDll != NULL);
	return m_fncEnumDevices(callback, param);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::Init( void )
{
	ASSERT(m_hDll != NULL);
	return m_fncInit();
}

//-----------------------------------------------------------------------------
HIO2Handle	CHio2DllIf::Open( HIO2DevicePath pathName, HIO2ReceiveCallback callback,
				 HIO2NotifyCallback notify, void* param)
{
	ASSERT(m_hDll != NULL);
	return m_fncOpen(pathName, callback, notify, param);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::Close( HIO2Handle h )
{
	ASSERT(m_hDll != NULL);
	return m_fncClose(h);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::ReadMailbox( HIO2Handle h, u32* mail )
{
	ASSERT(m_hDll != NULL);
	return m_fncReadMailbox(h, mail);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::WriteMailbox( HIO2Handle h, u32  mail )
{
	ASSERT(m_hDll != NULL);
	return m_fncWriteMailbox(h, mail);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::Read( HIO2Handle h, u32 addr, void* buffer, s32 size )
{
	ASSERT(m_hDll != NULL);
	return m_fncRead(h, addr, buffer, size);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::Write( HIO2Handle h, u32 addr, const void* buffer, s32 size )
{
	ASSERT(m_hDll != NULL);
	return m_fncWrite(h, addr, buffer, size);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::ReadAsync( HIO2Handle h, u32 addr, void* buffer,
							  s32 size, HIO2DmaCallback callback )
{
	ASSERT(m_hDll != NULL);
	return m_fncReadAsync(h, addr, buffer, size, callback);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::WriteAsync( HIO2Handle h, u32 addr, const void* buffer,
							   s32 size, HIO2DmaCallback callback )
{
	ASSERT(m_hDll != NULL);
	return m_fncWriteAsync(h, addr, buffer, size, callback);
}

//-----------------------------------------------------------------------------
BOOL	CHio2DllIf::ReadStatus( HIO2Handle h, u32* status )
{
	ASSERT(m_hDll != NULL);
	return m_fncReadStatus(h, status);
}

//-----------------------------------------------------------------------------
void	CHio2DllIf::Exit( void )
{
	ASSERT(m_hDll != NULL);
	m_fncExit();
}

//-----------------------------------------------------------------------------
HIO2Error	CHio2DllIf::GetLastError( void )
{
	ASSERT(m_hDll != NULL);
	return m_fncGetLastError();
}

// end of Hio2DllIf.cpp
