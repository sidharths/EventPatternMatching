
// EventPatternMatchingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EventPatternMatching.h"
#include "EventPatternMatchingDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*
FUNCTION
bool IEventCounter::checkAndParseFile (CString & multiLine, int& noOfFaults)

ON SUCCESS:
The function returns 'true' on SUCCESS.
the arguments 'multiLine' and 'noOfFaults' then contain the text descriptions of lines where fault was detected and the total number of faults.

ON FAILURE:
The function resturns 'false'
Which means the file is in bad and not the expected format. More messages can be added on later to what kind of format error occured.

*/

// a private mutex variable to synchrnozie threads accessing static variables
CMutex                            IEventCounter::mWriteMutex;


//   private dictionary variables to store maxevent count and the Display text for each devid. 
//   We will use mutex to exclusively accesss the dictionary when lot of threads are calling the functions.
CMap <CString, LPCSTR,     int,    int>	 IEventCounter::mMapDevIdCount;
CMap <CString, LPCSTR, CString, LPCSTR>  IEventCounter::mMapDevIdDisplayText;
 

bool IEventCounter::checkAndParseFile(CString & multiLine, int& noOfFaults) {

	CStdioFile  myFile;  //to open and read the input file
	CFileException fileException; 
	noOfFaults = 0;

	if (!myFile.Open(mPath, CFile::modeRead, &fileException)) {
		
		//Message box dialog pops if the file cannot be opened.
		MessageBox(NULL, 
			_T("Inout File Not Found"),
			_T("Error "), NULL);
		return false;	
	}

	CString strRow; // Read the text file line by line
	
	int lineNo = 1;  //Line Number of the parsed file


	bool stageThreeStarted = false;       //This is flagged true when we detect stage3 for first time or after a fault.

/*  I am calling the event - stage 3 to stage 2 transition happening after being on stage 3 for more than 5 minutes - 
	as "faultStageBegins" has happened.
    This is flagged true, when the aforementioned series of events happens 
*/
	bool faultStageBegins = false;
	CTime stageThreeStartTime;            // a timer to keep track of start of the 5 minute gap we are looking for
	CTimeSpan fiveMinuteGap(0, 0, 5, 0);  // Hard coding the time gap in a CTimeSpan constant

	while ( myFile.ReadString(strRow) )  // read line by line
	{ 
		int nTokenPos = 0;    // offset while we tokenize a string line

		CTime currentTime;    // In the CTime structure we will store the current lines time. CTime will help us in comparing time between two lines.
		int stageCurrent=-1;  // Here we will store the stage number of current line of file

		/*
		Here I am assuming that file is in correct format. That is we expect
		yyyy:mm:dd <whitespace> hh:mm:ss <whitespace> stagenumber
		So each line we have 3 token strings separated by tab. I am coding for more general separated by white space which could be tab, space, carriage feed etc
		1. The first token string tell the year day month and is seprated by hyphen (-)
		2. The second token string tells the hour, minute and second and is seperated by (:)
		3. The last token string is an integer limited to 0,1,2,3.

		The first line of the file has a header which should be "timestamp" <whitespace> "value"

		*/

		if (lineNo==1) { // check the header of file

			//The first line should be "timestamp" and "value" only

			CString tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (tokenized.MakeLower() != L"timestamp")
				return false;

			tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (tokenized.MakeLower() != L"value")
				return false;

			tokenized = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (!tokenized.IsEmpty())
				return false;			
			
			++lineNo;
			continue;
		}
		else {
			CString yearMonthDay  = strRow.Tokenize(_T(" \r\n\t"), nTokenPos); 
			CString hourMinSec    = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			CString stage         = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);

			CString tokenized     = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
			if (!tokenized.IsEmpty()) //not expecting any string after the first 3 tokens in the line
				return false;

			// currentTime will hold the parsed current time of line
			if ( !this->convertToCTime(yearMonthDay, hourMinSec, currentTime)) //Converting to CTime structure
				return false;

			//stageCurrent will hold the current stage number
			stageCurrent = _ttoi(stage);
		}
		 
		/* Here the logic of detecting fault is implemented. 
		Basically we are trying to isolate the state space.
		This is done as and when we parse the file, line by line.
		*/

		// if stage 3 or "faultstagebegins" hasnt been seen yet and the current stage is stage 3 then set the flag and save start time.
		if (!stageThreeStarted && !faultStageBegins 
			&& stageCurrent == 3) {
			stageThreeStarted   = true;
			stageThreeStartTime = currentTime; 
		}

		// While on stage 3 but not on faultStageBegins, if we get stage 0 or 1 at anytime then reset the stage 3 start flag. because these case dont lead to fault.
	    else if (stageThreeStarted && !faultStageBegins 
			 && (stageCurrent == 0 || stageCurrent == 1)) {
			stageThreeStarted = false; 
		}

		// While on stage 3 but not on faultStageBegins, if we get stage 2  BUT if it hasnt been 5 minutes in stage 3 then this case also wont lead to fault. Hence reset the flag.
		else if (stageThreeStarted && !faultStageBegins  
			&&  stageCurrent == 2 
			&& ((currentTime - stageThreeStartTime) < fiveMinuteGap)) {
			stageThreeStarted = false; 
		}

		// While on stage 3 but not on faultStageBegins,  we reach stage 2 BUT after spending 5 minutes in stage 3. Then we can say we are closer to fault stage. Hence we set the faultStageBegins flag.
		else if (stageThreeStarted && !faultStageBegins  
			 &&  stageCurrent == 2 &&
			( (currentTime-stageThreeStartTime) >= fiveMinuteGap) ) {
			faultStageBegins = true; 
		}

		// Once fault stage has started then if stage is recorded that means reset all flags and start again.
		else if (faultStageBegins && stageCurrent == 1 ) {
			stageThreeStarted = false;
			faultStageBegins  = false; 
		}

		// if we reach however to stage 0. That means this is the fault we are looking for. 
		else if (faultStageBegins && stageCurrent == 0) {
			++noOfFaults; // increase the count of total faults

			//store the information of the location of the fault in CString so that it can be displayed in GUI
			CString formatStr;
			formatStr.Format(_T("Fault on Stage 0 at Timestamp %d-%02d-%02d %02d:%02d:%02d ( Line Number %d ) \r\n"),
								currentTime.GetYear(),
								currentTime.GetMonth(),
								currentTime.GetDay(),
								currentTime.GetHour(),
								currentTime.GetMinute(),
								currentTime.GetSecond(),
								lineNo
							 );
			multiLine += formatStr;

			//reset all flags after the fault has been detected and processed
			stageThreeStarted = false;
			faultStageBegins  = false;

		}
		//	MessageBox(NULL,(LPCWSTR)  strRow,(LPCWSTR)L" ",NULL);
		
		++lineNo;
	}

	myFile.Close(); //close the file
	mLines = lineNo; //store total line numbers in file just for debugging purposes.

	//MessageBox(NULL,(LPCWSTR)L"File Loaded Successfully",(LPCWSTR)L" ",	NULL);
	
	return true;
}

/*
FUNCTION
bool IEventCounter::convertToCTime (CString   yearMonthDay, CString   hourMinSec, CTime& mTime)

ON SUCCESS:
The function returns 'true' on SUCCESS.
the argument 'mTime' would coontain the CTime structure of the input timestamp (yearMonthDay & hourMinSec)

ON FAILURE:
The function resturns 'false' 
Currently I am not returning false but that feature can be added if the format of file has to be rigorously checked.
*/


bool IEventCounter::convertToCTime(CString   yearMonthDay, CString   hourMinSec, CTime& mTime) {

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

	mTime = CTime(yyyy, mm, dd, hh, mi, ss);

	return true;

}

/*
FUNCTION

void IEventCounter::ParseEvents(CString deviceID, const char* logName)

ON SUCCESS:
The  file provided by the path (logName) would be correctly parsed and the fault events would have been detected.

ON FAILURE:
Message boxes would pop up to the user saying "file could not be opened" or "bad format"

This function also updates the internal dictionary and keeps track of the total number of faults for the deviceID.

*/


void IEventCounter::ParseEvents(CString deviceID, const char* logName) {
		
	int temp;

/*	
	//retreive from file cache
	int totalFaults = this->retreiveFaultsFromCache(deviceID);
	
	if (totalFaults != -1) {
		mMapDevIdCount[deviceID] = totalFaults;
		return;
	}
*/

	mWriteMutex.Lock();
	bool status = mMapDevIdCount.Lookup(deviceID, temp);
	mWriteMutex.Unlock();

	if (!status) {  // if deviceID is not in the CMap dictionary'=

		CString path(logName);
		mPath = path;
		CString multiLine = _T("");
		int noOfFaults = 0;

		// call checkAndParsefile to process the file
		this->checkAndParseFile(multiLine, noOfFaults);

		//store debug info on details of events in a public variable
		mDisplayLines = multiLine;

		//update the dictionary and store the total number of faults as the value to the deviceID key.

		mWriteMutex.Lock();
		mMapDevIdCount[deviceID]       = noOfFaults;
		mMapDevIdDisplayText[deviceID] = mDisplayLines;
		mWriteMutex.Unlock();

		//update file cache
		//this->UpdateCache(deviceID, noOfFaults);
	}

}


/*
FUNCTION

int  IEventCounter::GetEventCount(CString deviceID)

ON SUCCESS:
Retreives from the internal dictionary CMap, the total number of fault events based on the deviceID.

ON FAILURE:
returns -1

*/

int  IEventCounter::GetEventCount(CString deviceID) {
	
	int temp;

	mWriteMutex.Lock();

	if (!mMapDevIdCount.Lookup(deviceID, temp))
		return -1;
	else
		return temp;

	mWriteMutex.Unlock();
	 
}



CString  IEventCounter::GetEventLines(CString deviceID) {

	CString temp;

	mWriteMutex.Lock();

	if (!mMapDevIdDisplayText.Lookup(deviceID, temp))
		return _T("");
	else
		return temp;

	mWriteMutex.Unlock();

}


// This function is the event handler we select and open the file from the GUI.

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
		char* myString;
		myString= CT2A(PathToFile);
		mIEventCounter->ParseEvents(_T("myDeviceId"), myString);
		totalFaults = mIEventCounter->GetEventCount(_T("myDeviceId"));
		faultLines  = mIEventCounter->GetEventLines(_T("myDeviceId"));
#else
		if (!mIEventCounter->checkAndParseFile(faultLines, totalFaults)) {
			MessageBox((LPCSTR)L"File not in correct format, please check again!");
		}
#endif

		mListOfFaults  = faultLines;
		mTotalNoFaults = totalFaults;
		UpdateData(false);

	}

}




/*
Function for cache file based implementation. This could be useful in multi process environment, but file access will be slower to hash map implementation.

*/


int IEventCounter::retreiveFaultsFromCache(CString deviceId) {

	CString strRow;
	int nTokenPos = 0;


	mWriteMutex.Lock();

	myCacheFile.Seek(0, CFile::begin);
	while (myCacheFile.ReadString(strRow))  // read line by line
	{
		CString devId = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);
		CString totalCount = strRow.Tokenize(_T(" \r\n\t"), nTokenPos);

		if (devId == deviceId)
			return _ttoi(totalCount);
	}

	mWriteMutex.Unlock();

	return -1;
}

void  IEventCounter::UpdateCache(CString deviceId, int noOfFaults) {
	CString strRow;
	strRow.Format(_T("%s\t%d"), deviceId, noOfFaults);

	mWriteMutex.Lock();

	myCacheFile.Seek(0, CFile::end);
	myCacheFile.WriteString(strRow);

	mWriteMutex.Unlock();
}

/* rest of the code is the template that came with MFC

*/



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
