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

GCodeParser* parser;
Motor *xMotor;
Motor *yMotor;

QueueHandle_t commandQueue;

struct coordObject {
	double xCoord, yCoord;
	coordObject(double xCoord_, double yCoord_);
};

coordObject::coordObject(double xCoord_, double yCoord_) {
	xCoord = xCoord_;
	yCoord = yCoord_;
}

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
}

extern "C" {
#if 0
void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER);// clear IRQ flag

	if (RIT_count > 0) {
		RIT_count--;
		// do something useful here...
		state = !state;
		step->write(state);

		if (limSw1->read() || limSw2->read()) {
			RIT_count = 0;
		}
	} else {
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

/* TASKS */

static void vParserTask(void *pvParameters) {
	char buffer[40];
	while (1) {
		parser->read();
		coordObject o(parser->getXCoord(), parser->getYCoord());
		xQueueSendToBack(commandQueue, &o, portMAX_DELAY);
		sprintf(buffer, "vParser - X: %0.2f Y: %0.2f\n", parser->getXCoord(),
				parser->getYCoord());
		ITM_write(buffer);
	}
}

static void vStepperTask(void *pvParameters) {
	coordObject *o;
	char buffer[40];
	xMotor->calibrate();
	while (1) {
		xQueueReceive(commandQueue, &o, portMAX_DELAY);
		sprintf(buffer, "vStepper - X: %02f Y: %0.2f", o->xCoord, o->yCoord);
		vTaskDelay(10);
	}
}

/* END OF TASKS */

int main(void) {

	prvSetupHardware();

	ITM_init();

	parser = new GCodeParser();

	DigitalIoPin xStep(0, 24, DigitalIoPin::output, true);
	DigitalIoPin xDir(1, 0, DigitalIoPin::output, true);
	DigitalIoPin xLimitStart(0, 27, DigitalIoPin::pullup, true);
	DigitalIoPin xLimitEnd(0, 28, DigitalIoPin::pullup, true);

	xMotor = new Motor(xDir, xStep, xLimitStart, xLimitEnd);

	commandQueue = xQueueCreate(10, sizeof(coordObject));

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vStepperTask, "vStepperTask", configMINIMAL_STACK_SIZE * 5,
			NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
