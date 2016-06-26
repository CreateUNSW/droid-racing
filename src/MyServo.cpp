#include <cstdlib>
#include <iostream>
#include <sstream>
#include <csignal>
#include <cassert>

#include "MyServo.hpp"

#define DEBUG_SERVO

using namespace std;

bool servoInitialised = false;

/*
*	Signal handler only in the case where servo process is started.
*/
void signalHandler( int signum )
{
	cout << "Received signal " << signum << endl;
	if (signum == SIGINT){
		cout << endl << "Caught SIGINT" << endl;
		exit(1);
	}
}

/*
*	Initialises servo process if it isn't running.
*	Schedules it to be killed on process end if it is started here.
*/
void servo_init()
{
	int i = system("pgrep servod > /dev/null");
	
	if ( i > 0 ){
		cout << "Initialising servo process" << endl;
		system("sudo ./bin/servod --min=900us --max=1800us --cycle-time=14200us --step-size=5us");

		// Trap exit behaviour
		signal(SIGINT, signalHandler);
		atexit(servo_stop);
	} else {
		cout << "Servos already inititalised" << endl;
	}

	servoInitialised = true;
}

/*
*	Kills servo process
*/
void servo_stop()
{
	// Neutral/off position for both servo signals
	servo_write(0, SERVO_MID);
	servo_write(1, SERVO_MID);

	cout << "Stopping servo process" << endl;
	system("sudo kill $(pgrep servod)");

	servoInitialised = false;
}

/*
*	Write to a servo: needs "servod" running
*	Inputs: servo number, pulse duration in microseconds
*/
void servo_write(int index, int val)
{
	// Check for servos initialised
	assert(servoInitialised);

	// Check for bad input
	assert(val >= SERVO_MIN && val <= SERVO_MAX);

	// Round to the nearest 10us, for './servod'
	if (val % 5 > 0) {
		if (val % 5 >= 3){
			val += 5 - (val % 5);
		} else {
			val -= (val % 5);
		}
	}

	// Compose system command string
	stringstream ss;
	ss << "echo " << index << '=' << val << "us > /dev/servoblaster";
	string command = ss.str();

	// Convert to char pointer and send
	char * cmdPtr = &command[0];
	system(cmdPtr);

	// Debug output
	#ifdef DEBUG_SERVO
	cout << "Writing: " << cmdPtr << endl;
	#endif

}
