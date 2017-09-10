
// EventPatternMatchingDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include <string>
#include <iostream>
#include <sstream> 
#include <unordered_map>

 
#define TESTING 0
#define CACHE_FILE_NAME  _T("event_cache.txt") 
class IEventCounter {

	CString mPath;
	int mLines;
	static CMap <CString, LPCSTR, int, int>           mMapDevIdCount;
	static CMap <CString, LPCSTR, CString, LPCSTR>    mMapDevIdDisplayText;
	CStdioFile  myCacheFile;
	static CMutex mWriteMutex;

public: 

	CString mDisplayLines;

	//constructor, store the path to file and open the cache file
	IEventCounter(CString path) {
		mPath = path;
		/*CFileException fileException;
		if (!myCacheFile.Open(CACHE_FILE_NAME, 
			CFile::modeReadWrite | CFile::typeText | CFile::modeCreate | CFile::modeNoTruncate,
			&fileException)
			) {

			//Message box dialog pops if the cachefile cannot be opened.
			MessageBox(NULL,
				_T("Cache File could not be opened"),
				_T("Warning "), NULL);

		}*/
	}
	
	IEventCounter( ) { 
	}	

	~IEventCounter() {
		//myCacheFile.Close();
	}
	
	void     ParseEvents  (CString deviceID, const char* logName);
	int      GetEventCount(CString deviceId);
	CString  GetEventLines(CString deviceID);

	bool checkAndParseFile(CString & multiLine, int& noOfFaults); 
	bool convertToCTime(CString   yearMonthDay, CString   hourMinSec, CTime& t);

	/*Cache file based implementation*/
	int  retreiveFaultsFromCache(CString deviceId);
	void UpdateCache(CString deviceId, int noOfFaults);

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

	// These 2 variables control the external GUI display. Once they are updated then DisplayData() should be called to reflect that in GUI.
	// Total number of faults found by parsing the csv file
	int mTotalNoFaults;
	// A multi line string containing all the fault locations and time stamps found in csv file
	CString mListOfFaults;
};
