/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multiDoc.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multiDoc.h,v 1.3 2006/03/15 06:31:26 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multiDoc.h : interface of the CMultiDoc class
//


#pragma once

class CMultiDoc : public CDocument
{
///////// for multiApp /////////
public:
	HIO2IF_ID	m_nHioIfID;
	void	UpdateTitle()
	{
		SetTitle(GetApp()->GetStatus(m_nHioIfID));
	};

protected: // create from serialization only
	CMultiDoc();
	DECLARE_DYNCREATE(CMultiDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CMultiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


