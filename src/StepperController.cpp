/*
 * StepperController.cpp
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#include "StepperController.h"

StepperController::StepperController() : motorX(0,27,0,28,1,0,0,24), motorY(0,0,0,0,0,0,0,0){
	//These guys take lim sw and motor dir and step pins and ports as params
}

StepperController::~StepperController() {
	// TODO Auto-generated destructor stub
}

//This might be full of shit, feel free to modify
void StepperController::Calibrate() {
	bool direction = false, firstHit = false, secondHit = false, calibrated = false, state = true;

	int steps = 0, avgSteps = 0;
	//Before starting counting steps hit either switch
	while (!firstHit && !calibrated) {
		motorX.getDir().write(direction);
		motorX.getDir().write(direction);
		motorX.getStep().write(state);
		state = !state;
		vTaskDelay(1);

		if (motorX.getLimStart().read() || motorX.getLimEnd().read()) {
			direction = !direction;
			motorX.getDir().write(direction);
			firstHit = true;
			break;
		}
	}
	//After first switch is hit start counting steps, when second switch is hit: calibrated
	while (firstHit && !calibrated) {
		motorX.getDir().write(direction);
		motorX.getStep().write(state);
		state = !state;
		steps++;
		vTaskDelay(1);

		if (!(motorX.getLimStart().read() || motorX.getLimEnd().read())) {
			secondHit = true;
		}

		if (secondHit) {
			if ((motorX.getLimStart().read() || motorX.getLimEnd().read())) {
				direction = !direction;
				motorX.getDir().write(direction);

				calibrated = true;
				avgSteps = steps;
				steps = 0;
			}
		}
	}

	while (calibrated) {
		if (secondHit) {
			//REVERSE
			for (int i = 0; i < avgSteps * 0.02; ++i) {
				motorX.getDir().write(direction);
				motorX.getStep().write(state);
				state = !state;
				vTaskDelay(1);
				avgSteps -= 2;
			}
			secondHit = false;
			//Drive to center
			motorX.getDir().write(direction);
			//RIT_start(avgSteps / 2, 1000000 / 3000);
			for (int i = 0; i < avgSteps / 2; ++i) {
				motorX.getDir().write(direction);
				motorX.getStep().write(state);
				state = !state;
				vTaskDelay(1);
			}
		}
	}
}

void StepperController::DriveMotor(StepperMotor motor, int coord) {

}
