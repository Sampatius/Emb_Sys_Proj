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

int GCodeParser::read(char* inputStr) {
	if (strncmp(inputStr, "M10", 3) == 0) {
		return 1;
	} else if (strncmp(inputStr, "M1", 2) == 0) {
		parseAngle(inputStr);
		return 2;
	} else if (strncmp(inputStr, "G1", 2) == 0) {
		parseCoords(inputStr);
		return 3;
	} else {
		return 0;
	}
}

void GCodeParser::parseAngle(char* buffer) {
	char temp[5];
	int angle;
	sscanf(buffer, "%s%d", temp, &angle);
	angle_ = angle;
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
			temp[strlen(temp)] = 0;
			xCoord = atof(temp);
			xCoord_ = xCoord;
		}
		int tempIndex = 0;
		if (buffer[i] == 'Y') {
			for (int k = i + 1; isspace(buffer[k]) == false; ++k) {
				temp[tempIndex] = buffer[k];
				tempIndex++;
			}
			temp[strlen(temp)] = 0;
			yCoord = atof(temp);
			yCoord_ = yCoord;
		}
	}
}
