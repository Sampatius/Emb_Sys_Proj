/*
 * StepperMotor.cpp
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#include "StepperMotor.h"
#include "DigitalIoPin.h"

StepperMotor::StepperMotor(int limS_Pin, int limS_Port, int limE_Pin, int limE_Port, int motD_Pin, int motD_Port, int motS_Pin, int motS_Port) {
	// TODO Auto-generated constructor stub
	limStart = new DigitalIoPin(limS_Pin, limS_Port, DigitalIoPin::pullup, true);
	limEnd = new DigitalIoPin(limE_Pin, limE_Port, DigitalIoPin::pullup, true);

	dir = new DigitalIoPin(motD_Pin, motD_Port, DigitalIoPin::output, false);
	step = new DigitalIoPin(motS_Pin, motS_Port, DigitalIoPin::output, false);

}

StepperMotor::~StepperMotor() {
	// TODO Auto-generated destructor stub
}

