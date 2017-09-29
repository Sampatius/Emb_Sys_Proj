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
#include "ITM_write.h"

class GCodeParser {
public:
	GCodeParser();
	virtual ~GCodeParser();

	void read();

private:
	char M10Ans[48] = "M10 XY 380 310 0.00 0.00 A0 B0 H0 S80 U160 D90\n";
	char okAns[4] = "OK\n";
};

#endif /* GCODEPARSER_H_ */
