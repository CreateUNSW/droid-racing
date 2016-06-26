#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <errno.h>

extern bool bTerminated;

void SignalHandler(int signal)
{
    switch(signal)
    {
    case SIGPIPE:
        //printf("SIGPIPE signal caught");
        break;
    case SIGHUP:
        break;
    case SIGTERM:
	bTerminated = true;
        break;
    case SIGUSR1:
        break;
    case SIGUSR2:
        break;
    case SIGSEGV:
        break;
    }
}


int InitSignal()
{
    struct sigaction act;
    sigset_t block_mask;
    sigemptyset (&block_mask);

    sigaddset (&block_mask, SIGINT);
    sigaddset (&block_mask, SIGQUIT);
    sigaddset (&block_mask, SIGIO);

    //new method of handling child deaths;
    //OI_Thread th = 0;
    //StartThread(th, (void *)child_handler, 0);

    // Signal for SIGHUP and SIGTERM
    act.sa_handler = SignalHandler;
    act.sa_mask = block_mask;
    act.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    return 0;
}

