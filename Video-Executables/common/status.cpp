// status.cpp
#include "wininc.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include "status.h"

ProcStatus *pps = 0;

ProcStatus::ProcStatus(PSStopped sf, PSHandleInput hi)
	:dwPS(0)
	,pspp_main(0)
	,bPipes(false)
	,stop_func(sf)
	,inp_func(hi)
{
	InitializeCriticalSectionAndSpinCount(&csPS, dwPS);
	time(&tStart);
	if (sf && hi)
	{
		char achID[256];
		sprintf(achID, "iot_proc_%08d", getpid());
		pspp_main = new ServerPipePair();
		if (pspp_main)
			pspp_main->CreatePipes(achID);
	}
}

ProcStatus::~ProcStatus()
{
	if (pspp_main)
	{
		pspp_main->DestroyPipes();
		delete pspp_main;
	}
	DeleteCriticalSection(&csPS);
}

void
ProcStatus::InputLoop()
{
	std::string strResponse;
	if (usePipes())
	{
		std::string strCmd;
		std::string strPid;
		while (!(*stop_func)())
		{
			strCmd = "";
			strResponse= "";
			ReadString(strCmd, strPid);
			if (!strCmd.empty())
			{
				char *pch = new char[strCmd.length()+1];
				strcpy(pch, strCmd.c_str());
				while (pch[strlen(pch)-1] == '\r' || pch[strlen(pch)-1] == '\n')
					pch[strlen(pch)-1] = 0;
				bool bBreak = !(*inp_func)(pch, strResponse);
				delete [] pch;
				if (strResponse.length())
					SendString(strResponse, strPid);

				if (bBreak)
					break;
			}
			else
			{
				Sleep(100);
			}
		}
	}
	else
	{
		char ach[512];
		while (fgets(ach, sizeof(ach)-1, stdin) != 0)
		{
			if (*ach == 0)
				continue;

			while (ach[strlen(ach)-1] == '\n' || ach[strlen(ach)-1] == '\r')
				ach[strlen(ach)-1] = 0;

			strResponse = "";
			bool bBreak = !(*inp_func)(ach, strResponse);
			if (strResponse.length())
			{
				fprintf(stdout, "%s\n", strResponse.c_str());
				fflush(stdout);
			}
			if (bBreak)
				break;

		}
	}
}

void *
InputLoop(void *p)
{
	ProcStatus *pps = (ProcStatus *)p;
	pps->InputLoop();
	return 0;
}

bool
ProcStatus::HandleInputOutput(void)
{
	OC_Thread th = 0;
	StartThread(th, (void *)::InputLoop, (void *)this);
	CleanupThread(th);
	return true;
}

extern bool bTerminated;
extern bool bParentDied;

bool
StoppedForSomeReasonDefault()
{
	if (bTerminated || bParentDied)
		return true;
	return false;
}

bool
handleInputDefault(const char *ach, std::string &strResponse)
{
	if (!strncmp(ach, "hello", 5))
	{
		strResponse = "hello";
	}
	else if (!strncmp(ach, "stop", 4))
	{
		bTerminated = true;
		return false;
	}
	else if (!strncmp(ach, "status", 6))
	{
		//getStatusReport(strResponse);
	}
	else
	{
		//if (pps && pps->handleMessage(ach))
		//{
		//}
	}
	return true;
}

bool
ProcStatus::ReadString(std::string &strData, std::string &strPid)
{
	if (pspp_main)
		return pspp_main->ReadString(strData, strPid);
	return false;
}

bool
ProcStatus::SendString(const std::string strData, std::string &strPid)
{
	if (pspp_main)
		return pspp_main->SendString(strData, strPid);
	return false;
}

#ifndef WIN32
extern ProcStatus *pps;

long
GetTickCount()
{
	uint64_t lGetTickCount;
	timeval ts;
	gettimeofday(&ts, 0);

	uint64_t lSecs = (uint64_t)(ts.tv_sec);
	lSecs -= (uint64_t)(pps ? pps->tStart : 1260000000);

	lGetTickCount = (uint64_t)(lSecs * 1000 + (ts.tv_usec / 1000));
	long l = lGetTickCount;
	return l;
}
#endif
