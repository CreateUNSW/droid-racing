// cvtest.cpp
#include <mutex>

#include "ReadImage.hpp"

// Port for frame by frame controlled streaming
#define CV_PORT 8889

using namespace std;
using namespace cv;

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
	b = new Buffer;
	start(argc, argv);
}

/**
	Destructor: delete buffers, thread, critical sections
*/
ReadStream::~ReadStream()
{
	cout << "destruct" << endl;

	StopTestImageServer();
	delete b;
}

/**
	Writes out modified Mat image to TCP stream handler.
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
	Start main image retrieving process
*/
void ReadStream::start(int argc, char **argv)
{

	// Start image server on port
	StartTestImageServer(CV_PORT);
}
