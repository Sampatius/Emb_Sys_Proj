/*
 * Motor.cpp
 *
 *  Created on: Oct 2, 2017
 *      Author: Sami
 */

#include "Motor.h"

Motor::Motor(DigitalIoPin dir_, DigitalIoPin step_, DigitalIoPin limitStart_, DigitalIoPin limitEnd_) :
		dir(dir_), step(step_), limitStart(limitStart_), limitEnd(limitEnd_), steps(0) {

}

Motor::~Motor() {
	// TODO Auto-generated destructor stub
}

void Motor::ride(bool direction) {
	dir.write(direction);
	step.write(true);
	step.write(false);
}

void Motor::calibrate() {
	while(!calibrated) {
		if(limitStart.read()) {
			direction = !direction;
			steps = 0;
		}
		else if(limitEnd.read()) {
			direction = !direction;
			while (limitEnd.read()) {
				ride(direction);
			}
			steps = steps * 0.94;
			calibrated = true;
		}
		steps++;
		ride(direction);
		vTaskDelay(5);
	}
}
