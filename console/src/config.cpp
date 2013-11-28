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

#include "config.h"


#include <cstring>

#include "filestream.h"
#include "strutils.h"

Config::Config() {
#ifdef WIN32
	port = "COM1";
#else
	port = "/dev/ttyS0";
#endif
	baudrate = 115200;
	verify = true;
	verbose = false;
	quellprogress = false;
	smart = true;
	fileName = "tboot.conf";
	no_write = false;
	readTimeout = 1000;
	writeTimeout = 1000;
	loggingLevel = 0;
}


Config::~Config() {
}

bool Config::Load() {
	FileStream f;
	if ( !f.Open(fileName.c_str(), FileStream::EOpenRead) ) {
		printf("can't open config file: %s\n", fileName.c_str());
		return false;
	}
	TDeviceInfo *dev = NULL;
	line = 0;
	while ( !f.Eof() ) {
		char str[1024];
		line ++;
		f.ReadLine(str, 1024);
		std::string s = Trim(str);
		if ( s.length() == 0 || s[0] == '#' )
			continue;
		if ( s == "DEVICE" ) {
			if ( dev != NULL ) {
				printf("Error in config file, line %i: 'end' expected\n", line);
				return false;
			}
			dev = new TDeviceInfo();
		} else if ( s == "END" ) {
			if ( dev == NULL ) {
				printf("Error in config file, line %i: 'device' expected\n", line);
				return false;
			}
			devices.push_back(dev);
			dev = NULL;
		} else {
			std::string name = Trim(ExtractWord(1, s, "="));
			std::string value = Trim(ExtractWord(2, s, "="));
 			if ( dev == NULL ) {
				if ( name == "port" ) {
					if ( !InitStrParam(port, value) )
						return false;
				} else if ( name == "baudrate" ) {
					if ( !InitUIntParam(baudrate, value) )
						return false;
				} else if ( name == "verify" ) {
					if ( !InitBoolParam(verify, value) )
						return false;
				} else if ( name == "verbose" ) {
					if ( !InitBoolParam(verbose, value) )
						return false;
				} else if ( name == "quellprogress" ) {
					if ( !InitBoolParam(quellprogress, value) )
						return false;
				} else if ( name == "smart" ) {
					if ( !InitBoolParam(smart, value) )
						return false;
				} else if ( name == "device" ) {
					if ( !InitStrParam(device, value) )
						return false;
				} else if ( name == "readTimeout" ) {
					if ( !InitULongParam(readTimeout, value) ) {
						return false;
					}
				} else if ( name == "writeTimeout" ) {
					if ( !InitULongParam(writeTimeout, value) ) {
						return false;
					}
				} else if ( name == "loggingLevel" ) {
					if ( !InitIntParam(loggingLevel, value) ) {
						return false;
					}
				} else if ( name == "startCommand" ) {
					if ( !InitStrParam(startCommand, value) ) {
						return false;
					}
				} else if ( name == "finishCommand" ) {
					if ( !InitStrParam(finishCommand, value) ) {
						return false;
					}
				} else {
					printf("unknown config param: %s, line %i\n", name.c_str(), line);
					return false;
				}
			} else {
				if ( name == "id" ) {
					if ( !InitStrParam(dev->id, value) )
						return false;
				} else if ( name == "desc" ) {
					if ( !InitStrParam(dev->desc, value) )
						return false;
				} else if ( name == "rom_size" ) {
					if ( !InitIntParam(dev->rom_size, value) )
						return false;
				} else if ( name == "page_size" ) {
					if ( !InitIntParam(dev->page_size, value) )
						return false;
				} else if ( name == "eeprom_size" ) {
					if ( !InitIntParam(dev->eeprom_size, value) )
						return false;
				} else {
					printf("unknown config device param: %s, line %i\n", name.c_str(), line);
					return false;
				}

			}
		}
	}
	return true;
}

bool Config::SelectDevice(const std::string &id) {
	for ( size_t i = 0; i < devices.size(); i++ )
		if ( devices[i]->id == id ) {
			device_id = devices[i]->id;
			device_desc = devices[i]->desc;
			device_rom_size = devices[i]->rom_size;
			device_page_size = devices[i]->page_size;
			device_eeprom_size = devices[i]->eeprom_size;
			device = id;
			return true;
		}
	printf("Error: unknown device - %s\n", id.c_str());
	return false;
}


bool Config::ParseCommandLine(int argc, char *argv[]) {
	char errorStr[] = "Error in command line: ";
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		std::string argValue;
		if ( i < argc-1 )
			argValue = argv[i+1];
		if ( arg == "-p" ) {
			if ( argValue.length() == 0 ) {
				printf("%s -p <partno> argument expected\n", errorStr);
				return false;
			}
			device = argValue;
		} else if ( arg == "-b" ) {
			if ( argValue.length() == 0 ) {
				printf("%s -b <baudrate> argument expected\n", errorStr);
				return false;
			}
			if ( !VerifyInt(argValue) ) {
				printf("%s invalid argument -b %s\n", errorStr, argValue.c_str());
				return false;
			}
			baudrate = StrToInt(argValue);
		} else if ( arg == "-C" ) {
			if ( argValue.length() == 0 ) {
				printf("%s -C <config-file> argument expected\n", errorStr);
				return false;
			}
			fileName = argValue;
		} else if ( arg == "-P" ) {
			if ( argValue.length() == 0 ) {
				printf("%s -P <port> argument expected\n", errorStr);
				return false;
			}
			if ( !VerifyInt(argValue) ) {
				printf("%s invalid argument -P %s\n", errorStr, argValue.c_str());
				return false;
			}
			port = StrToInt(argValue);
		} else if ( arg == "-n" ) {
			no_write = true;
		} else if ( arg == "-V" ) {
			verify = false;
		} else if ( arg == "-v" ) {
			verbose = true;
		} else if ( arg == "-q" ) {
			quellprogress = true;
		} else if ( arg == "-U" ) {
			if ( argValue.length() == 0 ) {
				printf("%s -U argument expected\n", errorStr);
				return false;
			}
			std::string memType = ExtractWord(1, argValue, ":");
			std::string operationType = ExtractWord(2, argValue, ":");
			std::string file = ExtractWord(3, argValue, ":");

			TOperation *operation = new TOperation();
			if ( memType == "flash" )
				operation->target = 'f';
			else if ( memType == "eeprom" )
				operation->target = 'e';
			else {
				printf("%s -U %s  invalid memory, 'flash' or 'eeprom' expected\n", errorStr, argValue.c_str());
				return false;
			}
			if ( operationType == "r" )
				operation->type = 'r';
			else if ( operationType == "w" )
				operation->type = 'w';
			else if ( operationType == "v" )
				operation->type = 'v';
			else {
				printf("%s -U %s  invalid operation, 'r', 'w' or 'v' expected\n", errorStr, argValue.c_str());
				return false;
			}
			if ( file.length() == 0 )  {
				printf("%s -U %s  filename not defined\n", errorStr, argValue.c_str());
				return false;
			}
			operation->filename = file;
			operations.push_back(operation);
		}
	}
	if ( operations.size() == 0 ) {
		printf("%s operation not defined\n", errorStr);
		return false;
	}
	return true;
}


bool Config::InitBoolParam(bool &val, const std::string &str) {
	if ( str == "true" ) {
		val = true;
	} else if ( str == "false" ) {
		val = false;
	} else {
		printf("Invalid boolean param: '%s' in line %i\n", str.c_str(), line);
		return false;
	}
	return true;
}


bool Config::InitIntParam(int &val, const std::string &str) {
	if ( !VerifyInt(str) ) {
		printf("Invalid integer param: %s in line %i\n", str.c_str(), line);
		return false;
	}
	val = StrToInt(str);
	return true;
}

bool Config::InitUIntParam(unsigned int &val, const std::string &str) {
	if ( !VerifyInt(str) ) {
		printf("Invalid unsigned integer param: %s in line %i\n", str.c_str(), line);
		return false;
	}
	int i = StrToInt(str);
	if ( i < 0 ) {
		printf("Negative value: %s in line %i\n", str.c_str(), line);
		return false;
	}
	val = i;
	return true;
}


bool Config::InitULongParam(unsigned long &val, const std::string &str) {
	if ( !VerifyLong(str) ) {
		printf("Invalid unsigned long param: %s in line %i\n", str.c_str(), line);
		return false;
	}
	int i = StrToLong(str);
	if ( i < 0 ) {
		printf("Negative value: %s in line %i\n", str.c_str(), line);
		return false;
	}
	val = i;
	return true;
}


bool Config::InitStrParam(std::string &val, const std::string &str) {
	if ( str.length() < 2 || str[0] != '"' || str[str.length()-1] != '"' ) {
		printf("Invalid string param: %s in line %i\n", str.c_str(), line);
		return false;
	}
	val = str.substr(1, str.length()-2);
	return true;
}
