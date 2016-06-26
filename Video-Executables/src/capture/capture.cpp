// capture.cpp
//

#include "buffer.h"
#include "utils.h"
#include "lock.h"
#include "args.h"
#include "thread.h"
#include "webcam.h"
#include "jpeglive.h"
#include "status.h"

#include <string.h>
#include <stdlib.h>
#include <tgmath.h>
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

Args args;

bool	bParentDied = false;
bool	bTerminated = false;
bool	bMonitorMode = true;
char	achConfig[1024];
char	achConfigRoot[1024];
extern	ProcStatus *pps;

uint64_t liveCount = 0;
uint64_t capCount = 0;
float liveRate = 0;
float capRate = 0;

static int fd = -1;
CRITICAL_SECTION csLive;
CRITICAL_SECTION csRec;
DWORD dwRec = 0x40;
bool bRunRec = false;
OC_Thread thRec = 0;
bool bRecord = false;
FILE *fpRec = 0;
std::string strRec = "/tmp/cap.mjpg";
std::list<Buffer *> lFrames;
DWORD dwStartRec = 0;
DWORD dwEndRec = 0;
static bool bStats = true;

extern void StartImageServer(int iPort_);
extern void StopImageServer();

void *
RecThread(void *)
{
	while (bRunRec)
	{
		EnterCriticalSection(&csRec);
		if (bRecord)
		{
			if (!fpRec)
			{
				capCount = 0;
				fpRec = fopen(strRec.c_str(), "wb");
				dwStartRec = GetTickCount();
			}
			if (fpRec)
			{
				int i = 0;
				while (lFrames.size())
				{
					Buffer *pb = lFrames.front();
					fwrite(pb->getData(), 1, pb->Size(), fpRec);
					delete pb;
					lFrames.pop_front();
					i++;
					capCount++;
					if (i > 3)
						break;
				}
			}
		}
		else
		{
			if (fpRec)
			{
				while (lFrames.size())
				{
					Buffer *pb = lFrames.front();
					fwrite(pb->getData(), 1, pb->Size(), fpRec);
					delete pb;
					lFrames.pop_front();	
					capCount++;
				}
				fclose(fpRec);
				fpRec = 0;
				dwEndRec = GetTickCount();
				if (bStats)
				{
					double fps = 0;
					if ((dwEndRec - dwStartRec) != 0)
					{
						fps = (double)capCount / (double)(((dwEndRec - dwStartRec)));
						if (fps != 0)
							fps *= 1000.0;
					}
					std::cout << "Capture FPS    " << fps << "\n";
				}
			}
		}
		LeaveCriticalSection(&csRec);
		Sleep(1);
	}
	return 0;
}

void StartRecThread()
{
	bRunRec = true;
	StartThread(thRec, (void *)RecThread, (void *)0);
}

void StopRecThread()
{
	if (thRec)
	{
		bRunRec = false;
		WaitForThread(thRec);
	}
}

void
DoCapWebcam()
{
	StartRecThread();
	StartImageServer(args.getIntOption("p"));
	fd = OpenDevice("/dev/video0");
	if (InitDevice(fd, args.getIntOption("iw"), args.getIntOption("ih"), args.getIntOption("r"), args.getOption("d"), false))
	{
		StartCapturing(fd);
		mainloop(fd);
		StopCapturing(fd);
		UninitDevice();
		CloseDevice(fd);
	}
	StopRecThread();
	StopImageServer();
}

bool
handleInput(const char *ach, std::string &strResponse)
{
	if (!strcasecmp(ach, "stop"))
	{
		bRecord = false;
		bTerminated = true;
		return false;
	}
	else if (!strncasecmp(ach, "record", 6))
	{
		bRecord = true;
	}
	else if (!strncasecmp(ach, "file", 4))
	{
		StringVector sv = split(ach, " ");
		if (sv.size() > 1)
			strRec = sv[1];
	}
	else if (!strcasecmp(ach, "stoprecord"))
	{
		bRecord = false;
	}
	else if (!strcasecmp(ach, "frames"))
	{
		char ach[256];
		sprintf(ach, "%lld", liveCount);
		strResponse = ach;
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
	args.setOption("d", "device", true, "/dev/video0");
	args.setOption("r", "rate", true, "-1");
	args.setOption("iw", "image_width", true, "1280");
	args.setOption("ih", "image_height", true, "720");
	args.setOption("p", "port", true, "9999");
	args.setOption("sr", "streamrate", true, "10");
	args.setOption("sw", "streamwidth", true, "-1");
	args.setOption("sh", "streamheight", true, "-1");
	args.setOption("hf", "horz_flip", true, "0");
	args.setOption("vf", "vert_flip", true, "0");
	args.setOption("br", "bitrate", true, "25000000");
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
	DoCapWebcam();
	DWORD dwTickEnd = GetTickCount();
	double fps = 0.0;
	if ((dwTickEnd - dwTickStart) != 0)
	{
		fps = (double)liveCount / (double)(((dwTickEnd - dwTickStart)));
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

