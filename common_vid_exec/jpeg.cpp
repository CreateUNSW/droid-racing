// jpeg.cpp
#include <stdio.h>
#include "wininc.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "jpeg.h"
#include "utils.h"
#include "lock.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

bool
CVSimpleResizeJpeg(Buffer &buffer, int width, int height, int iRotate)
{
	cv::Mat img = cv::imdecode(cv::Mat(1, buffer.Size(), CV_8UC3, buffer.getData()), 1);
	cv::Mat &res = img;

	cv::Mat dst;

	cv::Size rs;
	if (iRotate == 90 || iRotate == 270)
	{
		rs.width = height;
		rs.height = width;
	}
	else
	{
		rs.width = width;
		rs.height = height;
	}

	if (rs.width != img.rows || rs.height != img.cols)
	{
		cv::resize(img, dst, rs);
		res = dst;
	}
	else
	{
		if (iRotate == 0)
		{
			//nothing doing
			return false;
		}
	}
	if (iRotate)
	{
		//cv::rotate(res, iRotate);
	}

	std::vector<uchar> v;
	cv::imencode(".jpg", res, v);
	buffer.reset();
	buffer.Add((char *)v.data(), v.size());
	return true;
}
