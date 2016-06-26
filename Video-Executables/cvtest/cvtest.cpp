// cvtest.cpp
//

#include "buffer.h"
#include "utils.h"
#include "lock.h"
#include "args.h"
#include "thread.h"
#include "status.h"

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string>
#include <list>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "buffer.h"
#include "lock.h"
#include "jpeg.h"

Args args;

bool	bParentDied = false;
bool	bTerminated = false;
char	achConfig[1024];
char	achConfigRoot[1024];
extern	ProcStatus *pps;
uint64_t procCount = 0;
CRITICAL_SECTION csLive;
CRITICAL_SECTION csRec;
DWORD dwRec = 0x40;

static std::string strLiveFile = "/dev/shm/live.jpg";
static std::string strTestFile = "/dev/shm/test.jpg";

extern void StartTestImageServer(int iPort_);
extern void StopTestImageServer();

extern void StartImageServer(int iPort_);
extern void StopImageServer();
#define TS_LEN 23

bool
GetLiveImage(std::string &strFile, std::string &strLast, Buffer &buffer)
{
	buffer.reset();
	
	EnterCriticalSection(&csLive);
	LockAndReadFile(strFile.c_str(), buffer);
	LeaveCriticalSection(&csLive);
	if (buffer.Size())
	{
		char achTS[30];
		memcpy(achTS, buffer.getData(), TS_LEN);
		achTS[TS_LEN] = 0;
		if (strcmp(achTS, strLast.c_str()))
		{
			buffer.consume(TS_LEN);
			if ((args.getIntOption("iw") != -1) || (args.getIntOption("ih") != -1))
			{
				CVSimpleResizeJpeg(buffer, args.getIntOption("iw"), args.getIntOption("ih"), 0); 
			}
			strLast = achTS;
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}


void
DoTest()
{
	Buffer b;
	std::string strTS;
	DWORD dwNow, dwLast, dwDiff;

	dwNow = 0;
	dwLast = 0;
	dwDiff = 0;
	if (args.getIntOption("r") != -1)
		dwDiff = (DWORD)(1000.0/(float)atof(args.getOption("r")));

	StartTestImageServer(args.getIntOption("p"));
	while (!StoppedForSomeReasonDefault())
	{
		dwNow = GetTickCount();
		if ((dwNow - dwLast) >= dwDiff)
		{
			if (GetLiveImage(strLiveFile, strTS, b))
			{
				dwLast = GetTickCount();

				cv::Mat img = cv::imdecode(cv::Mat(1, b.Size(), CV_8UC3, b.getData()), 1);

				/*const static cv::Scalar cw =  CV_RGB(255,255,255);
				cv::Point p1, p2;
				p1.x = img.cols/2;
				p2.x = p1.x;
				p1.y = (img.rows / 2) - 20;
				p2.y = p1.y - 40;
				cv::line(img, p1, p2, cw, 2);

				p1.y = (img.rows / 2) + 20;
				p2.y = p1.y + 40;
				cv::line(img, p1, p2, cw, 2);

				p1.y = img.rows / 2;
				p2.y = p1.y;

				p1.x = (img.cols/ 2) - 20;
				p2.x = p1.x - 40;
				cv::line(img, p1, p2, cw, 2);

				p1.x = (img.cols/ 2) + 20;
				p2.x = p1.x + 40;
				cv::line(img, p1, p2, cw, 2);

				std::vector<int> p;
				p.push_back(CV_IMWRITE_JPEG_QUALITY);
				p.push_back(100);
				std::vector<uchar> vout;
				cv::imencode(".jpg", img, vout, p);

				b.reset();
				b.Add((char *)vout.data(), vout.size());
				*/

				EnterCriticalSection(&csLive);
				LockAndWriteFile(strTestFile.c_str(), b, strTS);
				LeaveCriticalSection(&csLive);
			}
		}
		else
		{
			Sleep(1);
		}
	}
	StopTestImageServer();
}

bool
handleInput(const char *ach, std::string &strResponse)
{
	if (!strcasecmp(ach, "stop"))
	{
		bTerminated = true;
		return false;
	}
	else
	{
	}
	return true;
}

void
StartStdinWait()
{
	if (pps)
		pps->HandleInputOutput();
}

void sigterm(int signal, siginfo_t *siginfo, void *)
{
	bParentDied = true;
}

extern int InitSignal();

int main(int argc, char** argv)
{
	args.setOption("r", "rate", true, "-1");
	args.setOption("iw", "image_width", true, "1280");
	args.setOption("ih", "image_height", true, "720");
	args.setOption("p", "port", true, "9998");
	args.setOption("sr", "streamrate", true, "10");
	args.setOption("sw", "streamwidth", true, "-1");
	args.setOption("sh", "streamheight", true, "-1");
	args.setOption("i", "pidparent", true);

	if (!args.parse(argc, argv))
	{
		args.usage();
		return 1;
	}

	InitSignal();
	int sig;
	prctl(PR_GET_PDEATHSIG, &sig);
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = sigterm;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, 0);

	const char *pchID = args.getOption("i");
	if (pchID && *pchID)
		pps->setPipes(true);

	InitializeCriticalSectionAndSpinCount(&csLive, dwRec);
	InitializeCriticalSectionAndSpinCount(&csRec, dwRec);

	pps = new ProcStatus(StoppedForSomeReasonDefault, handleInput);
	StartStdinWait();
	DWORD dwTickStart = GetTickCount();
	DoTest();
	DWORD dwTickEnd = GetTickCount();
	double fps = 0.0;
	if ((dwTickEnd - dwTickStart) != 0)
	{
		fps = (double)procCount / (double)(((dwTickEnd - dwTickStart)));
		if (fps != 0)
			fps *= 1000.0;
	}
	std::cout << "Effective FPS    " << fps << "\n";

	DeleteCriticalSection(&csRec);
	DeleteCriticalSection(&csLive);
	if (pps)
		delete pps;
	return 0;
}
