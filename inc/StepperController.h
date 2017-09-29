/*
 * StepperController.h
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#ifndef STEPPERCONTROLLER_H_
#define STEPPERCONTROLLER_H_

class StepperController {
public:
	StepperController();
	virtual ~StepperController();
	void DriveMotor(StepperMotor motor, int coord);
	void Calibrate();

	StepperMotor* motorX, motorY;
private:
};

#endif /* STEPPERCONTROLLER_H_ */
