/*
 * StepperMotor.cpp
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#include "StepperMotor.h"

StepperMotor::StepperMotor(int limS_Pin, int limS_Port, int limE_Pin, int limE_Port, int motD_Pin, int motD_Port, int motS_Pin, int motS_Port) :
	dir(motD_Pin, motD_Port, DigitalIoPin::output, false), step(motS_Pin, motS_Port, DigitalIoPin::output, false),
	limStart(limS_Pin, limS_Port, DigitalIoPin::pullup, true), limEnd(limE_Pin, limE_Port, DigitalIoPin::pullup, true){

}

StepperMotor::~StepperMotor() {
	// TODO Auto-generated destructor stub
}

