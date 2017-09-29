/*
 * StepperController.h
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#ifndef STEPPERCONTROLLER_H_
#define STEPPERCONTROLLER_H_

#include "StepperMotor.h"
#include "FreeRTOS.h"
#include "task.h"

class StepperController {
public:
	StepperController();
	virtual ~StepperController();
	void DriveMotor(StepperMotor motor, int coord);
	void Calibrate();

private:
	StepperMotor motorX, motorY;
};

#endif /* STEPPERCONTROLLER_H_ */
