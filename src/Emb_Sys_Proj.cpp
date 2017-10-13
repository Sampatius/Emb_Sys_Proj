/*
 ===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
 ===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "queue.h"

#include "user_vcom.h"
#include "GCodeParser.h"
#include "DigitalIoPin.h"
#include "Motor.h"

volatile uint32_t xSteps, ySteps;
xSemaphoreHandle sbRIT;

GCodeParser* parser;
Motor *xMotor;
Motor *yMotor;

QueueHandle_t commandQueue;

int x0 = 0, x1, y0 = 0, y1, dx, dy;
double deltaError = 0.0;
double error = 0.0;
bool xDominating;
bool xDir, yDir;
bool stepTurn = true;
bool xCalibrated = false;
bool yCalibrated = false;

int xStepsTaken, yStepsTaken;

enum command {
	M10 = 1, M1, G1, G28
};

struct GObject {
	int command = 0;

	int servo = 0;

	int xCoord = 0;
	int yCoord = 0;
};

GObject gObject;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
}

extern "C" {
#if 1
void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER);	// clear IRQ flag

	if (!xCalibrated) {
		if (xSteps > 0) {
			xSteps--;
			stepTurn = !stepTurn;
			xMotor->driveISR(stepTurn);
		}
	}
	if (!yCalibrated) {
		if (ySteps > 0) {
			ySteps--;
			stepTurn = !stepTurn;
			yMotor->driveISR(stepTurn);
		}
	}

	else if (xCalibrated && yCalibrated) {

		//Drive xMotor
		if (xSteps > 0) {
			xSteps--;
			if (xMotor->getLimitStart().read()
					|| xMotor->getLimitEnd().read()) {
				xSteps = 0;
			} else {
				stepTurn = !stepTurn;
				xMotor->driveISR(stepTurn);
			}
		}

		//Drive yMotor
		if (ySteps > 0) {
			ySteps--;
			if (yMotor->getLimitStart().read()
					|| yMotor->getLimitEnd().read()) {
				ySteps = 0;
			} else {
				stepTurn = !stepTurn;
				yMotor->driveISR(stepTurn);
			}
		}
	}
	//when both steps have been driven stop
	if (xSteps <= 0 && ySteps <= 0) {
		Chip_RIT_Disable(LPC_RITIMER); // disable timer
		// Give semaphore and set context switch flag if a higher priority task was woken up
		xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
	}
	// End the ISR and (possibly) do a context switch
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}

void RIT_start(int xCount, int yCount, int us) {
	uint64_t cmp_value;
	// Determine approximate compare value based on clock rate and passed interval
	cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) us
			/ 1000000;
	// disable timer during configuration
	Chip_RIT_Disable(LPC_RITIMER);
	xSteps = xCount;
	ySteps = yCount;
	// enable automatic clear on when compare value==timer value
	// this makes interrupts trigger periodically
	Chip_RIT_EnableCompClear(LPC_RITIMER);
	// reset the counter
	Chip_RIT_SetCounter(LPC_RITIMER, 0);
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
	// start counting
	Chip_RIT_Enable(LPC_RITIMER);
	// Enable the interrupt signal in NVIC (the interrupt controller)
	NVIC_EnableIRQ(RITIMER_IRQn);
	// wait for ISR to tell that we're done
	if (xSemaphoreTake(sbRIT, portMAX_DELAY) == pdTRUE) {
		// Disable the interrupt signal in NVIC (the interrupt controller)
		NVIC_DisableIRQ(RITIMER_IRQn);
	} else {
		// unexpected error
	}
}

#endif

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

void SCTLARGE0_Init(void) {
	Chip_SCT_Init(LPC_SCTLARGE0);

	LPC_SCTLARGE0->CONFIG |= (1 << 17); // two 16-bit timers, auto limit at match 0
	LPC_SCTLARGE0->CTRL_L |= (72 - 1) << 5; // BIDIR mode, prescaler = 72, SCTimer/PWM clock = 1 MHz

	LPC_SCTLARGE0->MATCHREL[0].L = 20000 - 1; // match 0 @ 10/2MHz = 5 usec (100 kHz PWM freq)
	LPC_SCTLARGE0->MATCHREL[1].L = 1500; // match 1 used for duty cycle (in 10 steps)

	LPC_SCTLARGE0->EVENT[0].STATE = 0xFFFFFFFF; // event 0 happens in all states
	LPC_SCTLARGE0->EVENT[0].CTRL = (1 << 12); // match 0 condition only

	LPC_SCTLARGE0->EVENT[1].STATE = 0xFFFFFFFF; // event 1 happens in all states
	LPC_SCTLARGE0->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only

	LPC_SCTLARGE0->OUT[0].SET = (1 << 0); // event 0 will set SCTx_OUT0
	LPC_SCTLARGE0->OUT[0].CLR = (1 << 1); // event 1 will clear SCTx_OUT0

	LPC_SCTLARGE0->CTRL_L &= ~(1 << 2); // unhalt it by clearing bit 2 of CTRL reg
}
} // End of extern "C"

void triggerMotors(GObject object) {

	char buffer[96];
	x1 = object.xCoord;
	y1 = object.yCoord;

	dx = abs(x1 - x0) * (double) xMotor->getSteps() / (double) 340;
	dy = abs(y1 - y0) * (double) yMotor->getSteps() / (double) 310;

	xDir = (x0 < x1) ? true : false;
	yDir = (y0 < y1) ? true : false;

	xMotor->setDirection(xDir);
	yMotor->setDirection(yDir);

	xDominating = (x1 > y1) ? true : false;

	sprintf(buffer,
			"x0: %d, y0: %d, x1: %d, y1: %d, dx: %d, dy: %d, deltaError: %6.2lf\n",
			x0, y0, x1, y1, dx, dy, deltaError);
	ITM_write(buffer);

	if (xDir) {
		x0 += abs(x1 - x0);
	} else {
		x0 -= abs(x1 - x0);
	}
	if (yDir) {
		y0 += abs(y1 - y0);
	} else {
		y0 -= abs(y1 - y0);
	}

	ITM_write("--- Error values ---\n");
	if (xDominating) {
		if (dx == 0) {
			deltaError = 0.0f;
			x0 = x0;
			y0 = y0;
		} else {
			deltaError = (double) ((double) dy / (double) dx);

		}
		while (dx > 0) {
			sprintf(buffer, "Error: %6.2lf ||| DeltaError: %6.2lf\n", error,
					deltaError);
			ITM_write(buffer);
			RIT_start(2, 0, 1000000 / 12000);
			error += deltaError;
			if (error > 0.5) {
				RIT_start(0, 2, 1000000 / 12000);
				error -= 1.0;
			}
			dx--;
			vTaskDelay(10);
		}
	} else {
		if (dy == 0) {
			deltaError = 0.0f;
			x0 = 0;
			y0 = 0;
		} else {
			deltaError = (double) ((double) dx / (double) dy);
		}
		while (dy > 0) {
			sprintf(buffer, "Error: %6.2lf ||| DeltaError: %6.2lf\n", error,
					deltaError);
			ITM_write(buffer);
			RIT_start(0, 2, 1000000 / 12000);
			error += deltaError;
			if (error > 0.5) {
				RIT_start(2, 0, 1000000 / 12000);
				error -= 1.0;
			}
			dy--;
			vTaskDelay(10);
		}
	}
	error = 0;
}

/* TASKS */

static void vCalibrate(void *pvParameters) {
	LPC_SCTLARGE0->MATCHREL[1].L = 1100;

	xMotor->setDirection(true);
	yMotor->setDirection(true);

	while(!xCalibrated || !yCalibrated) {
		if (!xCalibrated) {
			xStepsTaken++;
			RIT_start(2, 0, 1000000 / 15000);
			if (xMotor->getLimitStart().read()) {
				xMotor->setDirection(false);
				xStepsTaken = 0;
			} else if (xMotor->getLimitEnd().read()) {
				for (int i = 0; i < (xStepsTaken * 0.02); i++) {
					xMotor->setDirection(true);
					RIT_start(2, 0, 1000000 / 15000);
				}
				xStepsTaken = xStepsTaken * 0.98;
				xCalibrated = true;
			}
		}
		if (!yCalibrated) {
			yStepsTaken++;
			RIT_start(0, 2, 1000000 / 15000);
			if (yMotor->getLimitStart().read()) {
				yMotor->setDirection(false);
				yStepsTaken = 0;
			} else if (yMotor->getLimitEnd().read()) {
				for (int i = 0; i < (yStepsTaken * 0.02); i++) {
					yMotor->setDirection(true);
					RIT_start(0, 2, 1000000 / 15000);
				}
				yStepsTaken = yStepsTaken * 0.98;
				yCalibrated = true;
			}
		}
	}

	vTaskDelete(NULL);
}

static void vParserTask(void *pvParameters) {
	//Wait for USB serial to initialize
	vTaskDelay(10);

	char strFromUSB[96];
	char buffStr[96];
	bool loopRead = true;
	uint32_t len;

	while (1) {
		do {
			len = USB_receive((uint8_t *) strFromUSB, 96);
			strFromUSB[len] = 0;
			ITM_write(strFromUSB);
			ITM_write("\n");
			for (uint32_t i = 0; i < len; ++i) {
				if (strFromUSB[i] == '\n') {
					buffStr[i] = 0;
					loopRead = false;
					break;
				} else {
					buffStr[i] = strFromUSB[i];
				}
			}
		} while (loopRead);

		loopRead = true;

		switch (parser->read(buffStr)) {
		case M10:
			gObject.command = M10;
			xQueueSendToBack(commandQueue, &gObject, portMAX_DELAY);
			break;
		case M1:
			gObject.command = M1;
			gObject.servo = parser->getAngle();
			xQueueSendToBack(commandQueue, &gObject, portMAX_DELAY);
			break;
		case G1:
			gObject.command = G1;
			gObject.xCoord = parser->getXCoord();
			gObject.yCoord = parser->getYCoord();
			xQueueSendToBack(commandQueue, &gObject, portMAX_DELAY);
			break;
		case G28:
			gObject.command = G28;
			xQueueSendToBack(commandQueue, &gObject, portMAX_DELAY);
			break;
		default:
			break;
		}
	}
}

static void vStepperTask(void *pvParameters) {
	vTaskDelay(10);
	char buffer[96];
	while (1) {
		if (xQueueReceive(commandQueue, &gObject,
				configTICK_RATE_HZ * 10) == pdFALSE) {
			USB_send((uint8_t *) "OK\n", 4);
		} else {
			switch (gObject.command) {
			case M10:
				USB_send(
						(uint8_t *) "M10 XY 340 310 0.00 0.00 A0 B0 H0 S80 U160 D90\n",
						48);
				USB_send((uint8_t *) "OK\n", 4);
				break;
			case M1:
				sprintf(buffer, "gObjectAngle: %d\n", gObject.servo);
				ITM_write(buffer);
				if (gObject.servo == 160) { // oli 90
					LPC_SCTLARGE0->MATCHREL[1].L = 1100;
				} else {
					LPC_SCTLARGE0->MATCHREL[1].L = 1600;
				}
				USB_send((uint8_t *) "OK\n", 4);
				break;
			case G1:
				sprintf(buffer, "gObjectX: %d, gObjectY: %d\n", gObject.xCoord,
						gObject.yCoord);
				ITM_write(buffer);
				triggerMotors(gObject);
				USB_send((uint8_t *) "OK\n", 4);
				break;
			case G28:
				USB_send((uint8_t *) "OK\n", 4);
				break;
			}
			vTaskDelay(1);
		}
	}
}

/* END OF TASKS */

int main(void) {

	prvSetupHardware();
	sbRIT = xSemaphoreCreateBinary();

	ITM_init();

	parser = new GCodeParser();

#if 0
	//xMotor DigitalIoPins

	DigitalIoPin xStep(0, 24, DigitalIoPin::output, true);
	DigitalIoPin xDir(1, 0, DigitalIoPin::output, true);
	DigitalIoPin xLimitStart(0, 27, DigitalIoPin::pullup, true);
	DigitalIoPin xLimitEnd(0, 28, DigitalIoPin::pullup, true);

	//yMotor DigitalIoPins

	DigitalIoPin yStep(0, 9, DigitalIoPin::output, true);
	DigitalIoPin yDir(0, 29, DigitalIoPin::output, true);
	DigitalIoPin yLimitStart(1, 9, DigitalIoPin::pullup, true);
	DigitalIoPin yLimitEnd(1, 10, DigitalIoPin::pullup, true);
#endif

	//Plotter X DigitalIoPins
	DigitalIoPin xStep(0, 27, DigitalIoPin::output, true);
	DigitalIoPin xDir(0, 28, DigitalIoPin::output, true);
	DigitalIoPin xLimitStart(0, 0, DigitalIoPin::pullup, true);
	DigitalIoPin xLimitEnd(1, 3, DigitalIoPin::pullup, true);

	//Plotter Y DigitalIoPins
	DigitalIoPin yStep(0, 24, DigitalIoPin::output, true);
	DigitalIoPin yDir(1, 0, DigitalIoPin::output, true);
	DigitalIoPin yLimitStart(0, 9, DigitalIoPin::pullup, true);
	DigitalIoPin yLimitEnd(0, 29, DigitalIoPin::pullup, true);

	//X and Y Motors

	xMotor = new Motor(xDir, xStep, xLimitStart, xLimitEnd);
	yMotor = new Motor(yDir, yStep, yLimitStart, yLimitEnd);

	//Queue for commands

	commandQueue = xQueueCreate(1, sizeof(GObject));

	Chip_RIT_Init(LPC_RITIMER);

	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, 0, 10);
	SCTLARGE0_Init();

	NVIC_SetPriority(RITIMER_IRQn,
			configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);

#if 1
	xTaskCreate(vCalibrateX, "vCalibrate", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 3UL), (TaskHandle_t *) NULL);
#endif

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(vStepperTask, "vStepperTask", configMINIMAL_STACK_SIZE * 5,
			NULL, (tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
