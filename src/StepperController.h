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
	StepperMotor* motorX, motorY;
private:
	void Calibrate();
	void DriveMotor(int coord);
};

#endif /* STEPPERCONTROLLER_H_ */
