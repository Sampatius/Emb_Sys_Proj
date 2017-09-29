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
#include "queue"

#include "user_vcom.h"
#include "DigitalIoPin.h"
#include "GCodeParser.h"

GCodeParser* parser;
StepperController* stepContr;

QueueHandle_t commandQueue;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
}

extern "C" {

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
}

/* TASKS */

static void vParserTask(void *pvParameters) {
	vTaskDelay(10);
	while(1) {
		parser->read();

	}
}

static void vStepperTask(void *pvParameters) {

	stepContr = new StepperController();
	stepContr->Calibrate(stepContr->motorX);
	stepContr->Calibrate(stepContr->motorY);

	while(1) {

		vTaskDelay(10);
	}
}

/* END OF TASKS */

int main(void) {

	prvSetupHardware();

	ITM_init();

	parser = new GCodeParser();

	//Create stepper controller object which will crete 2 motor objects in its constructor
	stepContr = new StepperController();

	commandQueue = xQueueCreate(10, sizeof(int));

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vParserTask, "vStepperTask", configMINIMAL_STACK_SIZE * 5, NULL,
				(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
