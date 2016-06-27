#ifndef DRIVE_CONTROL_HPP_
#define DRIVE_CONTROL_HPP_

#include <thread>

using namespace std;

class DriveControl {
public:
	// Constructor / initialiser
	DriveControl();
	~DriveControl();

	// Threaded loop
	void run();

	// Controller functions
	void set_desired_speed(int speed);
	void set_desired_steer(int angle);

	// Emergency stop function
	void emergency_stop();

private:
	// Signal write functions
	void set_speed(int speed);
	void set_steer(int angle);

	// Variables for use in a controller for later
	int desiredSpeed;
	int desiredAngle;
	int currSpeed;
	int currAngle;

	thread *t;
	bool running;
};

#endif
