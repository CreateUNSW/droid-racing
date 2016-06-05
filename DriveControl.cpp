#include "DriveControl.hpp"
#include <algorithm>

using namespace std;

DriveControl::DriveControl()
{
	// Initialise servo process (if not already)
	servo_init();

	// Initialise servo signals
	set_speed(0);
	set_steer(0);
}

void DriveControl::set_speed(int speed)
{
	// Constrain speed range to +/- MAX_SPEED
	speed = min(speed, MAX_SPEED);
	speed = max(speed, -MAX_SPEED);

	// Write out speed servo signal
	servo_write(SPEED_SIGNAL, 1500 + speed);
	currSpeed = speed;
}

void DriveControl::set_steer(int steer)
{
	// Constrain steer range to +/- MAX_STEER
	steer = min(steer, MAX_STEER);
	steer = max(steer, -MAX_STEER);

	// Write out steer servo signal
	servo_write(STEER_SIGNAL, 1500 + steer);
	currAngle = steer;
}
