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
#define MOTORS_ENABLE

// Define to display all image output
//#define DISP_TEST

// Define to stream processed footage
//#define STREAMING

// GPIO and Arduino-like functions
#include <wiringPi.h>

// Flag pin
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

static string image_path = "./sample_images/";

typedef VideoCapture camera_t;
typedef enum {RIGHT, LEFT, NEUTRAL} steering_dir_t;

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

	// Region of interest for scanning
	// bottom half of image
	//Rect2i ROI(0, ih/4, iw, 3*ih/4);
	Rect2i ROI(0, 0, iw, ih);

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
		ReadStream readStream(argc, argv);
	#endif

	// variables for iterating through sample images
	stringstream filename;
	int imIndex = 462; // first jpeg in samples folder has index 0462

	// Instantiate drive signals
	#ifdef MOTORS_ENABLE
		DriveControl control;
	#endif
	double steeringAngle = 0;
	double speed = 0;

	Point2f src[] = {Point2f(0, 0), Point2f(ROI.width-1, 0), Point2f(aperture*ROI.width, ROI.height-1), Point2f((1-aperture)*ROI.width,ROI.height-1)};
	Point2f dst[] = {Point2f(0,0), Point2f(ROI.width-1, 0), Point2f(ROI.width-1, ROI.height-1), Point2f(0, ROI.height-1)};
	perspectiveMat = getPerspectiveTransform(src, dst);

	wiringPiSetup();

	pinMode(FLAG_PIN, INPUT);
	pullUpDnControl(FLAG_PIN, PUD_UP);

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

		// display image on screen
		#ifdef DISP_TEST
			imshow("Camera", im);
		#endif

		//warpPerspective(im, im, perspectiveMat, im.size());

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);

		/**COLOUR LINE METHOD 2 - THE BEST CURRENTLY**/
		// convert hsv image to bgr->greyscale for processing
		cvtColor(imHSV, imGrey, COLOR_BGR2GRAY);

		//imshow("HSV image", imHSV);
		//cvtColor(im, imGrey, COLOR_BGR2GRAY);

		// determines path and steering angle, returns corrected image
		detect_path(imGrey, steeringAngle, speed);

		//warpPerspective(im, im, perspectiveMat, im.size());
		if(abs(steeringAngle) > 0.1){
			int radius = abs(1000 / steeringAngle);
			Point2i centre;
			if(steeringAngle > 0){
				centre = Point2i(iw/2 -1 + radius, ih - 1);
			} else {
				centre = Point2i(iw/2 - 1 - radius, ih - 1);
			}
			//circle(im, centre, radius, Scalar(0,0,255));
			circle(imGrey, centre, radius, Scalar(255));
		}
		//cout << "Steering angle: " << steeringAngle << "  Speed: " << speed << endl;

		#ifdef MOTORS_ENABLE
			int outAngle;
			if(steeringAngle > 0){
				outAngle = steeringAngle*40;
			} else {
				outAngle = steeringAngle*70;
			}
			int outSpeed = speed * 20;
			cout << "Writing out speed: " << outSpeed << " angle: " << outAngle << endl;
			control.set_desired_speed(outSpeed);
			control.set_desired_steer(outAngle);
		#endif

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
		//cout << "Image size: " << im.size() << " Loop time: " << millis() - loopTime << endl;

		// display image on screen
		#ifdef DISP_TEST
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
			readStream.writeStream(imGrey);
			//imwrite(filename.str(), imPath);
			//cout << "Wrote out image" << endl;
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
	//warpPerspective(grey, grey, perspectiveMat, grey.size());

	//imshow("Single channel path frame", grey);

	// get edge binary mask from grey image
	Mat edges;

	/** PLEASE READ UP ON CANNY **/
	Canny(grey, edges, 80, 240); // note edge thresholding numbers

	warpPerspective(edges, edges, perspectiveMat, edges.size());
	warpPerspective(grey, grey, perspectiveMat, grey.size());


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
	steeringAngle = 0;
	double startAccel = 0;

	//steeringAngle = 0;
	double maxAccel = 0;
	int maxRow = height - 1;

	// keeps track of the presence of the left and right edges in each row of any frame
	bool rightFound, leftFound;
	double perspectiveCalc;

	// corner analysis
	bool turnInitiated = false;
	bool turnFinished = false;
	steering_dir_t steeringDirection = NEUTRAL;
	list<double> accels;

	// iterate from the bottom (droid) edge of the image to the top
	for(row = height - 1; row >= 0; --row){

		perspectiveCalc = width*(1-1/(2*aperture-1))*(row/(height-1))/2;

		// start looking for left border from the centre path, iterate outwards
		leftBorder = centre;
		leftFound = false;
		while(leftBorder > perspectiveCalc){
			leftBorder--;
			if(edges.at<uchar>(row, leftBorder) > 0){
				// if an edge is found, assume its the border, and break
				break;
			}
		}

		// start looking for right border from the centre path, iterate outwards
		rightBorder = centre;
		rightFound = false;
		while(rightBorder < width-perspectiveCalc){
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

		accel = min(accel, 0.3); accel = max(accel, -0.3);
		difference += accel;

		//if(row > 80){ // some threshold up the image after which sharpness of corners doesn't matter
			if(abs(accel) > maxAccel){
				maxAccel = abs(accel);
				maxRow = row;
			}
		//}

		if(!turnFinished){

			if(!turnInitiated && abs(steeringAngle) > 0.5){
				turnInitiated = true;
				if(steeringAngle > 0){
					steeringDirection = RIGHT;
				} else {
					steeringDirection = LEFT;
				}
			}

			steeringAngle += accel;
			accels.push_back(accel);
			if(accels.size() > 5){
				accels.pop_front();
			}
			double sum = 0;
			for(auto el : accels){
				sum += el;
			}
			sum /= accels.size();

			if(row < height - 120) {
				turnFinished = true;
			} else if (sum < 0 && steeringDirection == RIGHT){
				turnFinished = true;
			} else if (sum > 0 && steeringDirection == LEFT){
				turnFinished = true;
			}

			if(turnFinished){
				if(steeringAngle != NEUTRAL){
					steeringAngle /= (height - row);
					steeringAngle *= 360;
				} else {
					steeringAngle = 0;
				}
			}
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

	accels.clear();

	// show binary mask
	#ifdef DISP_TEST
		imshow("Canny", edges);
		imshow("Centre path", centrePath);
		imshow("Grey with path superimposed", grey);
	#endif

	//cout << "Max accel: " << maxAccel << " at row " << maxRow << endl;
	speed = 1 - maxAccel;
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
	#ifdef DISP_TEST
	//imshow("mask", mask);
	#endif

	// get shapes around convex hulls
	findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
	vector<vector<Point>> contours_poly( contours.size() );
	for(size_t i = 0; i < contours.size(); ++i){
		approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true);
		obj.push_back(boundingRect( Mat(contours_poly[i]) ));
	}
}

bool handle_remote_switch(DriveControl & control)
{
	if(digitalRead(FLAG_PIN) == LOW || digitalRead(REMOTE_PIN) == LOW){
		cout << "Switch detected: stopping motors" << endl;
		cout << "Flag pin: " << digitalRead(FLAG_PIN) << "  Remote pin: " << digitalRead(REMOTE_PIN) << endl;
		// flag or remote has activated, shut off motors
                control.set_desired_speed(0);
		control.set_desired_steer(0);
		sleep(0.5);

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
	// does this work?
	cam.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	cam.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
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
