/*
 * Copyright (c) 2009-2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <sys/types.h>

#include <string>
#include <vector>

class Config {
public:
	Config();
	~Config();

	bool Load();

	bool SelectDevice(const std::string &id);

	bool ParseCommandLine(int argc, char *argv[]);

	// loader settings
	std::string port;			// UART device name
	unsigned int baudrate;		// UART baudrate
	bool verify;				// verify data after writing
	bool verbose;				// verbose output
	bool quellprogress;			// don't show the progress indicator
	bool smart;					// "smart" writing mode
	std::string device;			// microcontroller name
	bool no_write;				// don't write anything to chip, debug mode only

	// device settings
	std::string device_id;			// device name
	std::string device_desc;		// device description
	uint device_rom_size;	// size of ROM
	uint device_page_size;	// size of ROM page
	uint device_eeprom_size;// size of EEPROM

	struct TOperation {
		char type;		// operation type: r|w|v
		char target;	// memory type: f|e
		std::string filename;
	};

	std::vector<TOperation*> operations;

	uint writeTimeout;				// UART write operation timeout, microseconds
	uint readTimeout;				// UART read operation timeout, microseconds
	std::string startCommand;		// send this string to UART before start
	std::string finishCommand;		// send this string to UART after finish
	int loggingLevel;				// for glog::V

private:
	struct TDeviceInfo {
		std::string id;
		std::string desc;
		int rom_size;
		int page_size;
		int eeprom_size;
	};

	std::vector<TDeviceInfo*> devices;
	int line;							// current line in file
	std::string fileName;			// filename

	bool InitBoolParam(bool &val, const std::string &str);
	bool InitIntParam(int &val, const std::string &str);
	bool InitUIntParam(unsigned int &val, const std::string &str);
	bool InitStrParam(std::string &val, const std::string &str);
};

#endif
