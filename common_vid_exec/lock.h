#ifndef __OI_LOCK_H
#define __OI_LOCK_H
#ifdef WIN32
#include <Windows.h>
#endif
#include <string>

class Buffer;

class OldFileLock
{
private:
	std::string	strFile;
	std::string	strLockFile;
	std::string	strError;
	bool		b_locked;
	bool		b_debug;
	int			age;

	bool		obtainLock(int iMaxAge);
public:
	OldFileLock(const char *pchFile, int iMaxAge = -1);
	OldFileLock(std::string &strFile, int iMaxAge = -1);
	~OldFileLock();

	bool	locked() const { return b_locked; }
	const char *getError() const { return strError.c_str(); }
	void	persist();
	const int	getAge() const { return age; }
};

class FileLock
{
private:
	std::string	strFile;
	std::string	strError;
	bool		b_locked;
	bool		b_debug;
#ifdef WIN32
	HANDLE		hMutex;
#else
	int		fd;
#endif

	bool		obtainLock();
public:
	FileLock(const char *pchFile);
	FileLock(std::string &strFile);
	~FileLock();

	bool	locked() const { return b_locked; }
	const char *getError() const { return strError.c_str(); }
};


bool	FileTooOld(std::string &strFile, std::string &strPid, int &age, int iMaxAge = -1);
void	RemoveLock(OldFileLock *&pfl);

void	RemoveLock(FileLock *&pfl);
bool	LockAndWriteFile(const char *pchFile, Buffer &b);
bool	LockAndWriteFile(const char *pchFile, Buffer &b, std::string &strTS);
bool	LockAndReadFile(const char *pchFile, Buffer &b);

#endif
