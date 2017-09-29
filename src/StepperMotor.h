/*
 * StepperMotor.h
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#ifndef STEPPERMOTOR_H_
#define STEPPERMOTOR_H_

class StepperMotor {
public:
	StepperMotor(int limS_Pin, int limS_port, int limE_Pin, int limE_Port, int motD_Pin, int motD_Port, int motS_Pin, int motS_Port);
	virtual ~StepperMotor();

	DigitalIoPin* limStart;
	DigitalIoPin* limEnd;
	DigitalIoPin* step;
	DigitalIoPin* dir;
private:
};

#endif /* STEPPERMOTOR_H_ */
