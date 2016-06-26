#ifndef __WININC_H__
#define __WININC_H__

#include <unistd.h>
#include <pthread.h>

typedef long DWORD;
typedef int  HANDLE;
typedef void* LPVOID;
typedef bool BOOL;

//#define FALSE false
//#define TRUE true

#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _strcmpi strcasecmp
#define strcmpi strcasecmp
#define _stricmp strcasecmp
#define stricmp strcasecmp
#define MAX_PATH 256

#define _access access
#define _unlink unlink

#define DeleteFileA DeleteFile

bool	DeleteFile(const char *pchFile);
DWORD GetTickCount();
void Sleep(int iMill);
bool ReadFile(int fd, void* buf, int size, DWORD *num, void *dummy);
bool WriteFile(int fd, const char* data, int size, DWORD* num, void *dummy);
int GetCurrentProcessId(void);


typedef pthread_mutex_t CRITICAL_SECTION;
bool TryEnterCriticalSection(pthread_mutex_t *mutex);

#define EnterCriticalSection(x) pthread_mutex_lock(x)
#define LeaveCriticalSection(x) pthread_mutex_unlock(x)
#define DeleteCriticalSection(x) pthread_mutex_destroy(x)
BOOL	InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *pcs, DWORD dwSpinCount);

#endif // __WININC_H__
