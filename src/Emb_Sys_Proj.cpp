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

struct coordObject {
	double xCoord = 0, yCoord = 0, oldX = 0, oldY = 0;
	coordObject();
	coordObject(double xCoord_, double yCoord_);
};

coordObject::coordObject() {

}

coordObject::coordObject(double xCoord_, double yCoord_) {
	xCoord = xCoord_;
	yCoord = yCoord_;
	oldX = xCoord;
	oldY = yCoord;
}

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

	//Drive xMotor
	if (xSteps > 0) {
		xSteps--;
		xMotor->driveISR();

		//Stop if limit read
		if (xMotor->getLimitStart().read() || xMotor->getLimitEnd().read()) {
			xSteps = 0;
		}
	}

	//Drive yMotor
	if (ySteps > 0) {
		ySteps--;
		yMotor->driveISR();

		//Stop if limit read
		if (yMotor->getLimitStart().read() || yMotor->getLimitEnd().read()) {
			ySteps = 0;
		}

	}
	//when both steps have been driven stop
	else if (xSteps <= 0) {
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

/* TASKS */

static void vCalibrateX(void *pvParameters) {
	xMotor->calibrate();
	vTaskDelete(NULL);
}

static void vCalibrateY(void *pvParameters) {
	yMotor->calibrate();
	vTaskDelete(NULL);
}

static void vParserTask(void *pvParameters) {
	//Wait for USB serial to initialize
	vTaskDelay(10);
	char buffer[64];
	bool penState = true;

	while (1) {
		if (parser->read() == 1) {
			if (penState) {
				penState = !penState;
				LPC_SCTLARGE0->MATCHREL[1].L = 1000;
			} else {
				LPC_SCTLARGE0->MATCHREL[1].L = 1500;
			}
		} else if (parser->read() == 2) {
			double xCoord = parser->getXCoord();
			double yCoord = parser->getYCoord();
			coordObject o(xCoord, yCoord);
			xQueueSendToBack(commandQueue, &o, portMAX_DELAY);
			sprintf(buffer, "vP X: %0.2f Y: %0.2f\n", xCoord, yCoord);
			ITM_write(buffer);
		} else {
			ITM_write("No coords.\n");
		}
	}
}

static void vStepperTask(void *pvParameters) {
	coordObject o;
	char buffer[64];

	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	int deltaX, deltaY;

	bool xDominating;

	while (1) {
		xQueueReceive(commandQueue, &o, portMAX_DELAY);
		sprintf(buffer, "vS X: %02f Y: %0.2f", o.xCoord, o.yCoord);
		ITM_write(buffer);

		x0 = o.xCoord;
		y0 = o.yCoord;

		if (x0 > y0) {
			xDominating = true;
		} else {
			xDominating = false;
		}

		deltaX = x1 - x0;
		deltaY = y1 - y0;

		if (deltaX < 0) {
			xMotor->setDirection(false);
		} else {
			xMotor->setDirection(true);
		}

		if (deltaY < 0) {
			yMotor->setDirection(true);
		} else {
			yMotor->setDirection(false);
		}

		if (xDominating) {
			while (abs(deltaX) > 0) {
				if (deltaX % deltaY == 0) {
					RIT_start(0, 2, 100000 / 2000);
					deltaX--;
				} else {
					RIT_start(2, 0, 1000000 / 2000);
					deltaX--;
				}
			}
		} else {
			while (abs(deltaY) > 0) {
				if (deltaY % deltaX == 0) {
					RIT_start(2, 0, 1000000 / 2000);
					deltaY--;
				} else {
					RIT_start(0, 2, 1000000 / 2000);
					deltaY--;
				}
			}
		}
	}
	vTaskDelay(10);
}

/* END OF TASKS */

int main(void) {

	prvSetupHardware();
	sbRIT = xSemaphoreCreateBinary();

	ITM_init();

	parser = new GCodeParser();

	DigitalIoPin xStep(0, 24, DigitalIoPin::output, true);
	DigitalIoPin xDir(1, 0, DigitalIoPin::output, true);
	DigitalIoPin xLimitStart(0, 27, DigitalIoPin::pullup, true);
	DigitalIoPin xLimitEnd(0, 28, DigitalIoPin::pullup, true);

	DigitalIoPin yStep(0, 9, DigitalIoPin::output, true);
	DigitalIoPin yDir(0, 29, DigitalIoPin::output, true);
	DigitalIoPin yLimitStart(1, 9, DigitalIoPin::pullup, true);
	DigitalIoPin yLimitEnd(1, 10, DigitalIoPin::pullup, true);

	xMotor = new Motor(xDir, xStep, xLimitStart, xLimitEnd);
	yMotor = new Motor(yDir, yStep, yLimitStart, yLimitEnd);

	commandQueue = xQueueCreate(10, sizeof(coordObject));

	Chip_RIT_Init(LPC_RITIMER);

	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, 0, 10);
	SCTLARGE0_Init();

	NVIC_SetPriority(RITIMER_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);

#if 0
	xTaskCreate(vCalibrateX, "vCalibrateX", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(vCalibrateY, "vCalibrateY", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);
#endif

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(vStepperTask, "vStepperTask", configMINIMAL_STACK_SIZE * 5,
			NULL, (tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
