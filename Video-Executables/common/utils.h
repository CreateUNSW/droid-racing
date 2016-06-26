#ifndef __OI_UTILS_H
#define __OI_UTILS_H

#include <string>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#ifndef WIN32
#include <sys/time.h>
#else
#include <Windows.h>
#endif

#define SUBTAG_ATTRS_CNT 4

enum LatencyType
{
	LiveRead,
	LiveSend,
	ClientRecv,
	ClientDisp
};

class
LatencyData
{
public:
	std::string strCamera;
	long long llLiveRead;
	long long llLiveSend;
	long long llClientRecv;
	long long llClientDisp;

	long long llLRTotal;
	long long llLSTotal;
	long long llCRTotal;
	long long llCDTotal;

	long lLRAvg;
	long lLSAvg;
	long lCRAvg;
	long lCDAvg;

	LatencyData()
		: llLiveRead(0)
		, llLiveSend(0)
		, llClientRecv(0)
		, llClientDisp(0)
		, llLRTotal(0)
		, llLSTotal(0)
		, llCRTotal(0)
		, llCDTotal(0)
		, lLRAvg(0)
		, lLSAvg(0)
		, lCRAvg(0)
		, lCDAvg(0)
	{
	}
	void	update(std::string &strTS, char *pchImage, int len, LatencyType lt, bool &bHasLD);
	void	updateH264(std::string &strTS, char *pchImage, int len, LatencyType lt, bool &bHasLD);
	void	readData(std::string &strComment);
	void	reset()
	{
		llLiveRead	= 0;
		llLiveSend	= 0;
		llClientRecv	= 0;
		llClientDisp	= 0;
		llLRTotal	= 0;
		llLSTotal	= 0;
		llCRTotal	= 0;
		llCDTotal	= 0;
		lLRAvg	= 0;
		lLSAvg	= 0;
		lCRAvg	= 0;
		lCDAvg	= 0;
	}
};


struct oi_rect
{
	int	left;
	int	top;
	int	right;
	int 	bottom;
	oi_rect(int a = 0, int b = 0, int c = 0, int d = 0)
		: left(a)
		, top(b)
		, right(c)
		, bottom(d)
	{
	}
};

typedef std::vector<std::string> StringVector;
typedef std::vector<std::string> __attrs_t;
StringVector	split(const std::string &string, const std::string chrs, int limit = 0);
StringVector	split_str(const std::string &string, const std::string chrs, int limit = 0);
void	StringVectorSort(StringVector &sv);
bool	ProcessExists_(long old_pid, const char *pchProc);
long long GetLL(const char *pch);

const char *	getLongDayOfWeek(int day);
const char *	getShortDayOfWeek(int day);
const char *	getShortMonth(int m);

const	int	getCurrentDayOfWeek();
const	int	getCurrentHour();
const	int	getCurrentMinute();

char *	UpperCaseString(char *pchString);
char *	LowerCaseString(char *pchString);
char *	StringDup(const char *pch);
char *	String_Concat(char *&pchOld, const char *pch);

int	CaseInsensitiveCompare(const char *pch1, const char *pch2, int iLen = 0);
int	CaseSensitiveCompare(const char *pch1, const char *pch2, int iLen = 0);

void	MakeGMTTimeStampFromLongTime(std::string &strTimeStamp, long long llTime);
void	MakeGMTTimeStampFromTime(std::string &strTimeStamp, time_t tStamp, int msecs);
void	MakeGMTTimeStamp(std::string &strTimeStamp, bool bUnique = false);
//void	MakeGMTTimeStamp(std::string &strTimeStamp, std::string &strTZ);
void	MakeLocalTimeStampFromLongTime(std::string &strTimeStamp, long long llTime);
void	MakeLocalTimeStampFromTime(std::string &strTimeStamp, time_t tStamp, int msecs);
void	MakeLocalTimeStamp(std::string &strTimeStamp, bool bUnique = false);
//void	MakeLocalTimeStamp(std::string &strTimeStamp, std::string &strTZ);
void	GetDateString(std::string &strDate);

int GetIntFromDateString(const char *pchDate);
long long GetLongTime();
long long	GetLongTimeFromFilenameString(const char *pchTime);
time_t	GetTimeFromTimeStamp(const char *pchTimestamp);
time_t	GetTimeFromFilenameString(const char *pchLast, time_t &tFile, struct tm &tmLast, long &msec);
time_t	GetTimeFromFilenameString(const char *pchLast);
time_t	GetTimeFromFolderName(const char *pchFolder);
double	GetFreeSpacePercentage(const char *pchPath, bool bUseCache = false);
long long GetDriveCapacity(const char *pchPath, long long &llFreeSpace);
void	FixURL(char *&pchURL);
#ifdef WIN32
void	GetVersion(char *achBuffer, HMODULE hMod = 0);
#else
void	GetVersion(char *achBuffer);
void	GetRevision(char *achBuffer);
#endif
bool	MakeFullPath(const char *pchDir_);
bool	getStreamFileName(std::string &strFile, const char *pchStreamID);
void	xmlExtractNodeContents(const char *pchDoc, const char *pchNode, std::string &strRet, bool bIncludeTag);

class PopulateSubVariable
{
public:
	PopulateSubVariable()
	{
	}
	virtual bool	PopulateData(const char *pchVar, std::string &strData) = 0;
};
void	QuoteQuotes(char pchQ, char *&pch);
void	SubstituteVariable(char const *pchVar, char const *pchValue, char *&pch);
void	SubstituteVariable(char const *pchVar, PopulateSubVariable &popClass, char *&pch);

char *	QuoteXML(const char *pchIn);
void	AddXML(std::string &strXML, const char *tag, const char *val, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, int iVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, float fVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, double dVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, long long llVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, long lVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, bool b, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void	AddXML(std::string &strXML, const char *tag, const std::string &strVal, bool bOmitIfEmpty = false, bool bDontQuoteLTGT = false);
void AddXML(std::string &strXML, const char *tag, std::vector<__attrs_t>
                cSubAttrs, bool bOmitIfEmpty = false, bool bDontQuoteLTGT =
                false);
void AddXML(std::string &strXML, const char *tag, const char* val, __attrs_t
                cTagAttrs, bool bOmitIfEmpty = false, bool bDontQuoteLTGT =
                false);
//void	xmlEasyRead(std::string &strXML, bool bRemoveHeader = false);

void	AddURLVal(std::string &strURL, const char *name, const std::string &strVal);
void	AddURLVal(std::string &strURL, const char *name, const char *val);
void	AddURLVal(std::string &strURL, const char *name, int i);
void	AddURLVal(std::string &strURL, const char *name, long l);
void	AddURLVal(std::string &strURL, const char *name, long long ll);
void	AddURLVal(std::string &strURL, const char *name, float f);

char	*FriendlyBytes(long long llval, bool shortform = false, int digits = 2);

bool	ReadSubHeader(char *pchPtr, char *&newp, int iLen, int &iContentLength);
void	showMemoryProblem();
int	getSecOffset(const char *pch_);
bool	AdjustForTZ(const char *pchSrvOff, const char *pchCamOff, char *achTS, bool bFromClient);
bool	AdjustForTZ(const char *pchSrvOff, const char *pchCamOff, time_t &t, bool bFromClient);

class DataPair
{
private:
	std::string	name;
	std::string	data;
	bool		bDirty;
public:
	DataPair(const char *pchName, const char *pchData)
		: name(pchName ? pchName : "")
		, data(pchData ? pchData : "")
		, bDirty(false)
	{
	}
	const char *getName() const { return name.c_str(); }
	const char *getData() const { return data.c_str(); }
	bool	needsSave() const { return bDirty; }
	void	setDirty(bool b) { bDirty = b; }
	void	setData(const char *pch)
	{
		if (pch)
		{
			if (CaseSensitiveCompare(pch, data.c_str()))
				bDirty = true;
		}
		else
		{
			if (data.length())
				bDirty = true;
		}
		data = pch ? pch : "";
	}
	void	toXML(std::string &strXML) const;
};

class DataValues
{
private:
	std::string strType;
	bool	bDeleted;
	std::vector<DataPair> vPairs;
	std::vector<DataValues *> vdv;
public:
	DataValues &operator=(const DataValues &other);
	DataValues(const DataValues &other);
	~DataValues() { clearChildren(); }
	DataValues() : bDeleted(false) {};
	DataValues(const char *pchType) : bDeleted(false) { strType = pchType ? pchType : ""; }
	DataValues *addChild(const char *pchType);
	void add(const char *pchName, const char *pchVal = 0);
	void addIfNotExists(const char *pchName, const char *pchVal = 0);
	const char *getValue(const char *pchName, const char *pchDef) const;
	const char *getValue(const char *pchName) const;
	int	getIntValue(const char *pchName, int iDef);
	int	getIntValue(const char *pchName);
	bool	getBoolValue(const char *pchName, bool bDef);
	bool	getBoolValue(const char *pchName);
	bool setValue(const char *pchName, const char *pchValue);
	DataPair *getPair(int i) { return &vPairs[i]; }

	void	setDeleted() { bDeleted = true; }
	bool	getDeleted() { return bDeleted; }
	const char *getType() { return strType.c_str(); }
	size_t	numChildren() const { return vdv.size(); }
	DataValues *getChild(size_t i) const { return i < vdv.size() ? vdv[i] : 0; }
	void	toXML(std::string &strXML) const;

	void clearChildren()
	{
		if (vdv.size() == 0)
			return;
		size_t i;
		for(i=0;i<vdv.size();i++)
			if (vdv[i])
				delete vdv[i];
		vdv.clear();
	}

	bool needsSave()
	{
		size_t i;
		if (bDeleted)
			return true;
		for(i=0;i<vPairs.size();i++)
			if (vPairs[i].needsSave())
				return true;
		for(i=0;i<vdv.size();i++)
			if (vdv[i]->needsSave())
				return true;
		return false;
	}
	size_t	size() { return vPairs.size(); }
	void assignFrom(DataValues &other);
};

char *	smakeword(char * word, char stop);
char	x2c(char *what);
void	unescape_url(char *url);
void	plustospace(char *str);

void	ProcessURL(const char *pchBuff, DataValues &dv);

#ifdef WIN32
bool	GetProgramFilesDirectory(std::string &strProgDir);
bool	GetApacheConfig(std::string &strApache);
bool	GetApacheIndex(std::string &strIndex);
#endif

class TempVar
{
private:
	char *pchData;
	char **ppchData;

public:
	TempVar(char **ppchData_) :
		pchData(0),
		ppchData(0)
	{
		if (ppchData_)
		{
			pchData = *ppchData_;
			*ppchData_ = 0;
			ppchData = ppchData_;
		}
	}

	~TempVar()
	{
		if (ppchData)
			*ppchData = pchData;
	}
};

void	setNetworkPathOffline(const char *pch, bool b);
bool	getNetworkPathOffline(const char *pch);
bool	isNetworkPath(const char *pch);
int	network_safe_access(const char *pchFile, int iMode);
void	clearOfflineDriveCache();
void	DeleteFolderAndContents(const char *pchFolder);
int	GetMinutesFromFilenameString(const char *pchLast, int &hour, int &min, int &sec);
int	DayOfWeekFromTime(time_t tFile);
int	HourFromTime(time_t tFile);

#define MAX_WATCHES 10
#define LIVE_MUTEX_SW_ID	0
#define NETWORK_SEND_SW_ID	1
#define CAPTURE_IO_SW_ID	2
#define READ_COMMAND_SW_ID	3

#ifdef WIN32
void UnixTimeToFileTime(time_t t, LPFILETIME pft);
void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst);
#endif

#ifdef WIN32
#include <Windows.h>
class Stopwatch {
	int sw_id;
	LARGE_INTEGER l1, l2;
public:
	Stopwatch(int sw_id_);
	~Stopwatch();
};
#else
class Stopwatch {
	int sw_id;
	struct timeval l1, l2;
public:
	Stopwatch(int sw_id_);
	~Stopwatch();
};
#endif
double getStopwatchSeconds(int sw_id);
void resetStopwatch(int sw_id);

void	trim_crlf(char *buff);
/*
// trim from start
static inline std::string &ltrim(std::string &s)
{
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
        return ltrim(rtrim(s));
}
*/
void	getRobotLiveFileName(const char *pchLivePath, const char *pchRobotID, std::string &strLiveFile);
bool	createCacheImageName(std::string &strURL, std::string &strCacge);

#ifdef WIN32
wchar_t		*CharPointerWideString(char *orig);
char		*WideStringToCharPointer(wchar_t *orig);
#endif

#ifndef WIN32
int	oc_execlp(std::string &resource, StringVector &svArgs, StringVector &svVals);
#endif

#endif
