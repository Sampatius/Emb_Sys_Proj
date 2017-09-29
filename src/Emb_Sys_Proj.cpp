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
void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER); // clear IRQ flag

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

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
}

/* TASKS */

//dunno lel
int coordX = 0, coordY = 0;

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
