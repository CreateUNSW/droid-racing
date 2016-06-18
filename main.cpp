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

Mat detect_path(Mat grey);
void detect_obstacles(Mat hsv, vector<Rect2i> & obj);
void camera_setup(camera_t & cam);
void sleep(double seconds);

int main(int argc, char * argv[])
{	
	// Mat variables for storage of images
	Mat im, imHSV, imGrey, imLarge, binaryPath;

	// Image channels for better analysis
	vector<Mat> channels(3);

	// Vector for storing locations of objects
	vector<Rect2i> obstacles;

	// Region of interest for scanning
	// bottom half of image
	Rect2i ROI = Rect2i(0, ih/4, iw, 3*ih/4);

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
			cam.retrieve(imLarge);

		#else
			// if using sample images, load next image

			// clear filename
			filename.clear();
			filename.str(string());

			// form filename from stubs
			filename << image_path << "IMG_0" << imIndex << ".jpg";
			cout << "Reading " << filename.str() << endl;

			// read file
			imLarge = imread(filename.str());
			if(imLarge.data==NULL){
				// exit on end
				cout << "Reached end of images" << endl;
				return 0;
			}

			// out-file filename for saving
			filename.clear();
			filename.str(string());
			filename << image_path << "out" << imIndex << ".jpg";

			// increment file index
			imIndex++;
		#endif

		// currently resize to 320x240 because webcam won't let me read at that
		resize(imLarge, im, Size(iw,ih), 0, 0, CV_INTER_LINEAR);

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);

		/**COLOUR LINE METHOD 1**/		
		// split HSV channels
		//split(imHSV, channels);
		//binaryPath = detect_path(channels[1](ROI));

		/**COLOUR LINE METHOD 2 - THE BEST CURRENTLY**/
		// convert hsv image to bgr->greyscale for processing
		cvtColor(imHSV, imGrey, COLOR_BGR2GRAY);
		binaryPath = detect_path(imGrey(ROI));

		/**COLOUR LINE METHOD 3 - SATURATION AND VOLUME**/
		// combine hue and saturation into new image
		//split(imHSV, channels);
		//Mat imSV = (channels[2]/2 + channels[1]/2);
		//binaryPath = detect_path(imSV(ROI));

		// saturation
		//Mat mask;
		//inRange(imHSV(ROI), Scalar(0, 80, 40), Scalar(255, 255, 255), mask);
		//imshow("Saturation mask", mask);
		
		// measure HSV colour at the centre of the image, for example
		//cout << imHSV.at<Vec3b>(imHSV.size().width/2,imHSV.size().height/2) << endl;

		/**DISPLAY PATH ON TOP OF IMAGE**/
		Mat imPath = im(ROI).clone();
		Mat invBinaryPath, threeChannelMask;
		Mat zeroMask = Mat::zeros(imPath.size(), CV_8UC1);
		bitwise_not(binaryPath,invBinaryPath);

		Mat pathMasks[] = {invBinaryPath, invBinaryPath, invBinaryPath};
		merge(pathMasks, 3, threeChannelMask);
		bitwise_and(imPath, threeChannelMask, imPath);

		Mat redMask[] = {zeroMask, zeroMask, binaryPath};
		merge(redMask, 3, threeChannelMask);
		bitwise_or(imPath, threeChannelMask, imPath);

		imshow("path", imPath);

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
		//imshow("camera", im);

		// allow for images to be displayed on desktop application
		#ifdef STILL_IMAGES
			//cout << "Saving file " << filename.str() << endl;
			//imwrite(filename.str(), imPath);
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

Mat detect_path(Mat grey)
{
	imshow("Single channel path frame", grey);

	// get edge binary mask from grey image
	Mat edges;

	/** PLEASE READ UP ON CANNY **/
	Canny(grey, edges, 80, 240); // note edge thresholding numbers

	// show binary mask
	imshow("Canny", edges);

	/*vector<Vec4i> lines;
	HoughLinesP(edges, lines, 1, CV_PI/180, 10, 10, 20);
	Mat hough = Mat::zeros(grey.size(), CV_8UC3);
	for(size_t i = 0; i < lines.size(); ++i){
		double grad = (double)(lines[i][3] - lines[i][1]) / (double)(lines[i][2]-lines[i][0]);
		if(abs(grad) > 0.2){
			line(hough, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), Scalar(0,0,255), 1, 8);
		}
	}
	imshow("Hough", hough);*/

	// Path matrix, which calculated path will be drawn on
	Mat centrePath = Mat::zeros(grey.size(), CV_8UC1);
	// Integer variables for features of image
	int leftBorder, rightBorder, row;
	int height = centrePath.size().height;
	int width = centrePath.size().width;
	// effect of perspective increases for further away objects
	int perspectiveDistance;

	// doubles for inputs into the PD controller which generates the line
	double centre = width/2;
	double deltaF = 0;
	double oldF = 0;
	double difference = 0;

	// keeps track of the presence of the left and right edges in each row of any frame
	bool rightFound, leftFound;

	// iterate from the bottom (droid) edge of the image to the top
	for(row = height - 1; row >= 0; --row){
		// calculate horizontal pseudo-effect of perspective 
		perspectiveDistance = (height- row) * width / (3 * height);

		// start looking for left border from the centre path, iterate outwards
		leftBorder = centre;
		leftFound = false;
		while(leftBorder > 0){
			leftBorder--;
			if(edges.at<uchar>(row, leftBorder) > 0){
				// if an edge is found, assume its the border, and break
				leftFound = true;
				break;
			}
		}

		// start looking for right border from the centre path, iterate outwards
		rightBorder = centre;
		rightFound = false;
		while(rightBorder < width){
			rightBorder++;
			if(edges.at<uchar>(row, rightBorder) > 0){
				// if an edge is found, assume its the border, and break
				rightFound = true;
				break;
			}
		}

		// if either border was not found, assume it is a certain distance away from the path based on perspective
		if(!leftFound){
			leftBorder = max(0, (int)centre - (width/2 - perspectiveDistance));
		}
		if(!rightFound){
			rightBorder = min(width, (int)centre + (width/2 - perspectiveDistance));
		}

		// feed calculated centre of the borders into the controller
		double deadCentre = (leftBorder + rightBorder)/2;	
		// proportional term
		double ff = deadCentre - centre;
		// derivative term
		deltaF = ff - oldF;
		// cap derivative term in case of sudden jumps in path
		deltaF = max(deltaF, -5.0); deltaF = min(deltaF, 5.0);

		// feed terms into horizontal rate accross image
		if(row == height - 1){
			difference += ff * 0.01;
		} else {
			difference += ff * 0.01 + deltaF * 0.1;
		}

		// adjust centre of path accordingly
		centre += difference;
		// bound path by edges of image
		centre = max(2.0, centre); centre = min(width-3.0, centre);
		// track past proportional term, used to calculate derivative term
		oldF = ff;

		// draw path pixels on images
		for(int i = - 1; i <=  1; ++i){
			grey.at<uchar>(row,  (int)deadCentre + i) = uchar(0);
			centrePath.at<uchar>(row, (int)centre + i) = uchar(255);
			grey.at<uchar>(row, (int)centre + i) = uchar(255);
		}
		
	}
	imshow("Centre path", centrePath);

	imshow("Grey with path superimposed", grey);

	return centrePath;

	// get approximate contours from binary mask
	/*vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	findContours(edges, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0,0));

	Mat edgeContours = Mat::zeros(grey.size(), CV_8UC3);
	for(size_t i = 0; i < contours.size(); ++i){
		if(arcLength(contours[i], true) > 100){
			drawContours(edgeContours, contours, i, Scalar(rand()& 255, rand()& 255, rand()& 255));
		}
	}
	
	imshow("Edge contours", edgeContours);*/
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
	//imshow("mask", mask);

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
