#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <string.h>
#include <sys/vfs.h>
#include <time.h>
#include <algorithm>
#include <unistd.h>

#include "utils.h"
#include "thread.h"
#include "buffer.h"

StringVector	split(const std::string &string, const std::string chrs, int limit)
{
	StringVector sv;
	std::string temp = string;
	std::string::size_type start = 0;
	std::string::size_type end = 0;

	do
	{
		end = string.find_first_of(chrs, start);
		if (end > 0)
		{
			sv.push_back( string.substr(start, end-start));
		}
		if (end == string.length())
			break;
		start = temp.find_first_not_of(chrs, end);
		if (limit && ( (int)sv.size() == (limit-1)))
		{
			sv.push_back(string.substr(start));
			break;
		}
	} while (start < string.length());
	return sv;
}

StringVector	split_str(const std::string &string_, const std::string chrs, int limit)
{
	StringVector sv;
	std::string::size_type start = 0;
	std::string::size_type end = 0;
	std::string string = string_;

	do
	{
		end = string.find(chrs, start);
		if (end > 0 && end < string.length())
		{
			sv.push_back( string.substr(start, end-start));
			string = string.substr(end-start+chrs.length(), string.length());
		}
	} while (end < string.length());
	sv.push_back(string);
	return sv;
}

void
trim_crlf(char *buff)
{
	while ((buff[strlen(buff)-1] == '\r') || (buff[strlen(buff)-1] == '\n'))
		buff[strlen(buff)-1] = 0;
}

bool
SVSortPredicate(const std::string & lhs, const std::string & rhs)
{
	return ((strcmp(lhs.c_str(), rhs.c_str()) < 0) ? true : false);
}

void
StringVectorSort(StringVector &sv)
{
	std::sort(sv.begin(), sv.end(), SVSortPredicate);
}

long long GetLL(const char *pch)
{
	long long ll = 0;
	sscanf(pch, "%lld", &ll);
	return ll;
}

const char	longDay[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char	shDay[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char	shMon[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

const char *
getLongDayOfWeek(int day)
{
	return longDay[day];
}
const char *
getShortDayOfWeek(int day)
{
	return shDay[day];
}
const char *
getShortMonth(int m)
{
	return shMon[m];
}

const int
getCurrentDayOfWeek()
{
	time_t t;
	time(&t);
	struct tm *tm = localtime(&t);
	return tm->tm_wday;
}
const int
getCurrentHour()
{
	time_t t;
	time(&t);
	struct tm *tm = localtime(&t);
	return tm->tm_hour;
}
const int
getCurrentMinute()
{
	time_t t;
	time(&t);
	struct tm *tm = localtime(&t);
	return tm->tm_min;
}

char *
UpperCaseString(char *pchString)
{
	char	*pch = pchString;

	while (pch && *pch)
	{
		*pch = toupper(*pch);
		pch++;
	}

	return pchString;
}

char *
LowerCaseString(char *pchString)
{
	char	*pch = pchString;

	while (pch && *pch)
	{
		*pch = tolower(*pch);
		pch++;
	}

	return pchString;
}

char *
StringDup(const char *pch)
{
	char *pchNew = 0;
	if (pch)
	{
		pchNew = new char[strlen(pch) + 1];
		strcpy(pchNew, pch);
	}
	return pchNew;
}

char *
String_Concat(char *&pchOld, const char *pch)
{
	char *pchNew = pchOld;
	if (pch)
	{
		char *pchNew = new char[strlen(pchOld) + strlen(pch) + 1];
		strcpy(pchNew, pchOld);
		strcat(pchNew, pch);
		delete [] pchOld;
		pchOld = pchNew;
	}
	return pchNew;
}

int
CaseInsensitiveCompare(const char *pch1, const char *pch2, int iLen)
{
	if (pch1 && pch2)
	{
		if (iLen)
			return strncasecmp(pch1, pch2, iLen);
		else
			return strcasecmp(pch1, pch2);
	}
	return 0;
}

int
CaseSensitiveCompare(const char *pch1, const char *pch2, int iLen)
{
	if (pch1 && pch2)
	{
		if (iLen)
			return strncmp(pch1, pch2, iLen);
		else
			return strcmp(pch1, pch2);
	}
	return 0;
}

void
MakeLocalTimeStampFromTime(std::string &strTimeStamp, time_t tStamp, int msecs)
{
	char achTemp[256];

	struct tm *tm = localtime(&tStamp);

	if (!tm)
		return;

	if (msecs >= 0)
	{
		sprintf(achTemp, "%04d-%02d-%02d %02d-%02d-%02d-%03d",
			tm->tm_year+1900,
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			msecs);
	}
	else
	{
		sprintf(achTemp, "%04d-%02d-%02d %02d-%02d-%02d",
			tm->tm_year+1900,
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);
	}

	strTimeStamp = achTemp;
}
void
MakeGMTTimeStampFromTime(std::string &strTimeStamp, time_t tStamp, int msecs)
{
	char achTemp[256];

	struct tm *tm = gmtime(&tStamp);

	if (!tm)
		return;

	if (msecs >= 0)
	{
		sprintf(achTemp, "%04d-%02d-%02d %02d-%02d-%02d-%03d",
			tm->tm_year+1900,
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			msecs);
	}
	else
	{
		sprintf(achTemp, "%04d-%02d-%02d %02d-%02d-%02d",
			tm->tm_year+1900,
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);
	}

	strTimeStamp = achTemp;
}

void
MakeGMTTimeStampFromLongTime(std::string &strTimeStamp, long long llTime)
{
	time_t t = (time_t)llTime / 1000;
	int msecs = (int)((long long)llTime - (long long)(t * 1000));
	MakeGMTTimeStampFromTime(strTimeStamp, t, msecs);
}

void
MakeLocalTimeStampFromLongTime(std::string &strTimeStamp, long long llTime)
{
	time_t t = (time_t)llTime / 1000;
	int msecs = (int)((long long)llTime - (long long)(t * 1000));
	MakeLocalTimeStampFromTime(strTimeStamp, t, msecs);
}

static char achLastTimeStamp[256];

void
MakeGMTTimeStamp(std::string &strTimeStamp, bool bUnique)
{
	char achTemp[256];

	bool done = false;

	while (!done)
	{
		timeval ts;
		gettimeofday(&ts, 0);
		time_t t;
		time(&t);
		struct tm *tm = gmtime(&t);

		long msecs = ts.tv_usec / 1000;

		sprintf(achTemp, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
				tm->tm_year+1900,
				tm->tm_mon+1,
				tm->tm_mday,
				tm->tm_hour,
				tm->tm_min,
				tm->tm_sec,
				msecs);
		if (!bUnique)
		{
			break;
		}
		else
		{
			if (strcmp(achLastTimeStamp, achTemp))
				break;
			usleep(100);
		}
	}
	strcpy(achLastTimeStamp, achTemp);
	strTimeStamp = achTemp;
}

void
GetDateString(std::string &strDate)
{
	char achTemp[256];

#if defined(WIN32)
	SYSTEMTIME st;
	GetLocalTime(&st);
#else
	time_t t;
	time(&t);
	struct tm *tm = localtime(&t);
#endif

#if defined(WIN32)
	sprintf(achTemp, "%04d-%02d-%02d",
			st.wYear,
			st.wMonth,
			st.wDay);
#else
	sprintf(achTemp, "%04d-%02d-%02d",
			tm->tm_year + 1900,
			tm->tm_mon+1,
			tm->tm_mday);
#endif
	strDate = achTemp;
}

long long
GetLongTime()
{
#if defined(WIN32)
	SYSTEMTIME st;
	GetLocalTime(&st);
	time_t t;
	time(&t);
	long long llMS = (t * 1000) + st.wMilliseconds;
#else
	timeval ts;
	gettimeofday(&ts, 0);
	long long llsec = ts.tv_sec;
	long long llusec = ts.tv_usec;
	long long llMS = (llsec * 1000) + (llusec / 1000);
#endif
	return llMS;
}

long long
GetLongTimeFromFilenameString(const char *pchTime)
{
	if (!pchTime || !*pchTime )
		return -1;

	long msec = 0;
	long long llTime = 0;
	struct tm tmLast;
	memset((void *)&tmLast, 0, sizeof(tmLast));

	sscanf(pchTime, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
			&tmLast.tm_year,
			&tmLast.tm_mon,
			&tmLast.tm_mday,
			&tmLast.tm_hour,
			&tmLast.tm_min,
			&tmLast.tm_sec,
			&msec);

	tmLast.tm_year -= 1900;
	tmLast.tm_mon -= 1;
	time_t tLast = mktime(&tmLast);
	if (tmLast.tm_isdst)
		tLast -= 60 * 60;

	llTime = tLast;
	llTime *= 1000;
	llTime += msec;

	return llTime;
}

int
GetIntFromDateString(const char *pchDate)
{
	if (!pchDate || !*pchDate )
		return -1;

    int year;
    int month;
    int day;
    int ret;
	sscanf(pchDate, "%04d-%02d-%02d",
           &year,
           &month,
           &day);
    ret = year * 10000 + month * 100 + day;
	return ret;
}


time_t
GetTimeFromFilenameString(const char *pchLast, time_t &tFile, struct tm &tmLast, long &msec)
{
	if (!pchLast && tFile == -1)
		return -1;

	time_t tLast = -1;
	if (pchLast)
	{
		memset((void *)&tmLast, 0, sizeof(tmLast));
		sscanf(pchLast, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
			&tmLast.tm_year,
			&tmLast.tm_mon,
			&tmLast.tm_mday,
			&tmLast.tm_hour,
			&tmLast.tm_min,
			&tmLast.tm_sec,
			&msec);
		tmLast.tm_year -= 1900;
		tmLast.tm_mon -= 1;
		tLast = mktime(&tmLast);
		if (tmLast.tm_isdst)
		{
			tLast -= 60 * 60;
		}
		tFile = tLast;
	}
	else if (tFile != -1)
	{
		struct tm *tm = localtime(&tFile);
		tmLast.tm_year = tm->tm_year;
		tmLast.tm_yday = tm->tm_yday;
		tmLast.tm_wday = tm->tm_wday;
		tmLast.tm_sec  = tm->tm_sec;
		tmLast.tm_mon  = tm->tm_mon;
		tmLast.tm_min  = tm->tm_min;
		tmLast.tm_mday = tm->tm_mday;
		tmLast.tm_isdst= tm->tm_isdst;
		tmLast.tm_hour = tm->tm_hour;
		tLast = tFile;
	}
	return tLast;
}

time_t
GetTimeFromFilenameString(const char *pchLast)
{
	time_t tFile = 0;
	struct tm tmLast;
	long msec = 0;
	return GetTimeFromFilenameString(pchLast, tFile, tmLast, msec);
}

time_t
GetTimeFromTimeStamp(const char *pchTimestamp)
{
	if (!pchTimestamp)
		return -1;

	time_t tLast = -1;
	long msec;
	struct tm tmLast;
	if (pchTimestamp)
	{
		memset((void *)&tmLast, 0, sizeof(tmLast));
		sscanf(pchTimestamp, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
			&tmLast.tm_year,
			&tmLast.tm_mon,
			&tmLast.tm_mday,
			&tmLast.tm_hour,
			&tmLast.tm_min,
			&tmLast.tm_sec,
			&msec);
		tmLast.tm_year -= 1900;
		tmLast.tm_mon -= 1;
		tLast = mktime(&tmLast);
		if (tmLast.tm_isdst)
		{
			tLast -= 60 * 60;
		}
	}
	return tLast;
}

time_t
GetTimeFromFolderName(const char *pchFolder)
{
	if (!pchFolder)
		return -1;

	char *pch = StringDup(pchFolder);
	char *p = pch + strlen(pch)-1;
	while (p && p > pch && *p)
	{
		if (*p == '/' || *p == '\\')
		{
			*p = 0;
			break;
		}
		p--;
	}

	int iMins = atoi(p+1);
	int iHrs = iMins / 60;

	iMins = iMins - (iHrs * 60);
	char achTime[256];

	p--;
	while (p && p > pch && *p)
	{
		if (*p == '/' || *p == '\\')
		{
			*p = 0;
			break;
		}
		p--;
	}
	sprintf(achTime, "%s %02d-%02d-00-000", p+1, iHrs, iMins);


	time_t tLast = -1;
	long msec;
	struct tm tmLast;

	memset((void *)&tmLast, 0, sizeof(tmLast));
	sscanf(achTime, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
			&tmLast.tm_year,
			&tmLast.tm_mon,
			&tmLast.tm_mday,
			&tmLast.tm_hour,
			&tmLast.tm_min,
			&tmLast.tm_sec,
			&msec);
	tmLast.tm_year -= 1900;
	tmLast.tm_mon -= 1;
	tLast = mktime(&tmLast);
	if (tmLast.tm_isdst)
	{
		tLast -= 60 * 60;
	}
	delete [] pch;
	return tLast;
}
#if 0
static StringVector saDrives;
static std::vector<double> vFree;
static std::vector<std::string> vOfflineDrives;

void
clearOfflineDriveCache()
{
	vOfflineDrives.clear();
}

double
GetFreeSpacePercentage(const char *pchPath, bool bUseCache)
{
	double dFree = 0.0;
	char ach[1024];
	if (pchPath && *pchPath)
	{
#ifdef WIN32
		strcpy(ach, pchPath);

		if (ach[1] == ':')
			ach[3] = 0;

		if (network_safe_access(ach, 0) < 0)
		{
			return (double)0.0;
		}

		if (bUseCache)
		{
			int i;
			for(i=0;i<(int)saDrives.size();i++)
			{
				if (!CaseSensitiveCompare(saDrives[i].c_str(), ach))
				{
					return vFree[i];
				}
			}
		}

		unsigned __int64 i64FreeBytesToCaller,
			 i64TotalBytes,
			 i64FreeBytes;


		if (GetDiskFreeSpaceExA(ach,
					 (PULARGE_INTEGER)&i64FreeBytesToCaller,
					 (PULARGE_INTEGER)&i64TotalBytes,
					 (PULARGE_INTEGER)&i64FreeBytes))
		{
			dFree = ((double)i64FreeBytes / (double)i64TotalBytes) * 100.0;
		}
#else
		strcpy(ach, pchPath);
		char *p = ach;
		if (*ach == '/')
			p++;

		if (bUseCache)
		{
			int i;
			for(i=0;i<(int)saDrives.size();i++)
			{
				if (!CaseSensitiveCompare(saDrives[i].c_str(), ach))
				{
					return vFree[i];
				}
			}
		}

		if (!access(ach, 0))
		{
			struct statfs st;
			if (statfs(ach, &st) != -1)
			{
				dFree = ((double)st.f_bavail / (double)st.f_blocks) * 100.0;
			}
		}
#endif
	}
	if (bUseCache)
	{
		saDrives.push_back(ach);
		vFree.push_back(dFree);
	}
	return dFree;
}

long long
GetDriveCapacity(const char *pchPath, long long &llFreeSpace)
{
	long long llRet = 0;
	if (pchPath && *pchPath)
	{
		char ach[1024];
#ifdef WIN32
		unsigned __int64 i64FreeBytesToCaller,
			 i64TotalBytes,
			 i64FreeBytes;


		strcpy(ach, pchPath);

		if (ach[1] == ':')
			ach[3] = 0;

		if (GetDiskFreeSpaceExA(ach,
					 (PULARGE_INTEGER)&i64FreeBytesToCaller,
					 (PULARGE_INTEGER)&i64TotalBytes,
					 (PULARGE_INTEGER)&i64FreeBytes))
		{
			llRet = (long long)i64TotalBytes;
			llFreeSpace = (long long)i64FreeBytes;
		}
#else
		strcpy(ach, pchPath);
		char *p = ach;
		if (*ach == '/')
			p++;

		if (!access(ach, 0))
		{
			struct statfs st;
			if (statfs(ach, &st) != -1)
			{
				//fixme get block size properly
				llRet = (long long)st.f_blocks * (long long)st.f_bsize;
				llFreeSpace = (long long)st.f_bavail * (long long)st.f_bsize;
			}
		}
#endif
	}
	return llRet;
}

void
setNetworkPathOffline(const char *pch, bool b)
{
	char ach[1024];
	strcpy(ach, pch);
	char *p = ach;
	int iCount = 0;
	while (p && *p)
	{
		if (*p == '\\')
		{
			iCount++;
			if (iCount == 4)
			{
				*p = 0;
				break;
			}
		}
		p++;
	}
	if (iCount == 4)
	{
		int j;
		bool found = false;
		for(j=0;j<(int)vOfflineDrives.size();j++)
		{
			if(!CaseInsensitiveCompare(vOfflineDrives[j].c_str(), ach))
			{
				found = true;
				break;
			}
		}
		if (!found)
			vOfflineDrives.push_back(ach);
	}
}

bool
getNetworkPathOffline(const char *pch)
{
	char ach[1024];
	strcpy(ach, pch);
	char *p = ach;
	int iCount = 0;
	while (p && *p)
	{
		if (*p == '\\')
		{
			iCount++;
			if (iCount == 4)
			{
				*p = 0;
				break;
			}
		}
		p++;
	}
	if (iCount == 4)
	{
		int j;
		for(j=0;j<(int)vOfflineDrives.size();j++)
		{
			if(!CaseInsensitiveCompare(vOfflineDrives[j].c_str(), ach))
			{
				return true;
			}
		}
	}
	return false;
}

bool isNetworkPath(const char *pch)
{
#ifdef WIN32
	return !CaseInsensitiveCompare(pch, "\\\\", 2);
#else
	return false; //Tian to fix please
#endif
}
#endif
/*
class NetworkFileData
{
public:
	std::string strFile;
	int	iMode;
	bool	bDeletSelf;
	CRITICAL_SECTION csNFD;
	DWORD	dwNFD;
	bool	bOffline;
	int	iResult;

	NetworkFileData(const char *pch, int iMode_)
		:strFile(pch ? pch : "")
		,iMode(iMode_)
		,bDeletSelf(false)
		,dwNFD(1)
		,bOffline(false)
		,iResult(-1)
	{
		InitializeCriticalSectionAndSpinCount(&csNFD, dwNFD);
	}

	~NetworkFileData()
	{
		DeleteCriticalSection(&csNFD);
	}
	int test_access();
};

int NetworkFileData::test_access()
{
	char ach[1024];
	strcpy(ach, strFile.c_str());
	char *p = ach;
	int iCount = 0;
	while (p && *p)
	{
		if (*p == '\\')
		{
			iCount++;
			if (iCount == 4)
			{
				*p = 0;
				break;
			}
		}
		p++;
	}
	if (iCount == 4)
	{
		iResult = _access(ach, iMode);
		if (iResult == -1)
		{
			bOffline = true;
			return iResult;
		}
	}

	iResult = _access(strFile.c_str(), iMode);
	return iResult;
}

void *test_access(void *p)
{
	NetworkFileData *pnfd = (NetworkFileData *)p;
	if (pnfd)
	{
		pnfd->test_access();
		EnterCriticalSection(&pnfd->csNFD);
		if (pnfd->bDeletSelf)
		{
			LeaveCriticalSection(&pnfd->csNFD);
			delete pnfd;
			return 0;
		}
		else
		{
			LeaveCriticalSection(&pnfd->csNFD);
		}
	}
	return 0;
}

int network_safe_access(const char *pch, int iMode)
{
	int iRet = -1;
	if (isNetworkPath(pch))
	{
		if (getNetworkPathOffline(pch))
			return -1;

		NetworkFileData *pnfd = new NetworkFileData(pch, iMode);

		OC_Thread th = 0;
		StartThread(th, (void *)test_access, (void *)pnfd);
		if (th)
		{
			if (WaitForThread(th, 2))
			{
				//success
				iRet = pnfd->iResult;
				if (pnfd->bOffline)
				{
					setNetworkPathOffline(pch, true);
				}
				delete pnfd;
			}
			else
			{
				//network drive probably not available
				EnterCriticalSection(&pnfd->csNFD);
				pnfd->bDeletSelf = true;
				LeaveCriticalSection(&pnfd->csNFD);
				setNetworkPathOffline(pch, true);
#ifdef WIN32
				TerminateThread(th, 0);
#else
#endif
			}
		}

	}
	else
	{
		iRet = _access(pch, iMode);
	}

	return iRet;
}
*/
void
FixURL(char *&pchURL)
{
	if (pchURL)
	{
		char *pchNew = new char[(strlen(pchURL)*4)+1];
		char *p = pchURL;
		char *q = pchNew;
		char	achTemp[10];

		while (p && *p)
		{
			if ((*p > 0 && *p < ' ') ||
				*p == ' '  ||
				*p == '='  ||
				*p == '&'  ||
				*p == '/'  ||
				*p == '#'  ||
				*p == '%'  ||
				*p == '\"' ||
				*p == '\'' ||
				*p == '?'  ||
				*p == '<'  ||
				*p == '>' )
			{
				sprintf(achTemp, "%02x", *p);
				*q++ = '%';
				*q++ = achTemp[0];
				*q++ = achTemp[1];
			}
			else
				*q++ = *p;

			p++;
		}
		*q = 0;

		delete [] pchURL;
		pchURL = pchNew;
	}
}


bool
MakeFullPath(const char *pchDir_)
{
	bool	bOk = true;
	if (pchDir_ && access(pchDir_, 0))
	{
		bOk = true;

		char *pchDir = StringDup(pchDir_);
		if (!CaseInsensitiveCompare(pchDir_, "\\\\", 2))
		{
			//unc path
			int count = 0;
			char *p = pchDir;
			while (p && *p)
			{
				if (*p == '\\')
				{
					count++;
					if (count == 3)
					{
						*p = 0;
						break;
					}
				}
				p++;
			}
			char *q = p+1;
			while (q && *q)
			{
				if (*q == '\\')
					*q = '/';
				q++;
			}
			int i;

			std::string strDir = pchDir;
			strDir += "\\";
			StringVector sv = split(p+1, "/");
			for (i=0;i<(int)sv.size();i++)
			{
				if (i)
					strDir += "/";

				strDir += sv[i];

				if (access(strDir.c_str(), 0))
				{
#ifdef WIN32
					CreateDirectoryA(strDir.c_str(), 0);
#else
					mkdir(strDir.c_str(), 0777);
#endif
					if (access(strDir.c_str(), 0))
					{
						bOk = false;
						break;
					}
				}
			}
			delete [] pchDir;
		}
		else
		{
			char *p = pchDir;
			while (p && *p)
			{
				if (*p == '\\')
					*p = '/';
				p++;
			}
			StringVector sv = split(pchDir, "/");
			delete [] pchDir;

			int i;
#ifdef WIN32
			std::string strDir = "";
			if (*pchDir_ == '/')
				strDir = "/";
#else
			std::string strDir = "/";
#endif

			for (i=0;i<(int)sv.size();i++)
			{
				if (i)
					strDir += "/";

				strDir += sv[i];

				if (access(strDir.c_str(), 0))
				{
#ifdef WIN32
					CreateDirectoryA(strDir.c_str(), 0);
#else
					mkdir(strDir.c_str(), 0777);
#endif
					if (access(strDir.c_str(), 0))
					{
						bOk = false;
						break;
					}
				}
			}
		}
	}
	return bOk;
}


bool
getStreamFileName(std::string &lastFile, const char *pchStreamID)
{
	static std::string strStreamFile;

	if (!strStreamFile.empty())
	{
		lastFile = strStreamFile;
	}
	else
	{
		char ach[128];
		sprintf(ach, "%s", (pchStreamID && *pchStreamID) ? pchStreamID : "no-output");
#ifdef WIN32
		lastFile = "c:/temp";
#else
		lastFile = "/tmp/nvr";
#endif
		if (MakeFullPath(lastFile.c_str()))
		{
			lastFile +=	"/";
			lastFile +=	ach;
			lastFile +=	".jpg";
			strStreamFile = lastFile;
		}
	}
	return true;
}

void
QuoteQuotes(char pchQ, char *&pch)
{
	if (pch && *pch)
	{
		char *pchNew = new char [strlen(pch)*3+1];
		char *p = pch;
		char *q = pchNew;
		while (p && *p)
		{
			if (*p == pchQ)
			{
				*q++ = '\\';
			}
			*q = *p;

			p++;
			q++;
		}
		*q = 0;
		delete [] pch;
		pch = pchNew;
	}
}


void
SubstituteVariable(char const *pchVar, char const *pchValue, char *&pch)
{
	if (!pch)
		return;

	bool done = false;
	bool bDate = false;
	bool bUTC = false;
	bool bNVR = false;
	bool bQuoted = false;

	if (!CaseInsensitiveCompare(pchVar, "%DATETIME%"))
	{
		bDate = true;
	}
	if (!CaseInsensitiveCompare(pchVar, "%UTCDATETIME%"))
	{
		bDate = true;
		bUTC = true;
	}

	if (!CaseInsensitiveCompare(pchVar, "%NVRTIME%"))
	{
		bDate = true;
		bNVR = true;
	}
	if (!CaseInsensitiveCompare(pchVar, "%%NVRTIME%"))
	{
		bDate = true;
		bNVR = true;
		bQuoted = true;
	}

	while (!done)
	{
		done = true;

		char *pchU = StringDup(pch);
		UpperCaseString(pchU);

		char *p = strstr(pchU, pchVar);
		if (p)
		{
			if (bDate)
			{
				p = pch+(p-pchU);
				*p = 0;
				char *pchNew = StringDup(pch);

				char *pchF = 0;
				char *pchTail = 0;
				char *pchFormat = StringDup(p + strlen(pchVar));
				char *end = strchr(pchFormat, '#');
				if (end)
				{
					*end = 0;
					end++;
					pchTail = StringDup(end);
				}
				else
				{
					pchTail = pchFormat;
					pchFormat = 0;
				}

				if (pchFormat)
				{
					if (bNVR)
					{
						time_t t, tF = -1;
						long msec;
						struct tm tms;
						memset(&tms, 0, sizeof(tms));
						std::string strTS = pchValue;
						if (bQuoted)
						{
							StringVector sv = split(pchValue, "%");
							strTS = sv[0];
							strTS += " ";
							strTS += sv[1].c_str() + 2;
						}

						t = GetTimeFromFilenameString(strTS.c_str(), tF, tms, msec);
						t = GetTimeFromFilenameString(0, t, tms, msec);

						if (t > 0)
						{
							char achTime[256];
							strftime(achTime, sizeof(achTime), pchFormat, &tms);
							pchF = StringDup(achTime);

							if (bQuoted)
								FixURL(pchF);
						}
					}
					else
					{
						char achTime[256];
						struct tm *tms;
						time_t t;
						time(&t);
						if (bUTC)
							tms = gmtime(&t);
						else
							tms = localtime(&t);
						strftime(achTime, sizeof(achTime), pchFormat, tms);
						pchF = StringDup(achTime);
					}
				}
				else
				{
					if (bNVR)
					{
						pchF = StringDup(pchValue);
					}
					else
					{
						std::string strTS;
						MakeGMTTimeStamp(strTS);
						pchF = StringDup(strTS.c_str());
					}
				}

				if (pchF)
				{
					String_Concat(pchNew, pchF);
					delete [] pchF;
				}
				String_Concat(pchNew, pchTail);
				delete [] pch;
				if (pchTail)
					delete [] pchTail;
				if (pchFormat)
					delete [] pchFormat;
				pch = pchNew;
				done = false;
			}
			else
			{
				p = pch+(p-pchU);
				*p = 0;
				char *pchNew = StringDup(pch);
				String_Concat(pchNew, pchValue);
				String_Concat(pchNew, p+strlen(pchVar));
				delete [] pch;
				pch = pchNew;
				done = false;
			}
		}
		delete [] pchU;
	}
}

void
SubstituteVariable(char const *pchVar, PopulateSubVariable &popClass, char *&pch)
{
	if (!pch)
		return;

	bool done = false;

	while (!done)
	{
		done = true;

		char *pchU = StringDup(pch);
		UpperCaseString(pchU);

		char *p = strstr(pchU, pchVar);
		if (p)
		{
			p = pch+(p-pchU);
			*p = 0;
			char *pchNew = StringDup(pch);
			std::string strData;
			popClass.PopulateData(pchVar, strData);
			String_Concat(pchNew, strData.c_str());
			String_Concat(pchNew, p+strlen(pchVar));
			delete [] pch;
			pch = pchNew;
			done = false;
		}
		delete [] pchU;
	}
}

bool
NeedsQuoting(const char *pchIn)
{
	return true;
}

char *
QuoteXML(const char *pchIn)
{
	char *pchNew;
	if (NeedsQuoting(pchIn))
	{
		pchNew = new char[strlen(pchIn)*6 + 10];
		const char *p = pchIn;
		char *q = pchNew;
		while (p && *p)
		{
			if (((int)*p < 32) || ((int)*p >= 127))
			{
				char ach[256];
				sprintf(ach, "0x%02X", (int)*p);
				int i = 0;
				while (ach[i])
					*q++ = ach[i++];
				p++;
			}
			else if (*p == '&' && CaseInsensitiveCompare(p, "&amp;", 6) && CaseInsensitiveCompare(p, "&quot;", 6) && CaseInsensitiveCompare(p, "&lt;", 4) && CaseInsensitiveCompare(p, "&gt;", 4))
			{
				*q++ = '&';
				*q++ = 'a';
				*q++ = 'm';
				*q++ = 'p';
				*q++ = ';';
				p++;
			}
			else if (*p == '"')
			{
				*q++ = '&';
				*q++ = 'q';
				*q++ = 'u';
				*q++ = 'o';
				*q++ = 't';
				*q++ = ';';
				p++;
			}
			else if (*p == '<')
			{
				*q++ = '&';
				*q++ = 'l';
				*q++ = 't';
				*q++ = ';';
				p++;
			}
			else if (*p == '>')
			{
				*q++ = '&';
				*q++ = 'g';
				*q++ = 't';
				*q++ = ';';
				p++;
			}
			else
				*q++ = *p++;
		}
		*q = 0;
	}
	else
		pchNew = StringDup(pchIn);
	return pchNew;
}

char *
QuoteXMLNOGTLT(const char *pchIn)
{
	char *pchNew;
	if (NeedsQuoting(pchIn))
	{
		pchNew = new char[strlen(pchIn)*6 + 10];
		const char *p = pchIn;
		char *q = pchNew;
		while (p && *p)
		{
			if (((int)*p < 32) || ((int)*p >= 127))
			{
				char ach[256];
				sprintf(ach, "0x%02X", (int)*p);
				int i = 0;
				while (ach[i])
					*q++ = ach[i++];
				p++;
			}
			else if (*p == '&' && CaseInsensitiveCompare(p, "&amp;", 6) && CaseInsensitiveCompare(p, "&quot;", 6))
			{
				*q++ = '&';
				*q++ = 'a';
				*q++ = 'm';
				*q++ = 'p';
				*q++ = ';';
				p++;
			}
			else if (*p == '"')
			{
				*q++ = '&';
				*q++ = 'q';
				*q++ = 'u';
				*q++ = 'o';
				*q++ = 't';
				*q++ = ';';
				p++;
			}
			else
				*q++ = *p++;
		}
		*q = 0;
	}
	else
		pchNew = StringDup(pchIn);
	return pchNew;
}


void
AddXML(std::string &strXML, const char *tag, const char *val, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && (!val || !*val))
		return;

	strXML += "<";
	strXML += tag;
	strXML += ">";
	char *pchVal = 0;
	if (bDontQuoteLTGT)
		pchVal = QuoteXMLNOGTLT(val ? val : "");
	else
		pchVal = QuoteXML(val ? val : "");
	strXML += pchVal;
	delete [] pchVal;
	strXML += "</";
	strXML += tag;
	strXML += ">";
}

void
AddXML(std::string &strXML, const char *tag, std::vector<__attrs_t>
                cAttrsInSubtags, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
        //if (bOmitIfEmpty)
                //return;

	strXML += "<";
	strXML += tag;
	strXML += ">";
        for (size_t i = 0; i < cAttrsInSubtags.size(); i++)
        {
                strXML += "<Process pid=\"";
                strXML += cAttrsInSubtags[i][0];
                strXML += "\" name=\"";
                strXML += cAttrsInSubtags[i][1];
                strXML += "\" cpu=\"";
                strXML += cAttrsInSubtags[i][2];
                strXML += "\" memory=\"";
                strXML += cAttrsInSubtags[i][3];
                strXML += "\"/>";
        }
	strXML += "</";
	strXML += tag;
	strXML += ">";
}

void
AddXML(std::string &strXML, const char *tag, const char* val, __attrs_t
                cTagAttrs, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty)
		return;

	strXML += "<";
	strXML += tag;
        strXML += " us=\"";
        strXML += cTagAttrs[0];
        strXML += "\" sy=\"";
        strXML += cTagAttrs[1];
        strXML += "\" ni=\"";
        strXML += cTagAttrs[2];
        strXML += "\" wa=\"";
        strXML += cTagAttrs[3];
        strXML += "\" hi=\"";
        strXML += cTagAttrs[4];
        strXML += "\" si=\"";
        strXML += cTagAttrs[5];
        strXML += "\" st=\"";
        strXML += cTagAttrs[6];
        strXML += "\">";
	char *pchVal = 0;
	if (bDontQuoteLTGT)
		pchVal = QuoteXMLNOGTLT(val ? val : "");
	else
		pchVal = QuoteXML(val ? val : "");
	strXML += pchVal;
	delete [] pchVal;
	strXML += "</";
	strXML += tag;
	strXML += ">";
}

void
AddXML(std::string &strXML, const char *tag, const std::string &strVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	AddXML(strXML, tag, strVal.c_str(), bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, int iVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && iVal == 0)
		return;

	char ach[256];
	sprintf(ach, "%d", iVal);
	AddXML(strXML, tag, ach, bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, float fVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && fVal == 0.0)
		return;

	char ach[256];
	sprintf(ach, "%f", fVal);
	AddXML(strXML, tag, ach, bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, double dVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && dVal == 0.0)
		return;

	char ach[256];
	sprintf(ach, "%f", dVal);
	if (strlen(ach) > 7 && !CaseInsensitiveCompare(ach+strlen(ach)-7, ".000000"))
		ach[strlen(ach)-7] = 0;
	AddXML(strXML, tag, ach, bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, long lVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && lVal == 0)
		return;

	char ach[256];
	sprintf(ach, "%ld", lVal);
	AddXML(strXML, tag, ach, bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, long long llVal, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	if (bOmitIfEmpty && llVal == 0)
		return;

	char ach[256];
	sprintf(ach, "%lld", llVal);
	AddXML(strXML, tag, ach, bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddXML(std::string &strXML, const char *tag, bool b, bool bOmitIfEmpty, bool bDontQuoteLTGT)
{
	AddXML(strXML, tag, b ? "True" : "False", bOmitIfEmpty, bDontQuoteLTGT);
}

void
AddURLVal(std::string &strURL, const char *name, const char *val)
{
	if (val && *val)
	{
		strURL += "&";
		strURL += name;
		strURL += "=";
		char *pchVal = val ? StringDup(val) : StringDup("");
		FixURL(pchVal);
		strURL += pchVal;
		delete [] pchVal;
	}
}

void
AddURLVal(std::string &strURL, const char *name, const std::string &strVal)
{
	AddURLVal(strURL, name, strVal.c_str());
}

void
AddURLVal(std::string &strURL, const char *name, int i)
{
	char ach[32];
	sprintf(ach, "%d", i);
	AddURLVal(strURL, name, ach);
}

void
AddURLVal(std::string &strURL, const char *name, long l)
{
	char ach[32];
	sprintf(ach, "%ld", l);
	AddURLVal(strURL, name, ach);
}

void
AddURLVal(std::string &strURL, const char *name, long long ll)
{
	char ach[64];
	sprintf(ach, "%lld", ll);
	AddURLVal(strURL, name, ach);
}

void
AddURLVal(std::string &strURL, const char *name, float f)
{
	char ach[64];
	sprintf(ach, "%f", f);
	AddURLVal(strURL, name, ach);
}

char achBytes[256];
char *FriendlyBytes(long long llval, bool shortform, int digits)
{
	*achBytes = 0;
	double d = (double)llval / 1024;
	int iUnits = 0;
	if ( d > 1024)
	{
		d = d / 1024;
		iUnits = 1;
		if ( d > 1024)
		{
			iUnits = 2;
			d = d / 1024;
			if ( d > 1024)
			{
				iUnits = 3;
				d = d / 1024;
			}
		}
	}
	if (digits == 0)
	{
		if (shortform)
			sprintf(achBytes, "%.0f%s", d, iUnits == 3 ? "T" : iUnits == 2 ? "G" : iUnits == 1 ? "M" : "K");
		else
			sprintf(achBytes, "%.0f%s (%lld bytes)", d, iUnits == 3 ? "T" : iUnits == 2 ? "G" : iUnits == 1 ? "M" : "K", llval);
	}
	else
	{
		char achF[256];
		if (shortform)
		{
			sprintf(achF, "%%.%df%%s", digits);
			sprintf(achBytes, achF, d, iUnits == 3 ? "T" : iUnits == 2 ? "G" : iUnits == 1 ? "M" : "K");
		}
		else
		{
			sprintf(achF, "%%.%df%%s (%%lld bytes)", digits);
			sprintf(achBytes, achF, d, iUnits == 3 ? "T" : iUnits == 2 ? "G" : iUnits == 1 ? "M" : "K", llval);
		}
	}
	return achBytes;
}

char *EndOfLine(char *p, int iLen)
{
	char *q = p;
	while (q && *q && *q != '\r' && *q != '\n')
	{
		q++;
		iLen--;
		if (iLen == 0)
			return 0;
	}
	return q;
}

bool
ReadSubHeader(char *pchPtr, char *&newp, int iLen, int &iContentLength)
{
	bool	ok = false;

	iContentLength = -1;

	char *p = pchPtr;

	while (p && *p && !ok && iLen)
	{
		if (!strncasecmp(p, "\n\n", 2))
		{
			newp = p + 2;
			return true;
		}
		else if (!strncasecmp(p, "\r\n\r\n", 4))
		{
			newp = p + 4;
			return true;
		}
		else if (!strncasecmp(p, "\r\n\n", 3))
		{
			newp = p + 3;
			return true;
		}

		if (!strncasecmp(p, "Content-Length: ", 16))
		{
			char achTemp[64];
			memset(achTemp, 0, sizeof(achTemp));
			char *q = EndOfLine(p, iLen);
			if (q && *q && ((q-p) < (int)(sizeof(achTemp)-1)))
			{
				strncpy(achTemp, p + 16, q-p - 16);
				iContentLength = atoi(achTemp);
				p = q;
			}
			else
			{
				iLen--;
				p++;
			}
		}
		else if (!strncasecmp(p, "DataLen: ", 9)) //sony cameras
		{
			char achTemp[64];
			memset(achTemp, 0, sizeof(achTemp));
			char *q = EndOfLine(p, iLen);
			if (q && *q && ((q-p) < (int)(sizeof(achTemp)-1)))
			{
				strncpy(achTemp, p + 9, q-p - 9);
				iContentLength = atoi(achTemp);
				p = q;
			}
			else
			{
				iLen--;
				p++;
			}
		}
		else
		{
			iLen--;
			p++;
		}
	}
	return ok;
}

static bool bShown = false;
void
showMemoryProblem()
{
	if (!bShown)
	{
#ifdef WIN32
#ifdef HAS_UI
		MessageBox(0, "Out of memory", "Error", 0);
#endif
#endif
		bShown = true;
	}
}


#ifdef WIN32
bool
GetProgramFilesDirectory(std::string &strProgDir)
{
	char achDir[512];
	*achDir = 0;

	GetEnvironmentVariableA("ProgramFiles(x86)", achDir, sizeof(achDir));
	if (*achDir && !access(achDir, 0))
	{
		strProgDir = achDir;
		return true;
	}
	GetEnvironmentVariableA("ProgramFiles", achDir, sizeof(achDir));
	if (*achDir && !access(achDir, 0))
	{
		strProgDir = achDir;
		return true;
	}
	return false;
}
bool
GetApacheConfig(std::string &strApache)
{
	bool bHasApache = false;
	strApache = "c:/xampp/apache/conf/httpd.conf";
	if (!access(strApache.c_str(), 0))
	{
		bHasApache = true;
	}
	return bHasApache;
}
bool
GetApacheIndex(std::string &strIndex)
{
	bool bHasApache = false;
	strIndex = "c:/xampp/htdocs/index.html";
	if (!access(strIndex.c_str(), 0))
	{
		bHasApache = true;
	}
	return bHasApache;
}
/*
bool
GetApacheConfig(std::string &strApache)
{
	bool bHasApache = false;
	if (GetProgramFilesDirectory(strApache))
	{
		strApache += "/Apache Software Foundation/Apache2.2/conf/httpd.conf";
		if (!access(strApache.c_str(), 0))
		{
			bHasApache = true;
		}
	}
	return bHasApache;
}
bool
GetApacheIndex(std::string &strIndex)
{
	bool bHasApache = false;
	if (GetProgramFilesDirectory(strIndex))
	{
		strIndex += "/Apache Software Foundation/Apache2.2/htdocs/index.html";
		if (!access(strIndex.c_str(), 0))
		{
			bHasApache = true;
		}
	}
	return bHasApache;
}
*/
#endif

void
DeleteFolderAndContents(const char *pchFolder)
{
#ifdef WIN32
	char ach[1024];

	std::string strPattern = pchFolder;
	strPattern += "/*.*";

	WIN32_FIND_DATAA fileData;
	HANDLE hFile = FindFirstFileA(strPattern.c_str(), &fileData);

	if (hFile == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			strcpy(ach, pchFolder);
			strcat(ach, "/");
			strcat(ach, fileData.cFileName);
			DeleteFileA(ach);
		}
		else
		{
			if (*fileData.cFileName != '.')
			{
				strcpy(ach, pchFolder);
				strcat(ach, "/");
				strcat(ach, fileData.cFileName);
				DeleteFolderAndContents(ach);
				RemoveDirectoryA(ach);
			}
		}
	} while(FindNextFileA(hFile, &fileData));

	FindClose(hFile);
	RemoveDirectoryA(pchFolder);
#else
#endif
}

#ifdef WIN32
LARGE_INTEGER sw_totals[MAX_WATCHES];
Stopwatch::Stopwatch(int sw_id_)
	:sw_id(sw_id_)
{
	QueryPerformanceCounter(&l1);
}

Stopwatch::~Stopwatch()
{
	QueryPerformanceCounter(&l2);
	sw_totals[sw_id].QuadPart += l2.QuadPart - l1.QuadPart;
}

double getStopwatchSeconds(int sw_id)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	double d = (double)sw_totals[sw_id].QuadPart / (double)freq.QuadPart;
	return d;
}

void resetStopwatch(int sw_id)
{
	if (sw_id >= 0 && sw_id < MAX_WATCHES)
		sw_totals[sw_id].QuadPart = 0;
}
#else
long long sw_totals[MAX_WATCHES];
Stopwatch::Stopwatch(int sw_id_)
	:sw_id(sw_id_)
{
	gettimeofday(&l1, 0);
}

Stopwatch::~Stopwatch()
{
	gettimeofday(&l2, 0);
	long long ll1 = (l1.tv_sec * 1000) + l1.tv_usec;
	long long ll2 = (l2.tv_sec * 1000) + l2.tv_usec;
	sw_totals[sw_id] += ll2 - ll1;
}

double getStopwatchSeconds(int sw_id)
{
	double d = (double)sw_totals[sw_id] / (double) 1000000;
	return d;
}

void resetStopwatch(int sw_id)
{
	if (sw_id >= 0 && sw_id < MAX_WATCHES)
		sw_totals[sw_id] = 0;
}
#endif

#ifndef WIN32
#define PTHREAD_JOIN_POLL_INTERVAL 10

typedef struct _waitData waitData;

struct _waitData
{
	pthread_t waitID;
	pthread_t helpID;
	int done;
};

void
sleep_msecs(int msecs)
{
	struct timeval tv;

	tv.tv_sec = msecs/1000;
	tv.tv_usec = (msecs % 1000) * 1000;
	select (0,NULL,NULL,NULL,&tv);
}
unsigned int
get_ticks()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_usec/1000 + tv.tv_sec * 1000);
}

void *
join_timeout_helper(void *arg)
{
	waitData *data = (waitData*)arg;

	pthread_join(data->waitID, NULL);
	data->done = true;
	return (void *)0;
}

int
pthread_join_timeout(pthread_t wid, int msecs)
{
	pthread_t id;
	waitData data;
	unsigned int start = get_ticks();
	int timedOut = false;

	data.waitID = wid;
	data.done = false;

	if (pthread_create(&id, NULL, join_timeout_helper, &data) != 0)
		return (-1);
	do {
		if (data.done)
			break;
		/* you could also yield to your message loop here... */
		sleep_msecs(PTHREAD_JOIN_POLL_INTERVAL);
	} while ((int)((get_ticks() - start)) < msecs);
	if (!data.done)
	{
		pthread_cancel(id);
		timedOut = true;
	}
	/* free helper thread resources */
	pthread_join(id, NULL);
	return (timedOut);
}
#endif

int
getSecOffset(const char *pch_)
{
	char *pch = (char *)pch_;

	int offset = 0;
	bool bNeg = false;

	if (*pch == '-')
	{
		bNeg = true;
		pch++;
	}
	else if (*pch == '+')
	{
		pch++;
	}

	if (strlen(pch) == 4)
	{
		char achH[3];
		char achM[3];

		achH[0] = pch[0];
		achH[1] = pch[1];
		achH[2] = 0;
		achM[0] = pch[2];
		achM[1] = pch[3];
		achM[2] = 0;
		offset = (atoi(achH) * 3600) + (atoi(achM) * 60);
	}
	else
	{
		offset = atoi(pch) * 3600;
	}
	return (bNeg ? -offset : offset);
}

bool
AdjustForTZ(const char *pchSrvOff, const char *pchCamOff, char *achTS, bool bFromClient)
{
	if (!pchSrvOff || !pchCamOff || !*pchSrvOff || !*pchCamOff)
		return false;

	int offsetC = getSecOffset(pchCamOff);
	int offsetS = getSecOffset(pchSrvOff);

	int iDiff = offsetC - offsetS;
	if (iDiff)
	{
		time_t t, tF = -1;
		long msec;
		struct tm tms;
		memset(&tms, 0, sizeof(tms));
		t = GetTimeFromFilenameString(achTS, tF, tms, msec);
		t = GetTimeFromFilenameString(0, t, tms, msec);

		if (t > 0)
		{
			if (bFromClient)
				t -= iDiff;
			else
				t += iDiff;
		}
		if (t > 0)
		{
			std::string strTS;
			MakeGMTTimeStampFromTime(strTS, t, msec);
			strcpy(achTS, strTS.c_str());
			return true;
		}
	}
	return false;
}

bool
AdjustForTZ(const char *pchSrvOff, const char *pchCamOff, time_t &t, bool bFromClient)
{
	if (!pchSrvOff || !pchCamOff || !*pchSrvOff || !*pchCamOff)
		return false;

	int offsetC = getSecOffset(pchCamOff);
	int offsetS = getSecOffset(pchSrvOff);

	int iDiff = offsetC - offsetS;
	if (iDiff)
	{
		if (bFromClient)
			t -= iDiff;
		else
			t += iDiff;
		return true;
	}
	return false;
}

int
GetMinutesFromFilenameString(const char *pchLast, int &hour, int &min, int &sec)
{
	struct tm tmLast;

	if (!pchLast)
		return -1;

	long msec;
	if (pchLast)
	{
		memset((void *)&tmLast, 0, sizeof(tmLast));
		sscanf(pchLast, "%04d-%02d-%02d %02d-%02d-%02d-%03ld",
			&tmLast.tm_year,
			&tmLast.tm_mon,
			&tmLast.tm_mday,
			&tmLast.tm_hour,
			&tmLast.tm_min,
			&tmLast.tm_sec,
			&msec);
	}
	hour = tmLast.tm_hour;
	min = tmLast.tm_min;
	sec = tmLast.tm_sec;

	return tmLast.tm_min;
}

int
DayOfWeekFromTime(time_t tFile)
{
	struct tm *tm = localtime(&tFile);
	return tm->tm_wday;
}

int
HourFromTime(time_t tFile)
{
	struct tm *tm = localtime(&tFile);
	return tm->tm_hour;
}

char *	smakeword(char * word, char stop)
{
	while (*word && *word != stop) word++;
	if (*word)
		*word++ = '\0';
	return word;
}

char	x2c(char *what)
{
	register char digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
	return(digit);
}

void	unescape_url(char *url)
{
	register int x,y;

	for(x=0,y=0;url[y];++x,++y)
	{
		if((url[x] = url[y]) == '%')
		{
			url[x] = x2c(&url[y+1]);
			y+=2;
		}
	}
	url[x] = '\0';
}

void	plustospace(char *str)
{
	while (*str)
	{
		if (*str == '+')
			*str = ' ';
		str++;
	}
}

DataValues *
DataValues::addChild(const char *pchType)
{
	DataValues *pdv = new DataValues(pchType);
	vdv.push_back(pdv);
	return pdv;
}

void
DataValues::addIfNotExists(const char *pchName, const char *pchVal)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(vPairs[i].getName(), pchName))
			return;
	}
	DataPair dp(pchName, pchVal);
	vPairs.push_back(dp);
}

void
DataValues::add(const char *pchName, const char *pchVal)
{
	DataPair dp(pchName, pchVal);
	vPairs.push_back(dp);
}

int
DataValues::getIntValue(const char *pchName)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
			return atoi(vPairs[i].getData());
	}
	return 0;
}

bool
DataValues::getBoolValue(const char *pchName)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
		{
			const char *pch = vPairs[i].getData();
			if (pch && (*pch == 'T' || *pch == 't' || *pch == 'Y' || *pch == 'y' || *pch == '1'))
				return true;
			else
				return false;
		}

	}
	return false;
}

int
DataValues::getIntValue(const char *pchName, int iDef)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
		{
			if (strlen(vPairs[i].getData()))
				return atoi(vPairs[i].getData());
			else
				break;
		}
	}
	return iDef;
}

bool
DataValues::getBoolValue(const char *pchName, bool bDef)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
		{
			const char *pch = vPairs[i].getData();
			if (pch && (*pch == 'T' || *pch == 't' || *pch == 'Y' || *pch == 'y' || *pch == '1'))
				return true;
			else
				return false;
		}

	}
	return bDef;
}


const char *
DataValues::getValue(const char *pchName) const
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
			return vPairs[i].getData();
	}
	return "";
}

const char *
DataValues::getValue(const char *pchName, const char *pchDef) const
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
		{
			if (strlen(vPairs[i].getData()))
				return vPairs[i].getData();
			else
				return pchDef;
		}
	}
	return pchDef;
}


bool
DataValues::setValue(const char *pchName, const char *pchValue)
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
	{
		if (!CaseInsensitiveCompare(pchName, vPairs[i].getName()))
		{
			vPairs[i].setData(pchValue);
			return true;
		}
	}
	//this will make it dirty
	DataPair dp(pchName, "");
	dp.setData(pchValue);
	vPairs.push_back(dp);
	return false;
}

DataValues::DataValues(const DataValues &other)
{
	strType = other.strType;
	bDeleted = other.bDeleted;
	vPairs = other.vPairs;

	size_t i;
	for(i=0;i<other.numChildren();i++)
	{
		DataValues *pNew = new DataValues(*other.getChild(i));
		vdv.push_back(pNew);
	}
}

DataValues &
DataValues::operator=(const DataValues &other)
{
	clearChildren();

	strType = other.strType;
	bDeleted = other.bDeleted;
	vPairs = other.vPairs;

	size_t i;
	for(i=0;i<other.numChildren();i++)
	{
		DataValues *pNew = new DataValues(*other.getChild(i));
		vdv.push_back(pNew);
	}
	return *this;
}

void
DataValues::assignFrom(DataValues &other)
{
	size_t i;
	for(i=0;i<other.vPairs.size();i++)
	{
		setValue(other.vPairs[i].getName(), other.vPairs[i].getData());
	}

	bDeleted = other.bDeleted;

	for(i=0;i<other.numChildren();i++)
	{
		DataValues *pdvo = other.getChild(i);
		DataValues *pdv = 0;

		if (numChildren() > i)
		{
			pdv = getChild(i);
		}
		else
		{
			pdv = addChild(pdvo->getType());
		}
		if (pdv)
			pdv->assignFrom(*pdvo);
	}
}

void
DataPair::toXML(std::string &strXML) const
{
	AddXML(strXML, name.c_str(), data.c_str());
}

void
DataValues::toXML(std::string &strXML) const
{
	size_t i;
	for(i=0;i<vPairs.size();i++)
		vPairs[i].toXML(strXML);
}

void
ProcessURL(const char *pchBuff, DataValues &dv)
{
	char	*cp,*t1,*t2;

	if (pchBuff)
	{
		char *cp2 = (char *)pchBuff;
		while (cp2)
		{
			cp = StringDup(cp2 ? cp2 :"");
			cp2 = cp;
			while(cp && *cp)
			{
				t1 = cp;
				cp = smakeword(cp, '&');
				plustospace(t1);
				unescape_url(t1);
				t2 = smakeword(t1,'=');
				if (t1 && *t1)
				{
					if (dv.setValue(t1, t2))
						continue;
				}
			}
			delete [] cp2;
			cp2 = 0;
		}
	}
}

void
xmlExtractNodeContents(const char *pchDoc, const char *pchNode, std::string &strRet, bool bIncludeTag)
{
	Buffer b;
	b.Add(pchDoc, strlen(pchDoc));
	std::string strStart = "<";
	strStart += pchNode;
	strStart += ">";

	std::string strEnd = "</";
	strEnd += pchNode;
	strEnd += ">";

	int iStart = b.locate(strStart.c_str());
	if (iStart != -1)
	{
		int iEnd = b.locate(strEnd.c_str());
		if (iEnd != -1)
		{
			int iLen = iEnd - iStart + strEnd.length();

			char *pchNew = new char [iLen+1];
			if (bIncludeTag)
			{
				memcpy(pchNew, b.getData() + iStart, iLen);
				pchNew[iLen] = 0;
			}
			else
			{
				memcpy(pchNew, b.getData() + iStart + strStart.length(), iLen - strStart.length() - strEnd.length());
				pchNew[iLen - strStart.length() - strEnd.length()] = 0;
			}
			strRet = pchNew;
			delete [] pchNew;
		}
	}
}

bool
isWhite(char c)
{
	if (c == '\n' || c == '\r' || c == '\t' || c == ' ')
		return true;
	return false;
}

//dont use broken if tag is empty or has no data
/*
void
xmlEasyRead(std::string &strXML, bool bRemoveHeader)
{
	std::string strNew;
	int iLevel = -1;
	char *pch = StringDup(strXML.c_str());
	char *p = pch;
	Buffer b;
	char ach[2];
	bool bInTag = false;
	bool bInData = false;

	char achPad[1024];

	while (p && *p)
	{
		if (*p == '\r' || *p == '\n')
		{
		}
		else if (*p == ' ' || *p == '\t')
		{
			if (bInData)
			{
				ach[0] = *p;
				ach[1] = 0;
				b.Add(ach, 1);
			}
			else if (bInTag)
			{
				ach[0] = *p;
				ach[1] = 0;
				b.Add(ach, 1);
			}
			else
			{
			}
		}
		else if (*p == '>')
		{
			bInTag = false;
			b.Add(">",1);
			if (*(p+1) == '<' || isWhite(*(p+1)))
			{
				b.Add("\n", 1);
			}
			else
			{
				bInData = true;
			}
		}
		else if (*p == '<')
		{
			if (bRemoveHeader)
			{
				if (*(p+1) == '?')
				{
					while (p && *p)
					{
						if (*p == '>' && *(p-1) == '?')
						{
							break;
						}
						p++;
					}
					while (p && *p)
					{
						if (*p == '<')
							break;
						p++;
					}
				}
			}

			bInTag = true;
			int i = 0;
			if (*(p+1) == '/')
			{
				if (*(p-1) == '>' || isWhite(*(p-1)))
				{
					for(i=0;i<iLevel*2;i++)
					{
						achPad[i] = ' ';
					}
				}
				iLevel--;
			}
			else
			{
				iLevel++;
				if (*(p-1) == '>' || isWhite(*(p-1)))
				{
					for(i=0;i<iLevel*2;i++)
					{
						achPad[i] = ' ';
					}
				}
			}
			achPad[i] = 0;
			b.Add(achPad, strlen(achPad));
			b.Add("<", 1);
		}
		else
		{
			ach[0] = *p;
			ach[1] = 0;
			b.Add(ach, 1);
		}

		p++;
	}

	delete [] pch;
	b.Add("", 1);

	strXML = b.getData();
}
*/

#ifndef WIN32

static 	int nFailures = 0;

int	pthread_join_timeout(pthread_t wid, int msecs);

class p_data
{
public:
       	pid_t	old_pid;
	char	achProc[64];
	bool	bExists;

	p_data(pid_t pid_old, const char *pchProc)
	{
		bExists = false;
		old_pid = pid_old;
		strncpy(achProc, pchProc, sizeof(achProc));
	}
};

void *
process_exists_(void *p)
{
	p_data *pd = (p_data *)p;

	FILE *fp;
	char achTest[256];
	sprintf(achTest, "pidof %s", pd->achProc);

	if ((fp = popen(achTest, "r")) != 0)
	{
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		fgets(buff, sizeof(buff), fp);
		if (strlen(buff))
		{
			trim_crlf(buff);
			int i;
			StringVector sv = split(buff, " ");
			for (i=0;i<(int)sv.size();i++)
			{
				long lpid = atol(sv[i].c_str());
				if (lpid == pd->old_pid)
				{
					//old process is still running
					pd->bExists = true;
					break;
				}
			}
		}
		else
		{
			pd->bExists = false;
		}
		fclose(fp);
	}
	else
	{
		pd->bExists = false;
	}
	nFailures--;
	return 0;
}

#endif

bool
ProcessExists_(long old_pid, const char *pchProc)
{
	bool bExists = false;
	nFailures++;
	pthread_t thread = 0;
	p_data *pd = new p_data(old_pid, pchProc);
	pthread_create(&thread, NULL, (pthread_func)process_exists_, pd);
	if (thread)
	{
		int retVal = !pthread_join_timeout(thread, 2000);
		if (!retVal)
		{
			//syslog(LOG_ERR, "failed to join thread in ProcessExists_ for process %s with pid %d failures = %d", pchProc, old_pid, nFailures);
			pthread_detach(thread);
		}
		else
		{
			bExists = pd->bExists;
		}
	}
	return bExists;
}


static void
safeName(char *pch)
{
	char *p = pch;
	while (p && *p)
	{
		if (*p == '&' || *p == ':' || *p == '\\' || *p == '/' || *p == ' ')
			*p = '_';
		p++;
	}
}

bool
createCacheImageName(std::string &strURL, std::string &strCache)
{
	bool bOk = false;

	const char *pch =  strstr(strURL.c_str(), "?getimage");

	char achCamera[256];
	char achTime[256];
	char achCrop[256];
	char achQual[256];

	*achCamera = 0;
	*achTime = 0;
	*achCrop = 0;
	*achQual = 0;

	if (pch)
	{
		char achURL[2048];
		strcpy(achURL, pch+9);
		char *pchEnd = (char *)strstr(achURL, " HTTP");
		if (pchEnd)
			*pchEnd = 0;

		StringVector sv0 = split(achURL, "&");
		size_t i;
		for(i=0;i<sv0.size();i++)
		{
			StringVector sv1 = split(sv0[i].c_str(), "=");
			if (sv1.size() == 2)
			{
				if (!CaseInsensitiveCompare(sv1[0].c_str(), "camera"))
				{
					strcpy(achCamera, sv1[1].c_str());
					plustospace(achCamera);
					unescape_url(achCamera);
					safeName(achCamera);
				}
				else if (!CaseInsensitiveCompare(sv1[0].c_str(), "time"))
				{
					strcpy(achTime, sv1[1].c_str());
					plustospace(achTime);
					unescape_url(achTime);
					safeName(achTime);
				}
				else if (!CaseInsensitiveCompare(sv1[0].c_str(), "cropval"))
				{
					strcpy(achCrop, sv1[1].c_str());
					plustospace(achCrop);
					unescape_url(achCrop);
					safeName(achCrop);
				}
				else if (!CaseInsensitiveCompare(sv1[0].c_str(), "quality"))
				{
					strcpy(achQual, sv1[1].c_str());
					plustospace(achQual);
					unescape_url(achQual);
					safeName(achQual);
				}
			}
		}
	}

	if (*achCamera && *achTime)
		bOk = true;

	strCache += achCamera;
	strCache += "_";
	strCache += achTime;
	if (*achCrop)
	{
		strCache += "_";
		strCache += achCrop;
	}
	if (*achQual)
	{
		strCache += "_";
		strCache += achQual;
	}
	strCache += ".jpg";

	return bOk;
}

static unsigned char	achCom[3] = {0xff, 0xfe, 0x00};

void
LatencyData::update(std::string &strTS, char *pchImage, int len, LatencyType lt, bool &bHasLD)
{
	std::string strNow;
	MakeGMTTimeStamp(strNow);

	int i;
	if (strTS.length() == 0)
	{
		char achTS[32];
		for(i=len-12;i>0;i--)
		{
			if (!memcmp(pchImage+i, achCom, 2))
			{
				memcpy(achTS, pchImage+i+2, 23);
				achTS[23] = 0;
				strTS = achTS;
				break;
			}
		}
	}

	if (strTS.length() == 0)
		return;

	long long llNow = GetLongTimeFromFilenameString(strNow.c_str());
	long long llCap = GetLongTimeFromFilenameString(strTS.c_str());
	long long llDiff = llNow - llCap;

	char ach[256];
	bool found = false;
	const char *pch1 = 0;
	const char *pch2 = 0;

	switch(lt)
	{
	case LiveRead:
		for(i=len-2;i>0;i--)
		{
			if (!memcmp(pchImage+i, achCom, 2))
				break;
			if ((len-i >= 36) && !memcmp(pchImage+i, "<LRTS>", 6))
			{
				found = true;
				if (!memcmp(pchImage+i+6, "0000-00-00 00-00-00-000", 23))
				{
					pch1 = pchImage + i;
				}
				break;
			}
			if ((len-i >= 17) && !memcmp(pchImage+i, "<LRA>", 5))
				pch2 = pchImage + i;
		}
		llLiveRead++;
		llLRTotal += llDiff;
		lLRAvg = (long)(llLRTotal / llLiveRead);
		sprintf(ach, "%06ld", lLRAvg);
		break;
	case LiveSend:
		for(i=len-2;i>0;i--)
		{
			if (!memcmp(pchImage+i, achCom, 2))
				break;
			if ((len-i >= 36) && !memcmp(pchImage+i, "<LSTS>", 6))
			{
				found = true;
				if (!memcmp(pchImage+i+6, "0000-00-00 00-00-00-000", 23))
				{
					pch1 = pchImage + i;
				}
				break;
			}
			if ((len-i >= 17) && !memcmp(pchImage+i, "<LSA>", 5))
				pch2 = pchImage + i;
		}
		llLiveSend++;
		llLSTotal += llDiff;
		lLSAvg = (long)(llLSTotal / llLiveSend);
		sprintf(ach, "%06ld", lLSAvg);
		break;
	case ClientRecv:
		for(i=len-2;i>0;i--)
		{
			if (!memcmp(pchImage+i, achCom, 2))
				break;
			if ((len-i >= 36) && !memcmp(pchImage+i, "<CRTS>", 6))
			{
				found = true;
				if (!memcmp(pchImage+i+6, "0000-00-00 00-00-00-000", 23))
				{
					pch1 = pchImage + i;
				}
				break;
			}
			if ((len-i >= 17) && !memcmp(pchImage+i, "<CRA>", 5))
				pch2 = pchImage + i;
		}
		llClientRecv++;
		llCRTotal += llDiff;
		lCRAvg = (long)(llCRTotal / llClientRecv);
		sprintf(ach, "%06ld", lCRAvg);
		break;
	case ClientDisp:
		for(i=len-2;i>0;i--)
		{
			if (!memcmp(pchImage+i, achCom, 2))
				break;
			if ((len-i >= 36) && !memcmp(pchImage+i, "<CDTS>", 6))
			{
				found = true;
				if (!memcmp(pchImage+i+6, "0000-00-00 00-00-00-000", 23))
				{
					pch1 = pchImage + i;
				}
				break;
			}
			if ((len-i >= 17) && !memcmp(pchImage+i, "<CDA>", 5))
				pch2 = pchImage + i;
		}
		llClientDisp++;
		llCDTotal += llDiff;
		lCDAvg = (long)(llCDTotal / llClientDisp);
		sprintf(ach, "%06ld", lCDAvg);
		break;
	}

	if (!found)
	{
		bHasLD = false;
	}
	if (pch1 && pch2)
	{
		memcpy((void *)(pch1+6), strNow.c_str(), strNow.length());
		memcpy((void *)(pch2+5), ach, 6);
	}
}

void
LatencyData::updateH264(std::string &strTS, char *pchImage, int len, LatencyType lt, bool &bHasLD)
{
	int i;
	for(i=0;i<len;i++)
	{
		if (!memcmp(pchImage+i, "</XMLData>", 10))
		{
			update(strTS, pchImage, i+10, lt, bHasLD);
			break;
		}
	}
}

void
LatencyData::readData(std::string &strComment)
{
	std::string strC;
	std::vector<std::string> sv = split(strComment.c_str(), ";");
	if (sv.size() > 3)
	{
		strC = sv[3];
	}
	else
	{
		strC = strComment;
	}
	if (strC.length())
	{
		std::string strVal;
		xmlExtractNodeContents(strC.c_str(), "LRA", strVal, false);
		lLRAvg = atol(strVal.c_str());
		xmlExtractNodeContents(strC.c_str(), "LSA", strVal, false);
		lLSAvg = atol(strVal.c_str());
		xmlExtractNodeContents(strC.c_str(), "CRA", strVal, false);
		lCRAvg = atol(strVal.c_str());
		xmlExtractNodeContents(strC.c_str(), "CDA", strVal, false);
		lCDAvg = atol(strVal.c_str());
	}
}

#ifndef WIN32
int
iot_execlp(std::string &resource, StringVector &svArgs, StringVector &svVals)
{
	int iMax = 30;
	int i;

	char **ppchArgs = new char *[iMax];
	char **ppchVals = new char *[iMax];

	//FILE *fp = fopen("/tmp/debug.log", "w");
	//fprintf(fp, "exe is %s\n", resource.c_str());
	for(i=0;i<iMax;i++)
	{
		if (i < (int)svArgs.size())
		{
			std::string strArg = "-";
			if (*svArgs[i].c_str() == '-')
				strArg = "";
			strArg += svArgs[i];
			ppchArgs[i] = StringDup(strArg.c_str());
			if ((int)svVals.size() > i)
				ppchVals[i] = StringDup(svVals[i].c_str());
			else
				ppchVals[i] = 0;
			//fprintf(fp, "%02d %s : '%s'\n", i, ppchArgs[i], ppchVals[i]);
		}
		else
		{
			ppchArgs[i] = 0;
			ppchVals[i] = 0;
		}
	}
	//fclose(fp);

	int res = execlp(resource.c_str(), resource.c_str()
			,ppchArgs[0], ppchVals[0]
			,ppchArgs[1], ppchVals[1]
			,ppchArgs[2], ppchVals[2]
			,ppchArgs[3], ppchVals[3]
			,ppchArgs[4], ppchVals[4]
			,ppchArgs[5], ppchVals[5]
			,ppchArgs[6], ppchVals[6]
			,ppchArgs[7], ppchVals[7]
			,ppchArgs[8], ppchVals[8]
			,ppchArgs[9], ppchVals[9]
			,ppchArgs[10], ppchVals[10]
			,ppchArgs[11], ppchVals[11]
			,ppchArgs[12], ppchVals[12]
			,ppchArgs[13], ppchVals[13]
			,ppchArgs[14], ppchVals[14]
			,ppchArgs[15], ppchVals[15]
			,ppchArgs[16], ppchVals[16]
			,ppchArgs[17], ppchVals[17]
			,ppchArgs[18], ppchVals[18]
			,ppchArgs[19], ppchVals[19]
			,ppchArgs[20], ppchVals[20]
			,ppchArgs[21], ppchVals[21]
			,ppchArgs[22], ppchVals[22]
			,ppchArgs[23], ppchVals[23]
			,ppchArgs[24], ppchVals[24]
			,ppchArgs[25], ppchVals[25]
			,ppchArgs[26], ppchVals[26]
			,ppchArgs[27], ppchVals[27]
			,ppchArgs[28], ppchVals[28]
			,ppchArgs[29], ppchVals[29]
			,NULL);

	for(i=0;i<iMax;i++)
	{
		if (ppchArgs[i])
			delete [] ppchArgs[i];
		if (ppchVals[i])
			delete [] ppchVals[i];
	}

	delete [] ppchArgs;
	delete [] ppchVals;

	return res;
}
#endif
