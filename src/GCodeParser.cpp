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

bool GCodeParser::read() {
	char str[96];
	char buffStr[96];
	uint32_t len = 0;

	len = USB_receive((uint8_t *) str, 96);
	if (len > 0) {
		for (uint32_t i = 0; i < len; ++i) {
			buffStr[i] = str[i];
		}
		if (strncmp(buffStr, "M10", 3) == 0) {
			USB_send((uint8_t *) M10Ans, strlen(M10Ans));
			USB_send((uint8_t *) okAns, strlen(okAns));
			return false;
		} else if (strncmp(buffStr, "M1", 2) == 0) {
			USB_send((uint8_t *) okAns, strlen(okAns));
			return false;

		} else if (strncmp(buffStr, "G1", 2) == 0) {
			USB_send((uint8_t *) okAns, strlen(okAns));
			parseCoords(buffStr);
			return true;
		} else {
			USB_send((uint8_t *) okAns, strlen(okAns));
			return false;
		}
	}
}

void GCodeParser::parseCoords(char* buffer) {
	char temp[8];
	int tempIndex = 0;
	double xCoord = 0;
	double yCoord = 0;
	for (int i = 0; i < strlen(buffer); i++) {
		if (buffer[i] == 'X') {
			for (int j = i + 1; isspace(buffer[j]) == false; ++j) {
				temp[tempIndex] = buffer[j];
				tempIndex++;
			}
			xCoord = atof(temp);
			xCoord_ = xCoord;
		}
		int tempIndex = 0;
		if (buffer[i] == 'Y') {
			for (int k = i + 1; isspace(buffer[k]) == false; ++k) {
				temp[tempIndex] = buffer[k];
				tempIndex++;
			}
			yCoord = atof(temp);
			yCoord_ = yCoord;
		}
	}
}
