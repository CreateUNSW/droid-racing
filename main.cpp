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

void detect_obstacles(Mat hsv, vector<Rect2i> & obj);
void camera_setup(camera_t & cam);
uint32_t millis();
void sleep(double seconds);

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

		// measure HSV colour at the centre of the image, for example
		//cout << imHSV.at<Vec3b>(imHSV.size().width/2,imHSV.size().height/2) << endl;

		// get list of obstacles
		detect_obstacles(imHSV(ROI), obstacles);
		for(auto & el : obstacles){
			el.x += ROI.x;
			el.y += ROI.y;
			rectangle(im, el, Scalar(255,0,0));
		}
		
		// display image on screen
		imshow("camera", im);

		// allow for images to be displayed on desktop application
		waitKey(1);

		// clear vector after use
		obstacles.clear();

		// print out info
		cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;
	}
	return 0;
}

void detect_obstacles(Mat hsv, vector<Rect2i> & obj)
{
	Mat mask;

	// generate mask of purple colours
	inRange(hsv, Scalar(100, 65, 40), Scalar(140, 255, 255), mask);

	// eliminate noise
	int n = 2;
	Mat element = getStructuringElement(MORPH_RECT, Size(n*2+1, n*2+1), Point(n, n));
	morphologyEx(mask, mask, MORPH_OPEN, element);

	// variables for defining objects
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	
	// draw convex hulls around detected contours
	findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
	vector<vector<Point>> hull(contours.size());
	for(size_t i = 0; i < contours.size(); ++i){
		convexHull( Mat(contours[i]), hull[i], false);
		drawContours(mask, hull, (int)i, Scalar(255), CV_FILLED);
	}
	contours.clear();
	hierarchy.clear();

	// display mask
	imshow("mask", mask);

	// get shapes around convex hulls
	findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );	
	vector<vector<Point>> contours_poly( contours.size() );
	for(size_t i = 0; i < contours.size(); ++i){
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true);
		obj.push_back(boundingRect( Mat(contours_poly[i]) ));
	}
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
	cam.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, iw);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, ih);
}

/*
*	Get current pi millisecond time
*/
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

/*
*	Sleep pi for certain amount of time
*/
void sleep(double seconds)
{
	int wholeSeconds = (int)seconds;
	double remainderSeconds = seconds - wholeSeconds;
	long int wholeRemainderNanoseconds = round(remainderSeconds * 1e9);

	struct timespec t, t2;
	t.tv_sec = wholeSeconds;
	t.tv_nsec = wholeRemainderNanoseconds;
	nanosleep(&t, &t2);
}
