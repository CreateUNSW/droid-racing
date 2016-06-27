// C and C++ headers
#include <iostream>
#include <sys/time.h>
#include <cstdlib>
#include <list>

// OpenCV libraries
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Define if processing using still images instead of video
//#define STILL_IMAGES

// Define if motors are to be driven
//#define MOTORS_ENABLE

// Define to display all image output
//#define DISP_TEST

// Define to stream processed footage
#define STREAMING

// GPIO and Arduino-like functions
#include <wiringPi.h>

// Hardware trigger pins
#define FLAG_PIN 3
#define REMOTE_PIN 4

// Our own headers
#include "DriveControl.hpp"
#include "ReadImage.hpp"

// Image width and height constants
static const int iw = 320;
static const int ih = 240;

using namespace std;
using namespace cv;

// File path for sample image processing
static string image_path = "./sample_images/";

typedef VideoCapture camera_t;
typedef enum {RIGHT, LEFT, NEUTRAL} steering_dir_t;

// Globals for correcting oblique perspective
Mat perspectiveMat;
const float aperture = 3.0; // makes 2*apeture - 1 scaling

void detect_path(Mat & grey, double & steeringAngle, double & speed);
void detect_obstacles(Mat hsv, vector<Rect2i> & obj);
bool handle_remote_switch(DriveControl & control);
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

	// Region of interest for scanning for obstacles
	Rect2i ROI(0, ih/2 - 1, iw, ih/2);

	#ifndef STILL_IMAGES
		// Instantiate camera object
		camera_t cam;

		// Initialise camera configuration before opening
		camera_setup(cam);

		// Open camera
		cam.open(0);

		if (!cam.isOpened()){
			cout << "Camera not found" << endl;
			return -1;
		}
	#endif

	#ifdef STREAMING
		// Turn on streaming capability
		cout << "Turning on streaming" << endl;
		ReadStream readStream(argc, argv);
	#endif

	// variables for iterating through sample images
	stringstream filename;
	int imIndex = 462; // first jpeg in samples folder has index 0462

	// Instantiate drive signals
	#ifdef MOTORS_ENABLE
		DriveControl control;
	#endif

	// Instance variables for driving steering and speed
	double steeringAngle = 0;
	double speed = 0;

	// perspective transformation matrix based on aperture skew
	Point2f src[] = {Point2f(0, 0), Point2f(iw - 1, 0), Point2f(aperture * iw, ih - 1), Point2f((1 - aperture) * iw, ih - 1)};
	Point2f dst[] = {Point2f(0,0), Point2f(iw - 1, 0), Point2f(iw - 1, ih - 1), Point2f(0, ih - 1)};
	perspectiveMat = getPerspectiveTransform(src, dst);

	// set up use of GPIO
	cout << "Setting up GPIO" << endl;
	wiringPiSetup();

	// set IO for flag trigger
	pinMode(FLAG_PIN, INPUT);
	pullUpDnControl(FLAG_PIN, PUD_UP);

	// set IO for remote trigger
	pinMode(REMOTE_PIN, INPUT);
	pullUpDnControl(REMOTE_PIN, PUD_UP);

	sleep(0.1);

	// Variable for timing
	uint32_t loopTime;

	#ifdef MOTORS_ENABLE
		// Wait for remote switch to be pressed twice
		while(!handle_remote_switch(control)){}
	#endif

	cout << "Entering main loop" << endl;

	// main loop
	while(1)
	{
		// get time of beginning of loop
		loopTime = millis();

		#ifdef MOTORS_ENABLE
			// check for shutoff
			handle_remote_switch(control);
		#endif

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

		resize(imLarge, im, Size(320,240), 0, 0, CV_INTER_LINEAR);

		// DO NOT USE: currently carried out in detect path function
		//warpPerspective(im, im, perspectiveMat, im.size());

		// display image on screen
		#ifdef DISP_TEST
			imshow("Camera", im);
		#endif

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);
		// convert hsv image to bgr->greyscale for processing
		cvtColor(imHSV, imGrey, COLOR_BGR2GRAY);

		// get list of obstacles in our region of interest and plot
		detect_obstacles(imHSV, obstacles);
		for(auto & el : obstacles){
			//el.x += ROI.x;
			//el.y += ROI.y;

			// draw box around each obstacle
			//rectangle(im, el, Scalar(255,0,0));
			//rectangle(imGrey, el, Scalar(100));

			// draw triangle leading into obstacle for path calculation
			int triangleHeight = min(el.width/2, ih - (el.y + el.height));
			Point2i vertex(el.x + el.width/2, el.y + el.height + triangleHeight);
			Point2i bottomL(el.x, el.y + el.height - 1);
			Point2i bottomR = el.br();
			line(imGrey, bottomL, vertex, Scalar(255), 2);
			line(imGrey, bottomR, vertex, Scalar(255), 2);
		}

		//cout << "Obstacles: " << obstacles.size() << endl;

		// determines path and steering angle, returns corrected image
		detect_path(imGrey, steeringAngle, speed);

		// plot estimated steering path on top of image as circular arc
		if(abs(steeringAngle) > 0.1){
			int radius = abs(1000 / steeringAngle);
			Point2i centre;
			if(steeringAngle > 0){
				centre = Point2i(iw/2 -1 + radius, ih - 1);
			} else {
				centre = Point2i(iw/2 - 1 - radius, ih - 1);
			}
			circle(im, centre, radius, Scalar(0,0,255));
			circle(imGrey, centre, radius, Scalar(255));
		}

		//cout << "Steering angle: " << steeringAngle << "	Speed: " << speed << endl;

		#ifdef MOTORS_ENABLE
			// CHANGE CONSTANTS FOR MODIFIED RESPONSE
			int outAngle;
			if(steeringAngle > 0){
				outAngle = steeringAngle * 20; // right steering constant [change this]
			} else {
				outAngle = steeringAngle * 30; // left steering constant [change this]
				if(outAngle > 250){
					outAngle = 400;
				}
			}
			int outSpeed = speed * 50;	// speed constant [change this]
			cout << "Writing out speed: " << outSpeed << " angle: " << outAngle << endl;
			control.set_desired_speed(outSpeed);
			control.set_desired_steer(outAngle);
		#endif

		// measure HSV colour at the centre of the image, for example
		//cout << imHSV.at<Vec3b>(imHSV.size().width/2,imHSV.size().height/2) << endl;

		// print out info
		//cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;

		// display images on screen
		#ifdef DISP_TEST
			//imshow("HSV image", imHSV);
			imshow("Perspective correction", im);
		#endif

		// allow for images to be displayed on desktop application
		#ifdef STILL_IMAGES
			//cout << "Saving file " << filename.str() << endl;
			//imwrite(filename.str(), imPath);
			cout << "Press any key for next image" << endl;
			// need to be clicked in to one of the image windows (not terminal) for this to work
			waitKey();
		#elif defined DISP_TEST
			waitKey(5);
		#endif

		#ifdef STREAMING
			// stream modified frame over TCP
			readStream.writeStream(imGrey);
		#endif

		// clear vector after use
		obstacles.clear();

		// print out info
		cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;

	}
	return 0;
}

void detect_path(Mat & grey, double & steeringAngle, double & speed)
{
	// get edge binary mask from grey image
	Mat edges;

	/** PLEASE READ UP ON CANNY **/
	Canny(grey, edges, 80, 240); // note edge thresholding numbers

	// distort edge binary and grey image to correct for oblique perspective
	warpPerspective(edges, edges, perspectiveMat, edges.size());
	warpPerspective(grey, grey, perspectiveMat, grey.size());

	// Path matrix, which calculated path will be drawn on
	//Mat centrePath = Mat::zeros(grey.size(), CV_8UC1);

	// Integer variables for features of image
	int leftBorder, rightBorder, row;
	int height = grey.size().height;
	int width = grey.size().width;

	// effect of perspective increases for further away objects
	double perspectiveDistance;

	// doubles for inputs into the PD controller which generates the line
	double centre = width/2;
	double deltaF = 0;
	double oldF = 0;
	double difference = 0;
	steeringAngle = 0;
	double startAccel = 0;

	//steeringAngle = 0;
	double maxAccel = 0;
	int maxRow = height - 1;

	// keeps track of the presence of the left and right edges in each row of any frame
	bool rightFound, leftFound;

	// corner analysis
	bool turnInitiated = false;
	bool turnFinished = false;
	steering_dir_t steeringDirection = NEUTRAL;
	list<double> accels;

	// iterate from the bottom (droid) edge of the image to the top
	for(row = height - 1; row >= 0; --row){

		perspectiveDistance = width*(1-1/(2*aperture-1))*((double)row/(double)(1.5*height-1))/2;
		//perspectiveDistance = 0;
		//if(row == 30){
		//	cout << "Row: " << row << "	perspectivePixels: " << perspectiveCalc << endl;
		//}
		// start looking for left border from the centre path, iterate outwards
		leftBorder = centre;
		leftFound = false;
		while(leftBorder > perspectiveDistance){//(leftBorder > 0){
			leftBorder--;
			if(edges.at<uchar>(row, leftBorder) > 0){
				// if an edge is found, assume its the border, and break
				break;
			}
		}

		// start looking for right border from the centre path, iterate outwards
		rightBorder = centre;
		rightFound = false;
		while(rightBorder < width - perspectiveDistance){//(rightBorder < width){}
			rightBorder++;
			if(edges.at<uchar>(row, rightBorder) > 0){
				// if an edge is found, assume its the border, and break
				break;
			}
		}

		// feed calculated centre of the borders into the controller
		double deadCentre = (leftBorder + rightBorder)/2;
		// proportional term
		double ff = deadCentre - centre;
		// derivative term
		deltaF = ff - oldF;
		// cap derivative term in case of sudden jumps in path
		deltaF = max(deltaF, -3.0); deltaF = min(deltaF, 3.0);

		double accel;

		// feed terms into horizontal rate accross image
		if(row == height - 1){
			accel = 0;// ff * 0.005;
		} else {
			accel = ff * 0.001 + deltaF * 0.01;
		}

		// cap horizontal acceleration
		accel = min(accel, 0.3); accel = max(accel, -0.3);
		difference += accel;

		// if the end of the first turn hasn't been reached
		if(!turnFinished){
			// if the start of the first bend is detected
			if(!turnInitiated && abs(steeringAngle) > 0.12){
				// record initial turn direction
				turnInitiated = true;
				if(steeringAngle > 0){
					steeringDirection = RIGHT;
				} else {
					steeringDirection = LEFT;
				}
			}

			// record maximum acceleration in first turn
			if(abs(accel) > maxAccel){
				maxAccel = abs(accel);
				maxRow = row;
			}

			// generate the steering angle with a sum of accelerations
			steeringAngle += accel;

			// record the last few accelerations and produce an average
			accels.push_back(accel);
			if(accels.size() > 5){
				accels.pop_front();
			}
			double meanAccel = 0;
			for(auto el : accels){
				meanAccel += el;
			}
			meanAccel /= accels.size();

			// look for the end of a turn
			if(row < height - 200) {
				// no turn near
				turnFinished = true;
			} else if (meanAccel < 0 && steeringDirection == RIGHT){
				// acceleration to the left in a right turn
				turnFinished = true;
			} else if (meanAccel > 0 && steeringDirection == LEFT){
				// acceleration to the right in a left turn
				turnFinished = true;
			}

			if(turnFinished){
				// get the mean acceleration in the turn
				steeringAngle /= (height - row);

				// adjust steering angle with a signed square and other constants
				steeringAngle *= 10;
				if(steeringAngle >= 0){
					steeringAngle *= steeringAngle;
				} else {
					steeringAngle *= steeringAngle;
					steeringAngle = -steeringAngle;
				}
				//steeringAngle *= (steeringAngle * steeringAngle);
				steeringAngle *= 360;

				// slow down speed based on the maximum acceleration in the turn
				speed = 1 - 5*maxAccel;
				speed = min(0.75, speed);
			}
		}

		// adjust centre of path accordingly
		centre += difference;
		// bound path by edges of image
		centre = max(2.0, centre); centre = min(width-3.0, centre);
		// track past proportional term, used to calculate derivative term
		oldF = ff;

		// draw path pixels on images
		for(int i = - 1; i <=	1; ++i){
			grey.at<uchar>(row,	(int)deadCentre + i) = uchar(0);
			//centrePath.at<uchar>(row, (int)centre + i) = uchar(255);
			grey.at<uchar>(row, (int)centre + i) = uchar(255);
		}

	}

	// show binary mask
	#ifdef DISP_TEST
		imshow("Canny", edges);
		//imshow("Centre path", centrePath);
		imshow("Grey with path superimposed", grey);
	#endif

	//cout << "Max accel: " << maxAccel << " at row " << maxRow << endl;
}

void detect_obstacles(Mat hsv, vector<Rect2i> & obj)
{
	Mat mask;

	// generate mask of purple colours
	//inRange(hsv, Scalar(110, 100, 80), Scalar(130, 255, 255), mask);
	// generate mask of all saturated colours
	inRange(hsv, Scalar(0, 50, 50), Scalar(255, 255, 255), mask);

	// eliminate noise
	int n = 2;
	Mat element = getStructuringElement(MORPH_RECT, Size(n*2+1, n*2+1), Point(n, n));
	morphologyEx(mask, mask, MORPH_OPEN, element);

	//imshow("Obstacle mask", mask);

	// variables for defining objects
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	// draw convex hulls around detected contours
	findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
	for(size_t i = 0; i < contours.size(); ++i){
		drawContours(mask, contours, (int)i, Scalar(255), CV_FILLED);
	}
	n = 4;
	element = getStructuringElement(MORPH_RECT, Size(n*2+1, n*2+1), Point(n,n));
	erode(mask, mask, element);

	findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0,0) );

	//vector<vector<Point>> contours_poly( contours.size() );

	// get rectangles of large obstacles
	for(size_t i = 0; i < contours.size(); ++i){
		//approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true);
		if(contourArea(contours[i]) > 2000){
			Rect object = boundingRect( Mat(contours[i]) );
			obj.push_back(object);
		}
	}
}

bool handle_remote_switch(DriveControl & control)
{
	if(digitalRead(FLAG_PIN) == LOW || digitalRead(REMOTE_PIN) == LOW){
		cout << "Switch detected: stopping motors" << endl;
		cout << "Flag pin: " << digitalRead(FLAG_PIN) << "	Remote pin: " << digitalRead(REMOTE_PIN) << endl;
		// flag or remote has activated, shut off motors
		control.emergency_stop();
		sleep(1.0);

		// wait for remote and flag to be reset
		while(digitalRead(FLAG_PIN) == LOW || digitalRead(REMOTE_PIN) == LOW){}

		cout << "Switches ready" << endl;

		// wait for remote to be hit
		while(digitalRead(REMOTE_PIN) == HIGH){}
		cout << "Remote hit" << endl;
		sleep(0.1);

		// wait for remote signal to stop
		while(digitalRead(REMOTE_PIN) == LOW){}
		cout << "Remote released" << endl;
		sleep(0.1);

		return true;
	}
	return false;

}

void camera_setup(camera_t & cam)
{
	cam.set(CV_CAP_PROP_FORMAT, CV_8UC3); // 3 channel image
	cam.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	cam.set(CV_CAP_PROP_BRIGHTNESS, 80); // increased brightness
	cam.set(CV_CAP_PROP_SATURATION, 80); // increased saturation
	//cam.set(CV_CAP_PROP_AUTOFOCUS, 0); // set focus to manual
	//cam.set(CV_CAP_PROP_FOCUS, 255); // need to check if this works
	//cam.set(CV_CAP_PROP_EXPOSURE, 10);
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
