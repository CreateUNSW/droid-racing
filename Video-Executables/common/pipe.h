#ifndef __PIPE_H
#define __PIPE_H

#define	PIPE_BUFF_SIZE 10240*5
#define PIPE_TIMEOUT 10

#include "wininc.h"
typedef int C_Pipe;
#define PIPE_ERROR -1

#include <string>

class	PipePair
{
protected:
	C_Pipe	pipe_r;
	C_Pipe	pipe_s;
	std::string strPipeRecv;
	std::string strPipeSend;
	bool	bPersistentClient;
public:
	PipePair() :
		 pipe_r(PIPE_ERROR)
		,pipe_s(PIPE_ERROR)
		,bPersistentClient(false)
	{
	}

	~PipePair()
	{
	}
};


class	ServerPipePair : public PipePair
{
private:
public:
	ServerPipePair() :
		PipePair()
	{
	}

	~ServerPipePair()
	{
		DestroyPipes();
	}

	bool CreatePipes(const char *pchID);
	bool ReadString(std::string &strData, std::string &strPid);
	bool SendString(const std::string strData, std::string &strPid);
	void DestroyPipes();
	void SetPersistentClient(bool b) { bPersistentClient = b; }
};

class	ClientPipePair : public PipePair
{
private:
public:
	ClientPipePair() :
		PipePair()
	{
	}

	~ClientPipePair()
	{
		DestroyPipes();
	}

	bool CreateSendPipe(const char *pchID);
	bool CreatePipes(const char *pchID);
	bool ReadString(std::string &strData, int iSecsToWait = 5);
	bool SendString(const std::string strData);
	void DestroyPipes();
};

bool	SendProcMessage(int pid, const char *pchMSG, std::string &strResponse, bool bResponse = true, int iWait = 5);

void 	ReadLineFromPipe(int iPipe, std::string &strBuf);

#endif
