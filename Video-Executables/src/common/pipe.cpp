#include "pipe.h"
#include "utils.h"
#include "buffer.h"
#include "thread.h"

#include <sys/syslog.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/signal.h>
#include <time.h>
#include <string>
#include <fcntl.h>

#define PIPE_SLEEP_TIME 1

bool
CreatePipePair(const char *pchID, C_Pipe &hPipeRecv, C_Pipe &hPipeSend, std::string &strRecv, std::string &strSend)
{
	strRecv  = "/tmp/";
	strRecv += pchID;
	strRecv += "-ServRecv";

//	if ((stat (strRecv.c_str(), &sts)) == -1)
	{
		int fd, ret_val;
		ret_val = mkfifo(strRecv.c_str(), 0666);
		if (( ret_val == -1) && (errno != EEXIST))
		{
			return false;
		}
		fd = open(strRecv.c_str(), O_RDWR | O_NONBLOCK);
		hPipeRecv = fd;
	}

	return true;
}

StringVector *psvQueue = 0;

bool
ServerPipePair::ReadString(std::string &strData, std::string &strPid)
{
	strData = "";

	if (psvQueue && psvQueue->size())
	{
		char ach[32];
		memset(ach, 0, sizeof(ach));
		strncpy(ach, psvQueue->front().c_str(), 10);
		if (strlen(ach) > 8)
		{
			strData = psvQueue->front().substr(8, psvQueue->front().length());
			ach[8] = 0;
			strPid = ach;
		}
		else
		{
			//MessageBeep(0);
			//syslog(LOG_ERR, "bad pipe data received '%s'", ach);
		}
		psvQueue->erase(psvQueue->begin());
		return true;
	}

	if (pipe_r == PIPE_ERROR)
		return false;

	char	chRequest[PIPE_BUFF_SIZE];
	DWORD	cbBytesRead = 0;
	int iTries = 0;
	int iTotal = 0;
	bool found = false;
	while (iTries < 25)
	{
		cbBytesRead = read(pipe_r, chRequest+iTotal, sizeof(chRequest)-iTotal);

		if (cbBytesRead <= 0 && iTries == 0)
			return false;

		if (iTotal == 0 && iTries)
			return false;

		if (cbBytesRead < 0)
		{
			if (errno != EAGAIN)
			{
				return false;
			}
		}
		else
		{
			if (cbBytesRead > 0)
			{
				iTotal += cbBytesRead;
				if (chRequest[iTotal-1] == '\n')
				{
					cbBytesRead = iTotal;
					found = true;
					break;
				}
			}
		}
		iTries++;
		Sleep(PIPE_SLEEP_TIME);
	}
	if (!found)
	{
	}
	if (cbBytesRead < 0)
		cbBytesRead = 0;
	chRequest[cbBytesRead] = '\0';

	if (cbBytesRead)
	{
		while (chRequest[strlen(chRequest)-1] == '\n' || chRequest[strlen(chRequest)-1] == '\r')
			chRequest[strlen(chRequest)-1] = 0;


		StringVector sv = split(chRequest, "\n");
		if (sv.size())
		{
			if (sv.size() > 1)
			{
				if (!psvQueue)
					psvQueue = new StringVector();
				int i;
				for(i=1;i<(int)sv.size();i++)
				{
					psvQueue->push_back(sv[i]);
				}
			}
			char ach[256];
			strncpy(ach, sv[0].c_str(), 10);
			strData = sv[0].substr(8, sv[0].length());
			ach[8] = 0;
			strPid = ach;
		}
	}

	return true;
}


class SendData
{
public:
	std::string strPid;
	std::string strSend;

	SendData()
	{
	}
	~SendData()
	{
	}
};

void *
SendStreamData(void *pvData)
{
	SendData *psd = (SendData *)pvData;
	if (!psd)
		return 0;

	std::string strPipeSend;

	strPipeSend = "/tmp/";
	strPipeSend += psd->strPid;
	strPipeSend += "-Send";

	//int pipe_s = open(strPipeSend.c_str(), O_WRONLY);
	int pipe_s = open(strPipeSend.c_str(), O_RDWR);
	if (pipe_s == PIPE_ERROR)
	{
		return 0;
	}
	int iTries = 0;
	int iTotal = 0;
	std::string strS = psd->strSend;
	strS += "\r\n";
	while (iTries < 100)
	{
		int iWrote = write(pipe_s,
				strS.c_str() + iTotal,
				strS.length() - iTotal
				);
		iTotal += iWrote;
		if (iTotal == (int)strS.length())
		{
			Sleep(PIPE_SLEEP_TIME * 10); //give client a chance to read
			break;
		}
		else
		{
		}
		iTries++;
	}

	if (iTries >= 100)
	{
	}

	close(pipe_s);
	delete psd;
	return 0;
}

bool
ServerPipePair::SendString(const std::string strSend, std::string &strPid)
{
	strPipeSend = "/tmp/";
	strPipeSend += strPid;
	strPipeSend += "-Send";

	if (pipe_s != PIPE_ERROR)
		close(pipe_s);

	pipe_s = open(strPipeSend.c_str(), O_RDWR);
	if (pipe_s == PIPE_ERROR)
	{
		return false;
	}
	int iTries = 0;
	int iTotal = 0;
	std::string strS = strSend;
	strS += "\r\n";
	while (iTries < 10)
	{
		int iWrote = write(pipe_s,
				strS.c_str() + iTotal,
				strS.length() - iTotal
				);
		iTotal += iWrote;
		if (iTotal == (int)strS.length())
		{
			Sleep(PIPE_SLEEP_TIME * 2); //give client a chance to read
			close(pipe_s);
			pipe_s = PIPE_ERROR;
			return true;
		}
		else
		{
		}
		iTries++;
	}
	Sleep(PIPE_SLEEP_TIME * 2); //give client a chance to read
	close(pipe_s);
	pipe_s = PIPE_ERROR;
	return false;
}

void
ServerPipePair::DestroyPipes()
{
	if (pipe_r != PIPE_ERROR)
	{
		close(pipe_r);
		unlink(strPipeRecv.c_str());
		pipe_r = PIPE_ERROR;
	}

	if (pipe_s != PIPE_ERROR)
	{
		close(pipe_s);
		unlink(strPipeSend.c_str());
		pipe_s = PIPE_ERROR;
	}
}

bool
ServerPipePair::CreatePipes(const char *pchID)
{
	CreatePipePair(pchID, pipe_r, pipe_s, strPipeRecv, strPipeSend);

	return (pipe_r != PIPE_ERROR);
}

bool
OpenPipePair(const char *pchID, C_Pipe &hPipeSend, C_Pipe &hPipeRecv, std::string &strSend, std::string &strRecv)
{
	char ach[256];
	sprintf(ach, "%08d", (int)GetCurrentProcessId());

	strRecv  = "/tmp/";
	strRecv += ach;
	strRecv += "-Send";

	strSend  = "/tmp/";
	strSend += pchID;
	strSend += "-ServRecv";

	//hPipeSend = open(strSend.c_str(), O_WRONLY | O_NONBLOCK);
	if (_access(strSend.c_str(), 0))
	{
		//this needs to exist already
		return false;
	}
	hPipeSend = open(strSend.c_str(), O_RDWR | O_NONBLOCK);
	if (hPipeSend == PIPE_ERROR)
	{
		return false;
	}

//	if ((stat (strRecv.c_str(), &sts)) == -1)
	{
		int fd, ret_val;
		ret_val = mkfifo(strRecv.c_str(), 0666);
		if (( ret_val == -1) && (errno != EEXIST))
		{
			close(hPipeSend);
			return false;
		}
		fd = open(strRecv.c_str(), O_RDWR | O_NONBLOCK);
		hPipeRecv = fd;
	}

	return true;
}

bool
OpenSendPipe(const char *pchID, C_Pipe &hPipeSend, std::string &strSend)
{
	hPipeSend = -1;

	strSend  = "/tmp/";
	strSend += pchID;
	strSend += "-ServRecv";

	//hPipeSend = open(strSend.c_str(), O_WRONLY | O_NONBLOCK);
	hPipeSend = open(strSend.c_str(), O_RDWR | O_NONBLOCK);
	if (hPipeSend == PIPE_ERROR)
	{
		return false;
	}
	return true;
}

void
ClientPipePair::DestroyPipes()
{
	if (pipe_r != PIPE_ERROR)
	{
		close(pipe_r);
		unlink(strPipeRecv.c_str());
		pipe_r = PIPE_ERROR;
	}

	if (pipe_s != PIPE_ERROR)
	{
		close(pipe_s);
		pipe_s = PIPE_ERROR;
	}
}

bool
ClientPipePair::ReadString(std::string &strData, int iSecsToWait)
{
	char achBuff[PIPE_BUFF_SIZE];

	Buffer	b;
	int	iTries	= 0;
	DWORD	iRead	= 0;

	Sleep(1);
	while (iTries < 5000)
	{
		memset(achBuff, 0, sizeof(achBuff));
		iRead = read(pipe_r, achBuff, sizeof(achBuff));
		if (iRead < 0 && errno != EAGAIN)
		{
			return false;
		}
		else
		{
			if (iRead > 0)
			{
				b.Add(achBuff, iRead);
				int iPos = b.locate("\r\n");
				if (iPos != -1)
				{
					char *pch = b.getData();
					pch[iPos] = 0;
					strData = b.getData();
					return true;
				}
			}
		}
		iTries++;
		Sleep(1);
	}

	return false;
}

bool
ClientPipePair::SendString(const std::string strCmd)
{
	if (pipe_s == PIPE_ERROR)
		return false;


	char ach[256];
	sprintf(ach, "%08d", (int)GetCurrentProcessId());

	std::string strNew  = ach;
	strNew += strCmd;
	strNew += "\r\n";


	int iTries = 0;
	while (iTries < 30 )
	{
		int iWrote;
		int iTotal = 0;

		iWrote = write(pipe_s, (LPVOID)(strNew.c_str() + iTotal), strNew.length() - iTotal);
		if (iWrote > 0)
			iTotal += iWrote;
		if (iTotal < (int)strNew.length())
		{
			if (errno != EAGAIN)
			{
				return false;
			}
		}
		else
			break;
		iTries++;
		Sleep(PIPE_SLEEP_TIME);
	}
	if (iTries >= 30)
	{
		return false;
	}
	return true;
}

bool
ClientPipePair::CreatePipes(const char *pchID)
{
	OpenPipePair(pchID, pipe_s, pipe_r, strPipeSend, strPipeRecv);

	return (pipe_r != PIPE_ERROR && pipe_s != PIPE_ERROR);
}

bool
ClientPipePair::CreateSendPipe(const char *pchID)
{
	OpenSendPipe(pchID, pipe_s, strPipeSend);

	return (pipe_s != PIPE_ERROR);
}

void
ReadLineFromPipe(int iPipe, std::string &strBuf)
{
	DWORD	num_io;
	Buffer b;
	char buf[1];
	int iTries = 0;

	while (true)
	{
		if ( !ReadFile(iPipe, buf, 1, &num_io, NULL) || num_io <= 0 )
		{
			if (iTries >= 50)
			{
				break;
			}
		}

		if (num_io > 0)
		{
			b.Add(buf, 1);
			if (buf[0] == '\n')
			{
				char c = b.lastChar();
				while (c == '\n' || c == '\r')
				{
					b.truncate(b.Size()-1);
					c = b.lastChar();
				}
				b.nullTerminate();
				strBuf = b.getData();
				break;
			}
		}
		else
		{
			Sleep(1);
		}
		iTries++;
	}
}

bool
SendProcMessage(int pid, const char *pchMSG, std::string &strResponse, bool bResponse, int iWait)
{
	//set bResponse true if expectting response to message
	//set iWait to time you are willing to wait for response
	ClientPipePair cpp;
	char achID[256];
	sprintf(achID, "usv_proc_%08d", pid);
	if (cpp.CreatePipes(achID))
	{
		std::string strMsg = pchMSG ? pchMSG : "";
		if (cpp.SendString(strMsg))
		{
			if (bResponse)
			{
				if (cpp.ReadString(strResponse, iWait))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return true;
			}
		}
	}
	return false;
}

