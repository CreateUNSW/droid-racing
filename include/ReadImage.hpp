#ifndef READ_IMAGE_HPP_
#define READ_IMAGE_HPP_

#include "buffer.h"

#include <string>
#include <iostream>
#include <thread>

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

	Buffer *b;
};

#endif
