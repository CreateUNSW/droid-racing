#include <cstdlib>
#include <iostream>
#include <sstream>
#include <csignal>

#include "MyServo.hpp"

using namespace std;

/*
*	Signal handler only in the case where servo process is started.
*/
void signalHandler( int signum )
{
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
		// TODO: write constructor/configuration options for executable
		system("sudo ./servod");

		// Trap exit behaviour
		signal(SIGINT, signalHandler);
		atexit(servo_stop);
	} else {
		cout << "Servos already inititalised" << endl;
	}
}

/*
*	Kills servo process
*/
void servo_stop()
{
	cout << "Stopping servo process" << endl;
	system("sudo kill $(pgrep servod)");
}

/*
*	Write to a servo: needs "servod" running
*	Inputs: servo number, pulse duration
*/
void servo_write(int index, int val)
{
	// TODO: write index and val checks for bad input
	stringstream ss;
	ss << "echo " << index << '=' << val << " > /dev/servoblaster";
	string command = ss.str();
	char * cmdPtr = &command[0];
	system(cmdPtr);
}
