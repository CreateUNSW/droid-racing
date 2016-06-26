# droid-racing
CREATE's repository for our droid racing team

Compiled using OpenCV 3.1 on a Raspberry Pi 3 Model B V1.2 using a Raspberry Pi camera module.

Instructions for how to compile OpenCV on the Raspberry Pi can be found here http://www.pyimagesearch.com/2015/10/26/how-to-install-opencv-3-on-raspbian-jessie/

Download raspicam OpenCV API with instructions from here https://sourceforge.net/projects/raspicam/files/

Follow instructions to get a raspberry pi servo controller from here http://cihatkeser.com/servo-control-with-raspberry-pi-in-5-minutes-or-less/ for source.
Check './servod --help' (compiled from source) for usage.


Build Instructions:

Navigate to main directory with Makefile. Type 'make' into terminal and hit enter.


Run Instructions:

Comment/uncomment relevant hash defines at the top of main.cpp.

	#define USB_WEBCAM - use if you are using the Pi with a USB webcam instead of a pi cam
	#define STILL_IMAGES - use if you want to play back the stored images one by one for processing rather than video
		- note: pay attention to the file format, as defined on line 108 of main.cpp, if you wish to add more images

Navigate to directory with main.cpp. Type 'sudo ./bin/run.exe' and hit enter to run. This is needed for the image read/write file path to work.

NOTE: To run with 'servod' not yet running, the system performs a system call as root, and therefore needs to be able to do so without use of a password. Otherwise, the program should be run as the root user.
