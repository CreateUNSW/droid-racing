#ifndef DRIVE_CONTROL_HPP_
#define DRIVE_CONTROL_HPP_

#include "MyServo.hpp"

#define SPEED_SIGNAL 0
#define STEER_SIGNAL 1

#define MAX_SPEED 250
#define MAX_STEER 250

class DriveControl {
public:
	// Constructor / initialiser
	DriveControl();

	// Signal write functions
	void set_speed(int speed);
	void set_steer(int angle);

private:
	// Variables for use in a controller for later
	int currSpeed;
	int currAngle;
	
};

#endif
