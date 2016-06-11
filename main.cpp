// C and C++ headers
#include <iostream>
#include <sys/time.h>
#include <cstdlib>

// OpenCV libraries
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Define if using a normal camera instead of raspberry pi cam
//#define USB_WEBCAM

// Define if processing using still images instead of video
#define STILL_IMAGES

#ifndef USB_WEBCAM
// Raspicam CV
#include <raspicam/raspicam_cv.h>
#endif

// GPIO and Arduino-like functions
#include <wiringPi.h>

// Our own headers
#include "DriveControl.hpp"

// Image width and height constants
static const int iw = 320;
static const int ih = 240;

using namespace std;
using namespace cv;

static string image_path = "/home/pi/droid-racing/sample_images/";

// define camera_t as raspberry pi cv camera class (or usb webcam)
#ifndef USB_WEBCAM
	typedef raspicam::RaspiCam_Cv camera_t;
#else
	typedef VideoCapture camera_t;
#endif

void detect_path(Mat grey);
void detect_obstacles(Mat hsv, vector<Rect2i> & obj);
void camera_setup(camera_t & cam);
void sleep(double seconds);

int main(int argc, char * argv[])
{	
	// Mat variables for storage of images
	Mat im, imHSV, imGrey;

	// Image channels for better analysis
	vector<Mat> channels(3);

	// Vector for storing locations of objects
	vector<Rect2i> obstacles;

	// Region of interest for scanning
	// bottom half of image
	Rect2i ROI = Rect2i(0, ih/2, iw, ih/2);

	#ifndef STILL_IMAGES
		// Instantiate camera object
		camera_t cam;

		// Initialise camera configuration before opening
		camera_setup(cam);

		// Open camera
		cam.open(0);
		if (!cam.isOpened()){
			return -1;
		}
	#endif

	// variables for iterating through sample images
	stringstream filename;
	int imIndex = 462; // first jpeg in samples folder has index 0462

	// Instantiate drive signals
	//DriveControl control;

	// Variable for timing
	uint32_t loopTime;

	// main loop
	while(1)
	{
		// get time of beginning of loop
		loopTime = millis();

		// get next image
		#ifndef STILL_IMAGES
			// if using camera, grab image
			cam.grab();
			cam.retrieve(im);

		#else
			// if using sample images, load next image

			// clear filename
			filename.clear();
			filename.str(string());

			// form filename from stubs
			filename << image_path << "IMG_0" << imIndex << ".jpg";
			cout << "Reading " << filename.str() << endl;

			// read file
			im = imread(filename.str());
			if(im.data==NULL){
				// exit on end
				cout << "Reached end of images" << endl;
				return 0;
			}
			// increment file index
			imIndex++;
		#endif

		// currently resize to 320x240 because webcam won't let me read at that
		resize(im, im, Size(iw,ih), 0, 0, CV_INTER_LINEAR);

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);

		/**COLOUR LINE METHOD 1**/		
		// split HSV channels
		//split(imHSV, channels);
		//detect_path(channels[1]);

		/**COLOUR LINE METHOD 2 - THE BEST CURRENTLY**/
		// convert hsv image to bgr->greyscale for processing
		cvtColor(imHSV, imGrey, COLOR_BGR2GRAY);
		detect_path(imGrey);
		
		// measure HSV colour at the centre of the image, for example
		//cout << imHSV.at<Vec3b>(imHSV.size().width/2,imHSV.size().height/2) << endl;

		// get list of obstacles
		detect_obstacles(imHSV(ROI), obstacles);
		for(auto & el : obstacles){
			el.x += ROI.x;
			el.y += ROI.y;
			rectangle(im, el, Scalar(255,0,0));
		}

		// print out info
		cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;

		// display image on screen
		imshow("camera", im);

		// allow for images to be displayed on desktop application
		#ifdef STILL_IMAGES
			cout << "Press any key for next image" << endl;
			// need to be clicked in to one of the image windows (not terminal) for this to work
			waitKey();
		#else
			waitKey(5);
		#endif

		// clear vector after use
		obstacles.clear();
	}
	return 0;
}

void detect_path(Mat grey)
{
	// get edge binary mask from grey image
	Mat edges;
	/** PLEASE READ UP ON CANNY **/
	Canny(grey, edges, 60, 180); // note edge thresholding numbers

	// show binary mask
	imshow("Canny", edges);

	// get approximate contours from binary mask
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	findContours(edges, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0,0));

	vector<vector<Point>> contours_poly(contours.size());
	for(size_t i = 0; i < contours.size(); ++i){
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true);
	}
}

void detect_obstacles(Mat hsv, vector<Rect2i> & obj)
{
	Mat mask;

	// generate mask of purple colours
	inRange(hsv, Scalar(110, 100, 80), Scalar(130, 255, 255), mask);

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
	#ifndef USB_WEBCAM
		cam.set(CV_CAP_PROP_FORMAT, CV_8UC3);
		cam.set(CV_CAP_PROP_FRAME_WIDTH, iw);
		cam.set(CV_CAP_PROP_FRAME_HEIGHT, ih);
	#else
		// does this work?
		cam.set(CV_CAP_PROP_FRAME_WIDTH, iw);
		cam.set(CV_CAP_PROP_FRAME_HEIGHT, ih);
		cam.set(CV_CAP_PROP_EXPOSURE, 10);
	#endif
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
