
// EventPatternMatchingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EventPatternMatching.h"
#include "EventPatternMatchingDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//My class to handle the file and check for event

bool IEventCounter::checkAndParseFile(CString & multiLine, int& noOfFaults) {

	CStdioFile  myFile;
	CFileException fileException; 
	noOfFaults = 0;

	if (!myFile.Open(mPath, CFile::modeRead, &fileException)) {
		
		MessageBox(NULL,
			(LPCWSTR)L"File Not Found",
			(LPCWSTR)L" ", NULL);
		return false;	
	}

	CString strRow;
	int i = 0;
	bool stageThreeStarted = false;
	bool faultStageStarted = false;
	CTime stageThreeStartTime;
	CTimeSpan fiveMinuteGap(0, 0, 5, 0);

	while ( myFile.ReadString(strRow) )
	{ 
		int nTokenPos = 0;

		CTime currentTime;
		int stageCurrent=-1;

		if (!i) {
			//The first line should be "timestamp" and "value"only
			CString tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (tokenized.MakeLower() != L"timestamp")
				return false;

			tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (tokenized.MakeLower() != L"value")
				return false;

			tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (!tokenized.IsEmpty())
				return false;			
			
			++i;
			continue;
		}
		else {
			CString yearMonthDay = strRow.Tokenize(_T(" \r\n\t"), nTokenPos); 
			CString hourMinSec = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			CString stage = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);

			CString tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (!tokenized.IsEmpty())
				return false;

			if ( !this->convertToCTime(yearMonthDay, hourMinSec, currentTime))
				return false;

			stageCurrent = _ttoi(stage);
		}
		 
		if (!stageThreeStarted && stageCurrent == 3) {
			stageThreeStarted   = true;
			stageThreeStartTime = currentTime;
		}

		if (stageThreeStarted && (stageCurrent == 0 || stageCurrent == 1) )
			stageThreeStarted   = false;

		if (stageThreeStarted &&  stageCurrent == 2 &&
			((currentTime - stageThreeStartTime) < fiveMinuteGap) )
			stageThreeStarted   = false;

		if ( !faultStageStarted &&
			stageThreeStarted &&  stageCurrent == 2 &&  
			( (currentTime-stageThreeStartTime) >= fiveMinuteGap) ) {
			faultStageStarted = true;
		}

		if (faultStageStarted && stageCurrent == 1 ) {
			stageThreeStarted = false;
			faultStageStarted = false;
		}


		if (faultStageStarted && stageCurrent == 0) {
			++noOfFaults;
			CString formatStr;

			formatStr.Format(_T("Fault on Stage 0 at Timestamp %d-%02d-%02d %02d:%02d:%02d ( Line Number %d ) \r\n"),
								currentTime.GetYear(),
								currentTime.GetMonth(),
								currentTime.GetDay(),
								currentTime.GetHour(),
								currentTime.GetMinute(),
								currentTime.GetSecond(),
								i+1
							 );

			multiLine += formatStr;

			stageThreeStarted = false;
			faultStageStarted = false;

		}
		//	MessageBox(NULL,(LPCWSTR)  strRow,(LPCWSTR)L" ",NULL);
		
		++i;
	}

	myFile.Close();
	mLines = i - 1;

	//MessageBox(NULL,(LPCWSTR)L"File Loaded Successfully",(LPCWSTR)L" ",	NULL);
	
	return true;
}




bool IEventCounter::convertToCTime(CString   yearMonthDay, CString   hourMinSec, CTime& t) {

	int nTokenPos  = 0;
	CString year   = yearMonthDay.Tokenize(_T("-"), nTokenPos);
	CString month  = yearMonthDay.Tokenize(_T("-"), nTokenPos);
	CString day    = yearMonthDay.Tokenize(_T("-"), nTokenPos);

	nTokenPos = 0;
	CString hour   = hourMinSec.Tokenize(_T(":"), nTokenPos);
	CString minute = hourMinSec.Tokenize(_T(":"), nTokenPos);
	CString second = hourMinSec.Tokenize(_T(":"), nTokenPos);

	int yyyy = _ttoi(year);
	int mm   = _ttoi(month);
	int dd   = _ttoi(day);
	int hh   = _ttoi(hour);
	int mi   = _ttoi(minute);
	int ss   = _ttoi(second);	 

	t = CTime(yyyy, mm, dd, hh, mi, ss);

	return true;

}



void IEventCounter::ParseEvents(CString deviceID, const char* logName) {
		
	int temp;
	if (!mMapDevIdCount.Lookup(deviceID, temp)) {
		CString path(logName);
		mPath = path;
		CString multiLine = _T("");
		int noOfFaults = 0;

		this->checkAndParseFile(multiLine, noOfFaults);
		mMapDevIdCount[deviceID] = noOfFaults;
	}

}

int  IEventCounter::GetEventCount(CString deviceID) {
	
	int temp;

	if (!mMapDevIdCount.Lookup(deviceID, temp))
		return -1;
	else
		return temp;
	 
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CEventPatternMatchingDlg dialog



CEventPatternMatchingDlg::CEventPatternMatchingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_EVENTPATTERNMATCHING_DIALOG, pParent)
	, mTotalNoFaults(0)
	, mListOfFaults(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEventPatternMatchingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, mTotalNoFaults);
	DDX_Text(pDX, IDC_EDIT1, mListOfFaults);
}

BEGIN_MESSAGE_MAP(CEventPatternMatchingDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CEventPatternMatchingDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT1, &CEventPatternMatchingDlg::OnEnChangeEdit1) 
	ON_EN_CHANGE(IDC_EDIT2, &CEventPatternMatchingDlg::OnEnChangeEdit2)
END_MESSAGE_MAP()


// CEventPatternMatchingDlg message handlers

BOOL CEventPatternMatchingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEventPatternMatchingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEventPatternMatchingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEventPatternMatchingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CEventPatternMatchingDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here

	CFileDialog file1(true);

	if (file1.DoModal() == IDOK)
	{  
		CString PathToFile = file1.GetPathName();

		mIEventCounter = new IEventCounter(PathToFile);

		CString faultLines = _T("");
		int     totalFaults;

		//Testing  
#if TESTING
		char *myString; 
		myString = CA2W(PathToFile);
		mIEventCounter->ParseEvents(_T("myDeviceId"), myString);
		totalFaults = mIEventCounter->GetEventCount(_T("myDeviceId") );

#else
		if (!mIEventCounter->checkAndParseFile(faultLines, totalFaults)) {
				MessageBox( (LPCWSTR)L"File not in correct format, please check again!" );
		}
#endif
		 
		mListOfFaults = faultLines; 
		mTotalNoFaults = totalFaults;
		UpdateData(false); 

	}

}


void CEventPatternMatchingDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

 


void CEventPatternMatchingDlg::OnEnChangeEdit2()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
