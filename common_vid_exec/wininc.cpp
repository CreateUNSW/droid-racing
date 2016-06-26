#include <stdio.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syslog.h>
#include <stdint.h>
#include "wininc.h"
#include "status.h"

using namespace std;


bool
DeleteFile(const char *pchFile)
{
	return (unlink(pchFile) != -1);
}

void
Sleep(int iMill)
{
	if (iMill >= 1000)
	{
		//due to a problem with linux and GetTickCount around midnight
		//make sure we never sleep for more than 10 seconds
		int iSec = iMill / 1000;
		if (iSec > 10)
			iSec = 10;
		sleep(iSec);
	}
	else
		usleep(iMill * 1000);
}

int GetCurrentProcessId(void)
{
    return getpid();
}

/**
 * The linux implementation of WIN32 function ReadFile
 */
bool ReadFile(int fd, void* buffer, int size, DWORD *num, void *dummy)
{
    int ret = read(fd, buffer, size);
    *num = ret;
    return true;
}

/**
 * The linux implementation of WIN32 function WriteFile
 */
bool WriteFile(int fd,  const char* data, int size, DWORD* num, void *dummy)
{
    long ret = write(fd, data, size);
    *num = ret;
    return true;
}

static pthread_mutexattr_t mutexattr;

bool
InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *pcs, DWORD dwSpinCount)
{
	pthread_mutex_init(pcs, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
	return true;
}

bool TryEnterCriticalSection(pthread_mutex_t *mutex)
{
	int e = pthread_mutex_trylock(mutex);
	if (e == 0)
		return true;
	return false;
}

