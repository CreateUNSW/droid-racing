#include <iostream>
#include <cstdlib>
#include <chrono>
#include <string>

// OpenCV libraries
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;
using namespace std::chrono;

#define DISP_TEST

// Image width and height constants
static const int iw = 960;
static const int ih = 540;

typedef VideoCapture camera_t;

void camera_setup(camera_t & cam);
void sleep(double seconds);

int main(int argc, char * argv[])
{
	// Mat variables for storage of images
	Mat im, imHSV, imLarge;

	// Image channels for better analysis
	vector<Mat> channels(3);

    // Instantiate camera object
    camera_t cam;

    // Initialise camera configuration before opening
    //camera_setup(cam);

    // Open camera
    if(argc == 2){
        cam.open(argv[1]);
        //cam.open("test2.mp4");
    } else if(argc == 1){
        cam.open(0);
    } else {
        cout << "Incorrect usage" << endl;
        return 1;
    }

    if (!cam.isOpened()){
        cout << "Camera not found" << endl;
        return -1;
    }

	sleep(0.1);

	// Variable for timing
	std::cout << "Entering main loop" << endl;

    milliseconds start_time, end_time, diff;

	// main loop
	while(1)
	{
		// get time of beginning of loop
		start_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        
		// get next image
        // if using camera, grab image
        if(!cam.grab()){
            /*waitKey(30);
            if(!cam.grab()){
                cout << "No more frames" << endl;
                return 0;
            }*/
        }

        cam.retrieve(imLarge);

		resize(imLarge, im, Size(iw,ih), 0, 0, CV_INTER_LINEAR);
        //im = imLarge;
        
        split(im, channels);
        Mat imBlue = Mat::zeros(im.size(),CV_8UC1);
        Mat imYellow = Mat::zeros(im.size(),CV_8UC1);
        Mat imMagenta = Mat::zeros(im.size(),CV_8UC1);
        
        imBlue = channels.at(0)-(channels.at(1)+channels.at(2))/2;

		// convert image to HSV for processing
		cvtColor(im, imHSV, COLOR_BGR2HSV);
        split(imHSV, channels);
        Mat imV = channels.at(2);
        
		// Rotate hue channel to get Yellow, Magenta and Cyan
		for(MatIterator_<uint8_t> it = channels.at(0).begin<uint8_t>(); it!= channels.at(0).end<uint8_t>(); it++){
			*it = (*it + 85) % 255;
		}

        Mat imNewHsv, imNewYCM;
        merge(channels, imNewHsv);
        cvtColor(imNewHsv, imNewYCM, COLOR_HSV2BGR);
        split(imNewYCM, channels);
        imYellow = channels.at(0)-(channels.at(1)+channels.at(2))/2;
        imMagenta = channels.at(1)-(channels.at(0)+channels.at(2))/2;
        
        /*for(int i = 0; i < imYellow.rows; ++i){
            for(int j = 0; j < imYellow.cols; ++j){
                imYellow.at<uint8_t>(i,j) = min(255, imYellow.at<uint8_t>(i,j) + imV.at<uint8_t>(i,j)/4);
            }
        }*/

        Mat y_edge, b_edge, m_edge, edges;

        //adaptiveThreshold(imBlue,b_edge,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY_INV,15,8);
        //adaptiveThreshold(imYellow,y_edge,255,ADAPTIVE_THRESH_MEAN_C,THRESH_BINARY_INV,15,12);
        Canny(imYellow, y_edge, 40, 120);
        Canny(imBlue, b_edge, 80, 120);
        Canny(imMagenta, m_edge, 60, 100);
        //bitwise_or(y_edge, b_edge, edges);
        //bitwise_or(m_edge, edges, edges);
       
        /*vector<vector<Point>> b_contours, y_contours;
        vector<Vec4i> b_hierarchy, y_hierarchy;
        findContours(y_edge, y_contours, y_hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

        int idx = 0;
        for(; idx >= 0; idx = y_hierarchy[idx][0] )
        {
            Scalar color(0, 255, 255);
            drawContours( im, y_contours, idx, color );
        }
   

        findContours(b_edge, b_contours, b_hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
        idx = 0;
        for(; idx >= 0; idx = b_hierarchy[idx][0] )
        {
            Scalar color(255, 0, 0 );
            drawContours( im, b_contours, idx, color );
        }*/

        int x1,x2,y1,y2;

        vector<Vec4i> lines;
        int y_crop = 140;
        Scalar ln_colour;
        for(int i = 0; i < 3; i++){
            if(i == 0){
                HoughLinesP(b_edge(Rect(0, y_crop, iw, ih-y_crop)), lines, 2, CV_PI/180, 80, 50, 30 );
                ln_colour = Scalar(255,0,0);
            } else if(i == 1) {
                HoughLinesP(y_edge(Rect(0, y_crop, iw, ih-y_crop)), lines, 2, CV_PI/180, 80, 50, 30 );
                ln_colour = Scalar(0, 255, 255);
            } else {
                HoughLinesP(m_edge(Rect(0, y_crop, iw, ih-y_crop)), lines, 3, CV_PI/180, 40, 20, 10 );
                ln_colour = Scalar(255, 0, 255);
            }
            for( size_t i = 0; i < lines.size(); i++ )
            {
                x1 = lines[i][0];
                x2 = lines[i][2];
                y1 = lines[i][1] + y_crop;
                y2 = lines[i][3] + y_crop;
                if(x1 == x2){
                    x2 = x1 + 1;
                }
                float m = abs((float)(y2-y1)/(float)(x2-x1));
                if(m>0.1 || i == 2){
                    line(im, Point(x1,y1), Point(x2,y2), ln_colour, 3, 8 );
                }
            }
        }

		#ifdef DISP_TEST
            imshow("blue channel", imBlue);
            imshow("yellow channel", imYellow);
            //imshow("magenta channel", imMagenta);

           // imshow("blue threshold", b_edge);
        
			imshow("Camera", im);
            //imshow("Canny", edges);

			if(waitKey(30)=='p'){
                waitKey(0);
            }
		#endif

		end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        diff = end_time - start_time;
		// print out info
		cout << "Image size: " << im.size();
        cout << " Loop time: " << diff.count() << endl;
        
	}
	return 0;
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
