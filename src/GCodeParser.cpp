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
		if (strncmp(buffStr, "M10", 4)) {
			USB_send((uint8_t *) mkInit, strlen(mkInit));
			USB_send((uint8_t *) okResponse, strlen(okResponse));
		} else if (strncmp(buffStr, "M1", 3)) {
			USB_send((uint8_t *) okResponse, strlen(okResponse));

		} else if (strncmp(buffStr, "G1", 3)) {
			USB_send((uint8_t *) okResponse, strlen(okResponse));
		}
	}
}
