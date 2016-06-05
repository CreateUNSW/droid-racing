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

Navigate to 'bin' directory, type './run.exe' and hit enter.

NOTE: To run with 'servod' not yet running, the system performs a system call as root, and therefore needs to be able to do so without use of a password. Otherwise, the program should be run as the root user.
