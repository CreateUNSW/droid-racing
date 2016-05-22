// C and C++ headers
#include <iostream>
#include <sys/time.h>

// OpenCV libraries
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Raspicam CV
#include <raspicam/raspicam_cv.h>

// Image width and height constants
const int iw = 640;
const int ih = 480;

using namespace std;
using namespace cv;

// define camera_t as raspberry pi cv camera class
typedef raspicam::RaspiCam_Cv camera_t;

void camera_setup(camera_t & cam);
uint32_t millis();

int main(int argc, char * argv[])
{
	// Instantiate camera object
	camera_t cam;
	
	// Mat variables for storage of images
	Mat im, imSmall, imHSV;

	// Vector for storing locations of objects
	vector<Rect2i> obstacles;

	// Region of interest for scanning
	// bottom half of image
	Rect2i ROI = Rect2i(0, ih/2 - 1, iw, ih/2);

	// Initialise camera configuration before opening
	camera_setup(cam);

	// Open camera
	cam.open();
	if (!cam.isOpened()){
		return -1;
	}
	
	// Variable for timing
	uint32_t loopTime;

	// main loop
	while(1)
	{
		// get time of beginning of loop
		loopTime = millis();

		// get next image
		cam.grab();
		cam.retrieve(im);

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);
		
		// display image on screen
		//imshow("camera", im);
		//waitKey(1);

		// print out info
		cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;
	}
	return 0;
}

void detect_obstacles(Mat & hsv, vector<Rect2i> & obj)
{
	
}

void camera_setup(camera_t & cam)
{
	/**Sets a property in the VideoCapture. 
	* 
	* 
	* Implemented properties:
	* CV_CAP_PROP_FRAME_WIDTH,CV_CAP_PROP_FRAME_HEIGHT,
	* CV_CAP_PROP_FORMAT: CV_8UC1 or CV_8UC3
	* CV_CAP_PROP_BRIGHTNESS: [0,100]
	* CV_CAP_PROP_CONTRAST: [0,100]
	* CV_CAP_PROP_SATURATION: [0,100]
	* CV_CAP_PROP_GAIN: (iso): [0,100]
	* CV_CAP_PROP_EXPOSURE: -1 auto. [1,100] shutter speed from 0 to 33ms
	* CV_CAP_PROP_WHITE_BALANCE_RED_V : [1,100] -1 auto whitebalance
	* CV_CAP_PROP_WHITE_BALANCE_BLUE_U : [1,100] -1 auto whitebalance
	*
	*/
	
	// For example
	//cam.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, iw);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, ih);
}

uint32_t millis()
{
	struct timeval current;
	uint32_t mtime, seconds, useconds;

	gettimeofday(&current, NULL);

	seconds = current.tv_sec;
	useconds = current.tv_usec;

	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

	return mtime;
}
