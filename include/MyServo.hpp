#ifndef MY_SERVO_HPP_
#define MY_SERVO_HPP_

#define SERVO_MIN 900
#define SERVO_MAX 1800
#define SERVO_MID 1350

// very simple function declarations, see MyServo.cpp for detail

void servo_init();
void servo_stop();
void servo_write(int index, int val);

#endif
