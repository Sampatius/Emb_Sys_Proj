/*
 * GCodeParser.h
 *
 *  Created on: Sep 28, 2017
 *      Author: Sami
 */

#ifndef GCODEPARSER_H_
#define GCODEPARSER_H_

#include "FreeRTOS.h"
#include "user_vcom.h"
#include "string.h"
#include "task.h"
#include "ctype.h"
#include "stdlib.h"
#include "ITM_write.h"

class GCodeParser {
public:
	GCodeParser();
	virtual ~GCodeParser();

	int read(char* inputStr);

	inline double getXCoord() { return xCoord_; }
	inline double getYCoord() { return yCoord_; }

private:

	void parseCoords(char* buffer);

	double xCoord_;
	double yCoord_;
};

#endif /* GCODEPARSER_H_ */
