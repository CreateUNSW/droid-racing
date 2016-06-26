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

static string strLiveFile = "/dev/shm/live.jpg";
static string strTestFile = "/dev/shm/test.jpg";

extern int InitSignal();

extern void StartTestImageServer(int iPort_);
extern void StopTestImageServer();

extern void StartImageServer(int iPort_);
extern void StopImageServer();
/**END Lloyd's global and external variables**/

// Multithreading locks for image reading, writing and streaming
mutex buffLock;
mutex writeLock;
mutex readLock;

/**
	Constructor: initialise filenames, buffers, start the threading
*/
ReadStream::ReadStream(int argc, char **argv)
{
	inFile =strLiveFile;
	outFile = strTestFile;
	b = new Buffer;
	b_out = new Buffer;
	t = nullptr;
	start(argc, argv);
}

/**
	Destructor: delete buffers, thread, critical sections
*/
ReadStream::~ReadStream()
{
	cout << "destruct" << endl;
	DeleteCriticalSection(&csRec);
	DeleteCriticalSection(&csLive);

	StopTestImageServer();
	t->join();
	delete t;
	delete b;
	delete b_out;

	if (pps)
		delete pps;
}

/**
	Load the encoded live image file into the buffer.
	Returns true if successful.
*/
bool ReadStream::getLiveImage()
{
	b->reset();
	EnterCriticalSection(&csLive);
	LockAndReadFile(inFile.c_str(), *b);
	LeaveCriticalSection(&csLive);
	if (b->Size())
	{
		char achTS[30];
		memcpy(achTS, b->getData(), TS_LEN);
		achTS[TS_LEN] = 0;
		if (strcmp(achTS, strTS.c_str()))
		{
			b->consume(TS_LEN);
			strTS = achTS;
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

/**
	Writes out modified Mat image to TCP stream handler.
	Warning: very slow, uncomment if you wish to use
*/
void ReadStream::writeStream(Mat img)
{
	// not yet implemented with multithreading
	/*vector<int> p;
	p.push_back(CV_IMWRITE_JPEG_QUALITY);
	p.push_back(100);
	vector<uchar> vout;
	imencode(".jpg", img, vout, p);

	b->reset();
	b->Add((char *)vout.data(), vout.size());

	EnterCriticalSection(&csLive);

	LockAndWriteFile(outFile.c_str(), *b, strTS);
	LeaveCriticalSection(&csLive);*/
}

/**
	Writes out unmodified image to TCP stream handler directly from buffer.
*/
void ReadStream::writeStream()
{
	// Sleep until image capture and streaming buffer can be locked
	buffLock.lock();

	// Enter Lloyd's critical section and write file
	EnterCriticalSection(&csLive);
	LockAndWriteFile(outFile.c_str(), *b_out, strTS);
	LeaveCriticalSection(&csLive);

	// Unlock capture and streaming buffer
	buffLock.unlock();
}

/**
	Load live image and decode for use in OpenCV
*/
bool ReadStream::loadCV(Mat & img)
{
	// Get next image
	if (getLiveImage())
	{
		// Decode image
		img = imdecode(Mat(1, b->Size(), CV_8UC3, b->getData()), 1);

		// Sleep until image capture and streaming buffer can be locked
		buffLock.lock();

		// Write out image buffer to separate streaming buffer
		b_out->reset();
		b_out->Add(b->getData(), b->Size());

		// Unlock image capture and streaming buffer
		buffLock.unlock();
		return true;
	}

	// Return false if for some reason next image wasn't ready
	return false;	
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
	Multithreaded image capture process
*/
void ReadStream::run()
{
	while(1){
		// Sleep until write lock can be set
		writeLock.lock();

		// Get next image from camera
		loadCV(newImg);

		// Unlock image reading, so next frame can be read
		readLock.unlock();
	}
}

/**
	Function called by OpenCV main thread to retrieve image
*/	
void ReadStream::retrieveImage(Mat & img)
{
	// Sleep until read lock can be set
	readLock.lock();

	// Clone stored image
	img = newImg.clone();

	// Unlock image writing, so next frame can be stored
	writeLock.unlock();
}

/**
	Mostly Lloyd's code.
	Start main image retrieving process
*/
void ReadStream::start(int argc, char **argv)
{
	args.setOption("r", "rate", true, "-1");
	args.setOption("iw", "image_width", true, "1280");
	args.setOption("ih", "image_height", true, "720");
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

	// Lock image reading mutex until image is ready to be read
	readLock.lock();

	// Start thread
	t = new thread(&ReadStream::run, this);

}
