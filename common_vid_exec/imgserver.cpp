//imgserver.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <string>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <fcntl.h>
#define recv(x,y,z,a) read(x,y,z)
#define send(x,y,z,a) write(x,y,z)
#define closesocket(s) close(s)
typedef int SOCKET;

#include "thread.h"
#include "buffer.h"

#include "ReadImage.hpp"
#include <iostream>
#include <wiringPi.h>
#include <chrono>
#include <thread>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#define MAXCLIENTS 10
#define IDLETIMEOUT 10

struct client_t
{
  int inuse;
  SOCKET csock;
  time_t activity;
  bool header;
  Buffer bOut;
};

bool bServerRunning = false;
int iPort = 0;

char STREAMEOL[3] = {'\r','\n',0};
#define WRITE write

void
TCPGetDate(char *achTemp)
{
	time_t t;
	time(&t);
	struct tm *tm = gmtime(&t);
	strftime(achTemp, 64, "%A, %d %b %Y %H:%M:%S", tm);
	strcat(achTemp, " GMT");
}

int
TCPSendStatus(Buffer &b, int iStatus)
{
	char ach[128];

	if (iStatus == 200)
		sprintf(ach, "%s\r\n", "HTTP/1.1 200 OK");
	if (iStatus == 404)
		sprintf(ach, "%s\r\n", "HTTP/1.1 404 Object Not Found");
	if (iStatus == 401)
		sprintf(ach, "%s\r\n", "HTTP/1.1 401 Authorization Required");
	if (iStatus == 301)
		sprintf(ach, "%s\r\n", "HTTP/1.1 301");

	b.Add(ach, strlen(ach));
	return b.Size();
}

int
TCPSendMJPEGHeaders(Buffer &b)
{
	char ach[256];
	char achDate[256];
	TCPGetDate(achDate);
	TCPSendStatus(b, 200);
	sprintf(ach, "Date: %s\r\n", achDate);
	b.Add(ach, strlen(ach));
	sprintf(ach,"Server: iot_imgserver\r\n");
	b.Add(ach, strlen(ach));
	sprintf(ach,"Cache-Control: max-age=0, must-revalidate\r\n");
	b.Add(ach, strlen(ach));
	sprintf(ach, "Transfer-Encoding: %s\r\n", "chunked");
	b.Add(ach, strlen(ach));
	sprintf(ach, "Content-Type: %s\r\n", "multipart/x-mixed-replace;boundary=boundarydonotcross" );
	b.Add(ach, strlen(ach));
	sprintf(ach, "\r\n");
	b.Add(ach, strlen(ach));
	return b.Size();
}


//2016-12-12 12-12-12-000
#define TS_LEN 23

int
SendStreamHeader(Buffer &b)
{
	return TCPSendMJPEGHeaders(b);
}

bool bFirstB = true;

bool
SendStreamFile(Buffer &b, Buffer &img, std::string &strTS)
{
	char		achHeader[1024];
	char		achCache[256];
	int		iLen;

	strcpy(achCache, "");

	iLen = img.Size();
	if (!iLen)
		return true;

	sprintf(achHeader,
			"Content-Type: image/%s%s"
			"Content-Length: %d%s"
			"%s"
			"%s",
			"jpeg", STREAMEOL, iLen, STREAMEOL, achCache, STREAMEOL);

	//chunked encoding length
	uint32_t cl = strlen(achHeader);
	cl += img.Size();
	cl += 22;
	sprintf(achCache, "%x\r\n", cl);
	b.Add(achCache, strlen(achCache));
	b.Add(achHeader, strlen(achHeader));
	b.Add(img.getData(), img.Size());
	sprintf(achHeader, "--boundarydonotcross%s%s", STREAMEOL, STREAMEOL);
	b.Add(achHeader, strlen(achHeader));
	return true;
}

int
img_server()
{
	Buffer buffer;
	bServerRunning = true;
	SOCKET lsock;
	char buf[10240];
	struct sockaddr_in laddr;
	int i;
	struct client_t clients[MAXCLIENTS];

	uint32_t dwNow,dwLast,dwDiff;

	dwNow = 0;
	dwLast = 0;
	dwDiff = 50;

	/* reset all of the client structures */
	for (i = 0; i < MAXCLIENTS; i++)
	{
		clients[i].inuse = 0;
		clients[i].csock = 0;
		clients[i].header = false;
	}

	/* determine the listener address and port */
	bzero(&laddr, sizeof(struct sockaddr_in));
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(iPort);
	laddr.sin_addr.s_addr = INADDR_ANY; // inet_addr(strLoc.c_str());
	if (!laddr.sin_port) {
		fprintf(stderr, "invalid listener port\n");
		return 20;
	}

	/* create the listener socket */
	if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return 20;
	}

	int enable = 1;
	if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	if (fcntl(lsock, F_SETFL, O_NONBLOCK))
		printf("failed to set non blocking");

	if (bind(lsock, (struct sockaddr *)&laddr, sizeof(laddr))) {
		perror("bind");
		return 20;
	}
	if (listen(lsock, 100)) {
		perror("listen");
		return 20;
	}

	std::string strTS;
	/* main polling loop. */
	while (bServerRunning)
	{
		fd_set fdsr,fdsw;
		int maxsock;
		struct timeval tv = {0,5000};
		time_t now = time(NULL);
		if (!bServerRunning)
			break;

		/* build the list of sockets to check. */
		FD_ZERO(&fdsw);
		FD_ZERO(&fdsr);
		FD_SET(lsock, &fdsr);
		maxsock = (int) lsock;
		for (i = 0; i < MAXCLIENTS; i++)
		{
			if (clients[i].inuse)
			{
				FD_SET(clients[i].csock, &fdsr);
				if ((int) clients[i].csock > maxsock)
					maxsock = (int) clients[i].csock;

				if (clients[i].bOut.Size())
				{
					FD_SET(clients[i].csock, &fdsw);
					if (clients[i].csock > maxsock)
						maxsock = clients[i].csock;
				}
			}
		}
		if (select(maxsock + 1, &fdsr, &fdsw, NULL, &tv) < 0) {
			printf("select error\n");
			return 30;
		}


		/* check if there are new connections to accept. */
		if (FD_ISSET(lsock, &fdsr))
		{
			SOCKET csock = accept(lsock, NULL, 0);
			if (fcntl(csock, F_SETFL, O_NONBLOCK))
				printf("failed to set non blocking");

			for (i = 0; i < MAXCLIENTS; i++)
			{
				if (clients[i].inuse == 0)
					break;
			}

			if (i < MAXCLIENTS)
			{
				printf("got a new connection socket %d\n", i);
				clients[i].csock = csock;
				clients[i].activity = now;
				clients[i].inuse = 1;

				int optval;
				socklen_t optlen = sizeof(optval);
				/* Check the status for the keepalive option */
				if(getsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
					perror("getsockopt()");
					close(csock);
					exit(EXIT_FAILURE);
				}
				//printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
				/* Set the option active */
				optval = 1;
				optlen = sizeof(optval);
				if(setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
					perror("setsockopt()");
					close(csock);
					exit(EXIT_FAILURE);
				}
				//printf("SO_KEEPALIVE set on socket\n");

			} else {
				fprintf(stderr, "too many clients\n");
				closesocket(csock);
			}
		}


		/* service any client connections that have waiting data. */
		for (i = 0; i < MAXCLIENTS; i++)
		{
			int nbyt, closeneeded = 0;

			if (clients[i].inuse == 0)
				continue;

			if (FD_ISSET(clients[i].csock, &fdsr))
			{
				if ((nbyt = recv(clients[i].csock, buf, sizeof(buf), 0)) <= 0)
				{
					closeneeded = 1;
				}
				else
				{
					//if (send(clients[i].osock, buf, nbyt, 0) <= 0)
					//	closeneeded = 1;
					clients[i].activity = now;
				}
			}
			if (FD_ISSET(clients[i].csock, &fdsw))
			{
				int iw = WRITE(clients[i].csock, clients[i].bOut.getData(), clients[i].bOut.Size());
				if (iw <= 0)
				{
					closeneeded = 1;
				}
				else
				{
					clients[i].activity = now;
					clients[i].bOut.consume(iw);
				}
			}

			if (now - clients[i].activity > IDLETIMEOUT)
			{
				closeneeded = 1;
			}
			if (closeneeded)
			{
				printf("closing socket %d\n", i);
				closesocket(clients[i].csock);
				clients[i].csock = 0;
				clients[i].inuse = 0;
				clients[i].activity = 0;
				clients[i].header = false;
				clients[i].bOut.reset();
			}
		}

		dwNow = millis();
		if ((dwNow - dwLast) >= dwDiff)
		{
			if (getImageBuffer(buffer))
			{
				//std::cout << "Got image!" << std::endl;
				dwLast = millis();
				for (i = 0; i < MAXCLIENTS; i++)
				{
					if (clients[i].inuse)
					{
						if (!clients[i].header)
						{
							SendStreamHeader(clients[i].bOut);
							clients[i].header = true;
						}
						SendStreamFile(clients[i].bOut, buffer, strTS);
					}
				}
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}
	return 0;
}

void *
server_thread(void *)
{
	img_server();
	printf("img_server exited\n");
	return 0;
}

static OC_Thread thServer = 0;

void
StartImageServer(int iPort_)
{
	iPort = iPort_;
	StartThread(thServer, (void *)server_thread, (void *)0);
}

void
StopImageServer()
{
	if (thServer)
	{
		bServerRunning = false;
		WaitForThread(thServer);
	}
}

void
StartTestImageServer(int iPort_)
{
	iPort = iPort_;
	StartThread(thServer, (void *)server_thread, (void *)0);
}

void
StopTestImageServer()
{
	if (thServer)
	{
		bServerRunning = false;
		WaitForThread(thServer);
	}
}
