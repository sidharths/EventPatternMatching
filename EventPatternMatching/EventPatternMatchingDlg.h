
// EventPatternMatchingDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include <string>
#include <iostream>
#include <sstream> 
#include <unordered_map>

 
#define TESTING 1
class IEventCounter {

	CString mPath;
	int mLines;
	CMap <CString, LPCWSTR, int, int> mMapDevIdCount;

public: 

	IEventCounter(CString path) {
		mPath = path;
	}
	
	IEventCounter( ) { 
	}

	void ParseEvents  (CString deviceID, const char* logName);
	int  GetEventCount(CString deviceId);


	bool checkAndParseFile(CString & multiLine, int& noOfFaults); 
	bool convertToCTime(CString   yearMonthDay, CString   hourMinSec, CTime& t);

};


// CEventPatternMatchingDlg dialog
class CEventPatternMatchingDlg : public CDialogEx
{
// Construction
public:
	CEventPatternMatchingDlg(CWnd* pParent = NULL);	// standard constructor

	class IEventCounter*   mIEventCounter;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EVENTPATTERNMATCHING_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeEdit1(); 
	afx_msg void OnEnChangeEdit2(); 
	// Total number of faults found by parsing the csv file
	int mTotalNoFaults;
	// A multi line string containing all the fault locations and time stamps found in csv file
	CString mListOfFaults;
};
