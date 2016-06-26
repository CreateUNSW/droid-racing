#ifndef __STATUS_H
#define __STATUS_H

#include "thread.h"
#include "pipe.h"

typedef bool (*PSStopped)(void);
typedef bool (*PSHandleInput)(const char *, std::string&);

class	ProcStatus
{
private:
	CRITICAL_SECTION csPS;
	DWORD		 dwPS;
	ServerPipePair	*pspp_main;
	bool		bPipes;

	PSStopped	stop_func;
	PSHandleInput	inp_func;

public:
	time_t		tStart;

	ProcStatus(PSStopped sf = 0, PSHandleInput hi = 0);
	~ProcStatus();
	bool	handleMessage(const char *ach);
	void	setPipes(bool b) { bPipes = b; }
	bool	usePipes() const { return bPipes; }
	bool	ReadString(std::string &strData, std::string &strPid);
	bool	SendString(const std::string strData, std::string &strPid);

	void	InputLoop();
	bool	HandleInputOutput();
};

bool	handleInputDefault(const char *ach, std::string &strResponse);
bool	StoppedForSomeReasonDefault();
long	GetTickCount();

#endif //__STATUS_H
