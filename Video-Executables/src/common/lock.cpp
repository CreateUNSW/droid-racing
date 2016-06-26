#include <stdio.h>
#include "lock.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "buffer.h"

bool
FileTooOld(std::string &strFile, std::string &strPid, int &age, int iMaxAge)
{
	struct stat buf;
	time_t tnow;
	time(&tnow);
	char ach[16];
	int iAge = 5;

	if (iMaxAge != -1)
		iAge = iMaxAge;

	if (!stat(strFile.c_str(), &buf))
	{
		int secsDiff = tnow - buf.st_ctim.tv_sec;

		age = secsDiff * 1000;

		if (secsDiff > iAge)
		{
			int hFile = open(strFile.c_str(), O_RDONLY);
			if (hFile > 0)
			{
				//ReadFixedString(hFile, ach, 8);
				ach[8] = 0;
				strPid = ach;
				close(hFile);
			}
			return true;
		}
		else
			return false;

	}
	return true;
}


#define MAX_TRIES 3001

bool
OldFileLock::obtainLock(int iMaxAge)
{
	int iTries = 0;
	if (!strFile.empty())
	{
		strLockFile = strFile;
		strLockFile += ".lf";

		char ach[32];

		sprintf(ach, "%08ld", (long)getpid());

		while (!b_locked && iTries < MAX_TRIES)
		{
			if (access(strLockFile.c_str(), 0))
			{
				int h = open(strLockFile.c_str(), O_RDWR | O_CREAT);
				if (h > 0)
				{
					//WriteFixedString(h, ach, strlen(ach));
					time_t tNow;
					time(&tNow);
					//WriteInt32(h, (int32)tNow);
					close(h);
					b_locked = true;
				}
			}

			if (!b_locked && iTries && ((iTries % 10) == 0))
			{
				//if (b_debug)
				//	DebugLog(DEBUG_LOCKING, "Lock File %s still not created after %d attempts", strLockFile.c_str(), iTries);
			}

			if (iMaxAge != -1)
			{
				if (!b_locked && (((iTries == 1) || ((iTries % 1000) == 0) || iMaxAge != -1)))
				{
					std::string strPid;
					if (FileTooOld(strLockFile, strPid, age, iMaxAge))
					{
						//if (b_debug)
						//	DebugLog(DEBUG_LOCKING, "Lock File %s deleted too old and was created by %s", strLockFile.c_str(), strPid.c_str());
						unlink(strLockFile.c_str());
					}
					else
					{
						//if they have specified a max age then dont loop until we can lock the file
						return b_locked;
					}
				}
			}
			else
			{
				if (!b_locked && iTries && ((iTries % 1000) == 0))
				{
					std::string strPid;
					if (FileTooOld(strLockFile, strPid, age, iMaxAge))
					{
						//if (b_debug)
						//	DebugLog(DEBUG_LOCKING, "Lock File %s deleted too old and was created by %s", strLockFile.c_str(), strPid.c_str());
						unlink(strLockFile.c_str());
					}
				}
			}

			if (!b_locked)
			{
				usleep(100);
				iTries++;
			}
		}
	}
	if (!b_locked)
	{
		//if (b_debug)
		//	DebugLog(DEBUG_LOCKING, "Lock File %s could not be created", strLockFile.c_str());
		strError = "Could not obtain lock on ";
		strError += strFile;
	}
	else
	{
		if (iTries > 1)
		{
			//if (b_debug)
			//	DebugLog(DEBUG_LOCKING, "Lock File %s was created after %d attempts", strLockFile.c_str(), iTries);
		}
	}
	return b_locked;
}

OldFileLock::OldFileLock(const char *pchFile, int iMaxAge) :
	strFile(pchFile ? pchFile : "")
	,b_locked(false)
	,b_debug(false)
	,age(0)
{
	obtainLock(iMaxAge);
}

OldFileLock::OldFileLock(std::string &strFile_, int iMaxAge) :
	strFile(strFile_)
	,b_locked(false)
	,b_debug(false)
	,age(0)
{
	obtainLock(iMaxAge);
}

OldFileLock::~OldFileLock()
{
	if (!strLockFile.empty() && b_locked)
	{
		//if (b_debug)
		//	DebugLog(DEBUG_LOCKING, "Lock File %s released", strLockFile.c_str());
		unlink(strLockFile.c_str());
		b_locked = false;
	}
}

void
RemoveLock(OldFileLock *&pfl)
{
	if (pfl)
		delete pfl;
	pfl = 0;
}

//this will stop deletion of the lock file when destroyed so that we can use the lock file as a timer
void
OldFileLock::persist()
{
	//if (b_debug)
	//	DebugLog(DEBUG_LOCKING, "Lock File %s made persistent", strLockFile.c_str());
	strLockFile = "";
}

////////////////////////////////////////////////////////////////////////////////
//new FileLock

bool
FileLock::obtainLock()
{
	struct flock fl = {F_WRLCK, SEEK_SET,   0,      0,     0 };
	fl.l_pid = getpid();
	if (access(strFile.c_str(), 0))
	{
		//does not exist yet
		b_locked = true;
		return b_locked;
	}
	bool done = false;
	int iCount = 0;
	while (!done && iCount < 1000)
	{
		if ((fd = open(strFile.c_str(), O_RDWR)) != -1)
		{
			if (fcntl(fd, F_SETLKW, &fl) != -1)
			{
				b_locked = true;
				return b_locked;
			}
		}
		usleep(1000);
		iCount++;
	}
	return b_locked;
}

FileLock::FileLock(const char *pchFile) :
	strFile(pchFile ? pchFile : "")
	,b_locked(false)
	,b_debug(false)
	,fd(0)
{
	obtainLock();
}

FileLock::FileLock(std::string &strFile_) :
	strFile(strFile_)
	,b_locked(false)
	,b_debug(false)
	,fd(0)
{
	obtainLock();
}

FileLock::~FileLock()
{
	if (fd)
	{
		struct flock fl = {F_UNLCK, SEEK_SET,   0,      0,     0 };
		fl.l_pid = getpid();
		if (fcntl(fd, F_SETLKW, &fl) == -1)
		{
		}
		close(fd);
		fd = 0;
	}
}

void
RemoveLock(FileLock *&pfl)
{
	if (pfl)
		delete pfl;
	pfl = 0;
}

bool
LockAndWriteFile(const char *pchFile, Buffer &b)
{
	FileLock f(pchFile);
	if (f.locked())
	{
		return b.writeFile(pchFile);
	}
	return false;
}

bool
LockAndWriteFile(const char *pchFile, Buffer &b, std::string &strTS)
{
	FileLock f(pchFile);
	if (f.locked())
	{
		return b.writeFile(pchFile, strTS);
	}
	return false;
}

bool
LockAndReadFile(const char *pchFile, Buffer &b)
{
	FileLock f(pchFile);
	if (f.locked())
	{
		return b.readFile(pchFile);
	}
	return false;
}
