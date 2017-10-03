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

void Motor::drive(bool direction) {
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
<<<<<<< HEAD
			for(int i = 0; i < (steps * 0.02); i++) {
				drive(direction);
				vTaskDelay(5);
=======
			while (limitEnd.read()) {
				drive(direction);
>>>>>>> Interrupt
			}
			steps = steps * 0.97;
			calibrated = true;
		}
		steps++;
		drive(direction);
		vTaskDelay(5);
	}
}
