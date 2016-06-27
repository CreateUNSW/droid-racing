#ifndef READ_IMAGE_HPP_
#define READ_IMAGE_HPP_

#include "buffer.h"
#include "utils.h"
//#include "lock.h"
//#include "args.h"
//#include "thread.h"
//#include "status.h"
#include "jpeg.h"

#include <string>
#include <iostream>
#include <thread>
//#include <sys/prctl.h>
//#include <signal.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/syslog.h>
//#include <errno.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

bool getImageBuffer(Buffer & ret);

class ReadStream {
public:
	ReadStream(int argc, char **argv);
	~ReadStream();

	void writeStream(Mat img);


private:
	void start(int argc, char **argv);

	//string inFile;
	//string outFile;

	//string strTS;
	Buffer *b;
	//Buffer * b_out;
	//thread * t;
};

#endif
