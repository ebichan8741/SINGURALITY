/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     nultiDoc.cpp

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multiDoc.cpp,v 1.3 2006/03/15 06:31:26 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multiDoc.cpp : implementation of the CMultiDoc class
//

#include "stdafx.h"
#include "multi.h"

#include "multiDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMultiDoc

IMPLEMENT_DYNCREATE(CMultiDoc, CDocument)

BEGIN_MESSAGE_MAP(CMultiDoc, CDocument)
END_MESSAGE_MAP()


// CMultiDoc construction/destruction

CMultiDoc::CMultiDoc()
{
	// TODO: add one-time construction code here
	m_nHioIfID = HIO2IF_INVALID_ID;
}

CMultiDoc::~CMultiDoc()
{
}

BOOL CMultiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CMultiDoc serialization

void CMultiDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CMultiDoc diagnostics

#ifdef _DEBUG
void CMultiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMultiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMultiDoc commands
