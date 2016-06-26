#ifndef __OC_THREAD_H
#define __OC_THREAD_H

#ifdef WIN32
#include <windows.h>
typedef HANDLE OC_Thread;
#else
#include <pthread.h>
typedef pthread_t OC_Thread;
typedef void *(*pthread_func)(void *);
#endif
#ifdef WIN32
#else
#endif

inline void
WaitForThread(OC_Thread & thread)
{
	if (thread)
	{
#ifdef WIN32
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
#else
		pthread_join(thread, 0);
#endif
		thread = 0;
	}
}

#ifndef WIN32
int	pthread_join_timeout(pthread_t wid, int msecs);
#endif

inline bool
WaitForThread(OC_Thread & thread, int secs)
{
	bool retVal = false;
	if (thread)
	{
#ifdef WIN32
		DWORD dwVal = WaitForSingleObject(thread, secs * 1000);
		switch(dwVal)
		{
		case WAIT_ABANDONED:
			break;
		case WAIT_OBJECT_0:
			retVal = true;
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			break;
		}
		if (retVal)
			CloseHandle(thread);
		if (retVal)
			thread = 0;
#else
		retVal = !pthread_join_timeout(thread, secs * 1000);
		if (!retVal)
		{
			pthread_detach(thread);
			thread = 0;
		}
		else
		{
		}
		if (retVal)
			thread = 0;
#endif
	}
	return retVal;
}

inline void
KillThread(OC_Thread &th, int exitcode, bool bClose = true)
{
#ifdef WIN32
	TerminateThread(th, exitcode);
	if (bClose)
		CloseHandle(th);
#else
	pthread_cancel(th);
#endif
	th = 0;
}

inline bool
StartThread(OC_Thread & thread, void * func, void * thread_data)
{
#ifdef WIN32
	DWORD dwID;
	thread = CreateThread(0, 4096, (LPTHREAD_START_ROUTINE)func, thread_data, 0, &dwID);
#else
	pthread_create(&thread, NULL, (pthread_func)func, thread_data);
#endif
	return thread == 0 ? false : true;
}

inline void
CleanupThread(OC_Thread & thread)
{
	if (thread)
	{
#ifdef WIN32
		CloseHandle(thread);
#else
		pthread_detach(thread);
#endif
		thread = 0;
	}
}
#endif //__OC_THREAD_H
