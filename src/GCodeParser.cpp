/*
 * GCodeParser.cpp
 *
 *  Created on: Sep 28, 2017
 *      Author: Sami
 */

#include "GCodeParser.h"

GCodeParser::GCodeParser() {
	// TODO Auto-generated constructor stub

}

GCodeParser::~GCodeParser() {
	// TODO Auto-generated destructor stub
}

void GCodeParser::read() {
	char str[96];
	char buffStr[96];
	uint32_t len;

	len = USB_receive((uint8_t *) str, 96);
	if (len > 0) {
		for (uint32_t i = 0; i < len; ++i) {
			buffStr[i] = str[i];
		}
		buffStr[len] = 0;
		ITM_write("[");
		ITM_write(buffStr);
		ITM_write("]\n");
		vTaskDelay(50);
		USB_send((uint8_t *)buffStr+1, len-1);
		if (strncmp(buffStr, "M10", 3) == 0) {
			USB_send((uint8_t *) M10Ans, strlen(M10Ans));
			USB_send((uint8_t *) okAns, strlen(okAns));
		} else if (strncmp(buffStr, "M1", 2) == 0) {
			USB_send((uint8_t *) okAns, strlen(okAns));

		} else if (strncmp(buffStr, "G1", 2) == 0) {
			USB_send((uint8_t *) okAns, strlen(okAns));
		} else {
			USB_send((uint8_t *) okAns, strlen(okAns));
		}
	}
}
