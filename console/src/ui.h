/*
 * Copyright (c) 2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef _UI_H_
#define _UI_H_

class Config;
/*
 * TODO
 * 
 * 	XML config 
 * 	configurable timeout
 * 	send string on start
 * 	send string on exit
 * 	number of attempts
 * 	time limit for bootloader search
 * 	write progress-bar doesn't linear
 * 	pages to progress-bar
 * 	logging
 *
 *
avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.00s

avrdude: Device signature = 0x1e9502
avrdude: NOTE: FLASH memory has been specified, an erase cycle will be performed
         To disable this feature, specify the -D option.
avrdude: erasing chip
avrdude: reading input file "tboot.hex"
avrdude: writing flash (32448 bytes):

Writing | ################################################## | 100% 15.21s



avrdude: 32448 bytes of flash written
avrdude: verifying flash memory against tboot.hex:
avrdude: load data flash data from input file tboot.hex:
avrdude: input file tboot.hex contains 32448 bytes
avrdude: reading on-chip flash data:

Reading | ################################################## | 100% 9.04s



avrdude: verifying ...
avrdude: 32448 bytes of flash verified


 *
 *
 */ 
class UI {
public:
	UI(Config *config);

	void Error(const char * fmt, ...);

	void Info(const char *fmt, ...);

	void Warning(const char * fmt, ...);

	void Message(const char * fmt, ...);

	// Prints the progress-bar
	void Progress(const char* msg, int percent, float time);

	// Prints the progress-bar for the reading operation
	void ProgressRead(int percent, float time);

	// Prints the progress-bar for the writing operation
	void ProgressWrite(int percent, float time);

	//
	void ProgressDone();

private:
	Config *cfg;

	int lastPercent;
};

#endif // _UI_H_
