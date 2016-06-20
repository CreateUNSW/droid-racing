#include "DriveControl.hpp"
#include "MyServo.hpp"

#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>

#define SPEED_SIGNAL 0
#define STEER_SIGNAL 1

#define MAX_SPEED 250
#define MAX_STEER 250

#define DEAD_ZONE 100

#define INCREMENT 20

using namespace std;

DriveControl::DriveControl()
{
	// Initialise servo process (if not already)
	servo_init();

	desiredSpeed = 0;
	desiredAngle = 0;

	// Initialise servo signals
	set_speed(0);
	set_steer(0);

	// Begin thread
	running = true;
	t = new thread(&DriveControl::run, this);
}

DriveControl::~DriveControl()
{
	cout << "DriveControl destructor called" << endl;
	running = false;
	t->join();
	delete t;
}

void DriveControl::run()
{
	int speed, angle;
	while(running){
		if(desiredSpeed > currSpeed){
			speed = min(currSpeed + INCREMENT, desiredSpeed);
			set_speed(speed);
		} else if(desiredSpeed < currSpeed){
			speed = max(currSpeed - INCREMENT, desiredSpeed);
			set_speed(speed);
		}

		if(desiredAngle > currAngle){
			angle = min(currAngle + INCREMENT, desiredAngle);
			set_steer(angle);
		} else if(desiredAngle < currAngle){
			angle = max(currAngle - INCREMENT, desiredAngle);
			set_steer(angle);
		}
		
		this_thread::sleep_for(chrono::milliseconds(100));	
	}
}

void DriveControl::set_desired_speed(int speed)
{
	desiredSpeed = min(speed, MAX_SPEED);
	desiredSpeed = max(speed, -MAX_SPEED);
	cout << "Setting desired speed: " << speed << endl;
}

void DriveControl::set_desired_steer(int angle)
{
	desiredAngle = min(angle, MAX_STEER);
	desiredAngle = max(angle, -MAX_STEER);
	cout << "Setting desired steer: " << angle << endl;
}

void DriveControl::set_speed(int speed)
{
	int shift = 0;

	// Constrain speed range to +/- MAX_SPEED around dead zone
	if(speed > 0){
		speed = min(speed, MAX_SPEED);
		shift = DEAD_ZONE;
	} else if (speed < 0){
		speed = max(speed, -MAX_SPEED);
		shift = -DEAD_ZONE;
	}

	// Write out speed servo signal, in reverse
	servo_write(SPEED_SIGNAL, 1500 - (speed + shift));
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
