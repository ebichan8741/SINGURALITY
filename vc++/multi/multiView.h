/*---------------------------------------------------------------------------*
  Project:  HIO2 demos - multi
  File:     multiView.h

  (C)2005 HUDSON SOFT

  $Header: /home/cvsroot/SDK/build/demos/hio2demo/vc++/multi/multiView.h,v 1.3 2006/03/15 06:31:26 mitu Exp $

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

// multiView.h : interface of the CMultiView class
//


#pragma once


class CMultiView : public CFormView
{
///////// for multiApp /////////
public:
	enum USER_EVENT
	{
		EVENCT_RECEIVED,
		EVENT_CONNECT,
		EVENT_DISCONNECT,
		EVENT_UPDATE_PC_TIME,
		EVENT_WRITE_DONE,
	};
private:
	MULTI_PACKET m_stPacket;

protected: // create from serialization only
	CMultiView();
	DECLARE_DYNCREATE(CMultiView)

public:
	enum{ IDD = IDD_MULTI_FORM };

// Attributes
public:
	CMultiDoc* GetDocument() const;

// Operations
public:

// Overrides
	public:
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~CMultiView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnMyMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedRadioSync();
	afx_msg void OnBnClickedRadioAsync();
protected:
	virtual void OnDraw(CDC* /*pDC*/);
};

#ifndef _DEBUG  // debug version in multiView.cpp
inline CMultiDoc* CMultiView::GetDocument() const
   { return reinterpret_cast<CMultiDoc*>(m_pDocument); }
#endif

