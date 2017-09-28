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

// TODO: insert other include files here

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "user_vcom.h"
#include "DigitalIoPin.h"
#include "GCodeParser.h"

// TODO: insert other definitions and declarations here

GCodeParser* parser;

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

static void vParserTask(void *pvParameters) {
	while(1) {
		parser->read();
		vTaskDelay(10);
	}
}

int main(void) {

	prvSetupHardware();

	parser = new GCodeParser();

	xTaskCreate(vParserTask, "vParserTask", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(cdc_task, "CDC", configMINIMAL_STACK_SIZE * 5, NULL,
			(tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();
}
