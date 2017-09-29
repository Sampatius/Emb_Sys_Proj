/*
 * StepperController.cpp
 *
 *  Created on: 28.9.2017
 *      Author: Dan
 */

#include "StepperController.h"
#include "StepperMotor.h"

StepperController::StepperController() {
	//These guys take lim sw and motor dir and step pins and ports as params
	motorX = new StepperMotor(0,0,0,0,0,0,0,0);
	motorY = new StepperMotor(0,0,0,0,0,0,0,0);
}

StepperController::~StepperController() {
	// TODO Auto-generated destructor stub
}

//This might be full of shit, feel free to modify
void StepperController::Calibrate(StepperMotor motor) {
	bool direction = false, firstHit = false, secondHit = false, calibrated = false;

	int steps = 0, avgSteps = 0;
	//Before starting counting steps hit either switch
	while (!firstHit && !calibrated) {
		dir->write(direction);
		step->write(state);
		state = !state;
		vTaskDelay(1);

		if (limSw1->read() || limSw2->read()) {
			direction = !direction;
			dir->write(direction);
			firstHit = true;
			break;
		}
	}
	//After first switch is hit start counting steps, when second switch is hit: calibrated
	while (firstHit && !calibrated) {
		dir->write(direction);
		step->write(state);
		state = !state;
		steps++;
		vTaskDelay(1);

		if (!(limSw1->read() || limSw2->read())) {
			secondHit = true;
		}

		if (secondHit) {
			if ((limSw1->read() || limSw2->read())) {
				direction = !direction;
				dir->write(direction);

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
				dir->write(direction);
				step->write(state);
				state = !state;
				vTaskDelay(1);
				avgSteps -= 2;
			}
			secondHit = false;
			//Drive to center
			dir->write(direction);
			//RIT_start(avgSteps / 2, 1000000 / 3000);
			for (int i = 0; i < avgSteps / 2; ++i) {
				dir->write(direction);
				step->write(state);
				state = !state;
				vTaskDelay(1);
			}
		}
	}
}

void StepperController::DriveMotor(StepperMotor motor, int coord) {

}
