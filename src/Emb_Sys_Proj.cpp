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
bool xDir, yDir;
void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER);	// clear IRQ flag

	if (xSteps > 0) {
		xSteps--;
		xMotor->drive(xDir);

		/* maybe implement rit stopper if limit switches are hit
		 if (limSw1->read() || limSw2->read()) {
		 RIT_count = 0;
		 }*/
	}

	if (ySteps > 0) {
		ySteps--;
		yMotor->drive(yDir);

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
#endif

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
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

/* TASKS */

static void vParserTask(void *pvParameters) {
	//Wait for USB serial to initialize
	vTaskDelay(10);
	char buffer[64];
	while (1) {
		if (parser->read()) {
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

	xMotor->calibrate();
	while (1) {
		xQueueReceive(commandQueue, &o, portMAX_DELAY);
		sprintf(buffer, "vS X: %02f Y: %0.2f", o.xCoord, o.yCoord);
		ITM_write(buffer);
		double lastX, lastY;

		coordObject o;
		char buffer[40];
		//xMotor->calibrate();

		BaseType_t xStatus;

		while (1) {
			xStatus = xQueueReceive(commandQueue, &o, portMAX_DELAY);
			if (xStatus == pdTRUE) {
				RIT_start(o.xCoord, o.yCoord, 1000000 / 2000);
				sprintf(buffer, "vStepper - X: %02f Y: %0.2f", o.xCoord,
						o.yCoord);
			}
			vTaskDelay(10);
		}
	}
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

	xMotor = new Motor(xDir, xStep, xLimitStart, xLimitEnd);

	commandQueue = xQueueCreate(10, sizeof(coordObject));

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 8, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vStepperTask, "vStepperTask", configMINIMAL_STACK_SIZE * 5,
			NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
