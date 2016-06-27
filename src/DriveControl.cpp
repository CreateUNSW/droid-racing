#include "DriveControl.hpp"
#include "MyServo.hpp"

#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>

#include <wiringPi.h>

#define SPEED_SIGNAL 0
#define STEER_SIGNAL 1

#define MAX_SPEED 80
#define MAX_STEER 400

#define DEAD_ZONE_SPEED 10
#define DEAD_ZONE_STEER 10

#define INCREMENT 10
#define CONTROL_FREQ 10 // 10 Hz loop

using namespace std;

mutex emergencyLock;

DriveControl::DriveControl()
{
	// Initialise servo process (if not already)
	servo_init();

	// Zero desired control values
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
	
	// Cause while loop to finish
	running = false;
	
	// Wait for thread to complete
	t->join();

	// Delete thread safely
	delete t;
}

void DriveControl::run()
{
	int speed, angle;

	uint32_t time_ms = millis();

	// Main thread control loop, at CONTROL_FREQ Hz
	while(running){
		// increment/decrement speed value until desired speed is reached
		if(millis() - time_ms < 1000/CONTROL_FREQ){
			this_thread::sleep_for(chrono::milliseconds(200/CONTROL_FREQ));
			continue;
		} else {
			time_ms += 1000/CONTROL_FREQ;
		}

		emergencyLock.lock();

		if(desiredSpeed > currSpeed){
			speed = min(currSpeed + INCREMENT, desiredSpeed);
			set_speed(speed);
		} else if(desiredSpeed < currSpeed){
			speed = max(currSpeed - INCREMENT, desiredSpeed);
			set_speed(speed);
		}

		// increment/decrement desired steering value until it is reached
		if(desiredAngle > currAngle){
			angle = min(currAngle + INCREMENT*30, desiredAngle);
			set_steer(angle);
		} else if(desiredAngle < currAngle){
			angle = max(currAngle - INCREMENT*30, desiredAngle);
			set_steer(angle);
		}

		emergencyLock.unlock();
	}
}

void DriveControl::set_desired_speed(int speed)
{
	// set and constrain new desired speed
	desiredSpeed = min(speed, MAX_SPEED);
	desiredSpeed = max(speed, -MAX_SPEED);
	//cout << "Setting desired speed: " << speed << endl;
}

void DriveControl::set_desired_steer(int angle)
{
	// set and constrain new desired steering angle
	desiredAngle = min(angle, MAX_STEER);
	desiredAngle = max(angle, -MAX_STEER);
	//cout << "Setting desired steer: " << angle << endl;
}

void DriveControl::set_speed(int speed)
{
	int shift = 0;

	// Constrain speed range to +/- MAX_SPEED around dead zone
	if(speed > 0){
		speed = min(speed, MAX_SPEED);
		shift = DEAD_ZONE_SPEED;
	} else if (speed < 0){
		speed = max(speed, -MAX_SPEED);
		shift = -DEAD_ZONE_STEER;
	}

	// Write out speed servo signal, in reverse
	servo_write(SPEED_SIGNAL, SERVO_MID + (speed + shift));
	currSpeed = speed;
}

void DriveControl::set_steer(int steer)
{
	int shift = 0;

        // Constrain speed range to +/- MAX_SPEED around dead zone
        if(steer > 0){
                steer = min(steer, MAX_STEER);
                shift = DEAD_ZONE_STEER;
        } else if (steer < 0){
                steer = max(steer, -MAX_STEER);
                shift = -DEAD_ZONE_STEER;
        }

	// Write out steer servo signal
	servo_write(STEER_SIGNAL, SERVO_MID - (steer + shift));
	currAngle = steer;
}

void DriveControl::emergency_stop()
{
	// Make sure that thread doesn't overwrite during
	emergencyLock.lock();

	set_desired_speed(0);
	set_desired_steer(0);
	set_speed(0);
	set_steer(0);

	emergencyLock.unlock();
}


