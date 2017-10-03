/*
 * Motor.h
 *
 *  Created on: Oct 2, 2017
 *      Author: Sami
 */

#ifndef MOTOR_H_
#define MOTOR_H_

#include "DigitalIoPin.h"
#include "FreeRTOS.h"
#include "task.h"

class Motor {
public:
	//Constructor and destructor
	Motor(DigitalIoPin dir_, DigitalIoPin step_, DigitalIoPin limitStart_, DigitalIoPin limitEnd_);
	virtual ~Motor();

	//Inline getter functions
	inline DigitalIoPin getDir() { return dir; }
	inline DigitalIoPin getStep() { return step; }
	inline DigitalIoPin getLimitStart() { return limitStart; }
	inline DigitalIoPin getLimitEnd() { return limitEnd; }

	//Class functions
	void drive(bool direction);
	void calibrate();

private:
	DigitalIoPin dir, step, limitStart, limitEnd;
	int steps;
	bool calibrated = false;
	bool direction = true;
};

#endif /* MOTOR_H_ */
