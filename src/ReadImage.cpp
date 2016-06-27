// cvtest.cpp
#include <mutex>

#include "ReadImage.hpp"

// Port for frame by frame controlled streaming
#define CV_PORT 8889

using namespace std;
using namespace cv;

/**BEGIN Lloyd's global and external variables**/
#define TS_LEN 23

Args args;

bool	bParentDied = false;
bool	bTerminated = false;
char	achConfig[1024];
char	achConfigRoot[1024];
extern	ProcStatus *pps;
uint64_t procCount = 0;
CRITICAL_SECTION csLive;
CRITICAL_SECTION csRec;
DWORD dwRec = 0x40;

static string strTestFile = "/dev/shm/test.jpg";

extern int InitSignal();

extern void StartTestImageServer(int iPort_);
extern void StopTestImageServer();

mutex buffLock;
bool imReady = false;

Buffer streamBuffer;

/**
	Constructor: initialise filenames, buffers, start the threading
*/
ReadStream::ReadStream(int argc, char **argv)
{
	outFile = strTestFile;
	b = new Buffer;
	start(argc, argv);
}

/**
	Destructor: delete buffers, thread, critical sections
*/
ReadStream::~ReadStream()
{
	cout << "destruct" << endl;
	//DeleteCriticalSection(&csRec);
	DeleteCriticalSection(&csLive);

	StopTestImageServer();
	delete b;

	if (pps)
		delete pps;
}

/**
	Writes out modified Mat image to TCP stream handler.
	Warning: very slow, uncomment if you wish to use
*/
void ReadStream::writeStream(Mat img)
{
	// not yet implemented with multithreading
	//cout << "Writing image: " << img.size() << endl;
	vector<int> p;
	p.push_back(CV_IMWRITE_JPEG_QUALITY);
	p.push_back(100);
	vector<uchar> vout;
	imencode(".jpg", img, vout, p);
	buffLock.lock();
	//b->reset();
	//b->Add((char *)vout.data(), vout.size());
	streamBuffer.reset();
	streamBuffer.Add((char *)vout.data(), vout.size());
	imReady = true;
	buffLock.unlock();
}

bool getImageBuffer(Buffer & ret)
{
	buffLock.lock();
	if(!imReady){
		buffLock.unlock();
		return false;
	}
	ret.reset();
	ret.Add(streamBuffer.getData(), streamBuffer.Size());
	imReady = false;
	buffLock.unlock();
	return true;
}

/**
	Lloyd's function for termination
*/
bool handleInput(const char *ach, string &strResponse)
{
	if (!strcasecmp(ach, "stop"))
	{
		bTerminated = true;
		return false;
	}
	else
	{
	}
	return true;
}

/**
	Another of Lloyd's functions for termination
*/
void sigterm(int signal, siginfo_t *siginfo, void *)
{
	bParentDied = true;
}

/**
	Mostly Lloyd's code.
	Start main image retrieving process
*/
void ReadStream::start(int argc, char **argv)
{
	args.setOption("r", "rate", true, "-1");
	args.setOption("iw", "image_width", true, "320");
	args.setOption("ih", "image_height", true, "240");
	args.setOption("p", "port", true, "8889");
	args.setOption("sr", "streamrate", true, "25");
	args.setOption("sw", "streamwidth", true, "-1");
	args.setOption("sh", "streamheight", true, "-1");
	args.setOption("i", "pidparent", true);

	if (!args.parse(argc, argv))
	{
		args.usage();
		exit(1);
	}

	InitSignal();
	int sig;
	prctl(PR_GET_PDEATHSIG, &sig);
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = sigterm;
	act.sa_flags = SA_SIGINFO;
	sigaction(sig, &act, 0);

	const char *pchID = args.getOption("i");
	if (pchID && *pchID)
		pps->setPipes(true);

	InitializeCriticalSectionAndSpinCount(&csLive, dwRec);
	InitializeCriticalSectionAndSpinCount(&csRec, dwRec);

	pps = new ProcStatus(StoppedForSomeReasonDefault, handleInput);

	// Start image server on port
	StartTestImageServer(CV_PORT);
}
