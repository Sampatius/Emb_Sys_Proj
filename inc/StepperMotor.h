/*
 * StepperMotor.h
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#ifndef STEPPERMOTOR_H_
#define STEPPERMOTOR_H_

#include "DigitalIoPin.h"

class StepperMotor {
public:
	StepperMotor(int limS_Pin, int limS_Port, int limE_Pin, int limE_Port, int motD_Pin, int motD_Port, int motS_Pin, int motS_Port);
	virtual ~StepperMotor();

	inline DigitalIoPin getDir() { return dir; }
	inline DigitalIoPin getStep() { return step; }
	inline DigitalIoPin getLimStart() { return limStart; }
	inline DigitalIoPin getLimEnd() { return limEnd; }

private:
	DigitalIoPin dir;
	DigitalIoPin step;
	DigitalIoPin limStart;
	DigitalIoPin limEnd;
};

#endif /* STEPPERMOTOR_H_ */
