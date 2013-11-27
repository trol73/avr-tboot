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


#ifndef _DEVICE_H_
#define _DEVICE_H_

class UI;
class RS232;
//class Timer;

#include <string>
#include <sys/types.h>

#include "timer.h"

class Device {
public:
	Device(UI* ui);
	~Device();

	// Connect to the bootloader
	bool Open(const char *comName, int baudRate);
	
	// Close serial port and disconnect
	void Close();

	// Reads a memory block with size <= 0xff
	bool ReadBlock(int offset, unsigned char *data, int size);

	// Reads all chip memory
	bool ReadAll(unsigned char *data, int size);

	// Writes a memory page
	bool WritePage(int offset, unsigned char *data, int size);

	// Writes all memory pages
	// If smart == true, then before writing of a page it will be read completely.
	// And if changes is not required, the page won't write
	// If byte value is less than zero, this byte won't changed
	bool WriteAll(int *data, int size, bool smart);

	// Jump to program offset
	bool Jump(int offset);

	// Reset the bootloader and verify the signature
	bool Reset();

	// Gets a bootloader start offset
	int GetLoaderOffset();

	void SetPageSize(uint size) { pageSize = size; }

	void SetTimeouts(uint writeTimeout, uint readTimeout);

	// Disable writing to chip memory (debug mode)
	void DisableWriting(bool noWrite) { this->noWrite = noWrite; }

	uint GetTotalBytesIn() const { return totalBytesIn; }
	uint GetTotalBytesOut() const { return totalBytesOut; }
	float GetTotalReadTime() const { return timerUartIn->GetTime(); }
	float GetTotalWriteTime() const { return timerUartOut->GetTime(); }
	uint GetPagesWrited() const { return pagesWrited; }
	uint GetPagesForWriteTotal() const { return pagesForWriteTotal; }

	void SetStartCommand(std::string &cmd) { startCommand = cmd; }

	// Sends string to UART
	bool SendCommandStr(std::string &cmd);

private:
	RS232 *com;
	UI *ui;

	uint pageSize;
	bool noWrite;
	bool fullEchoMode;						// bootloader echo-mode. initialized on device reset
	bool binaryMode;						// bootloader binary-mode. initialized on device reset
	unsigned char bootloaderFlags;			// bootloader flags that returns 'Q' command
	uint bootloaderOffset;					// bootloader offset that returns 'Q' command
	uint writeDelay;
	uint totalBytesIn;
	uint totalBytesOut;
	
	Timer *timerRead;						// used in Progress-methods for read and verify operations
	Timer *timerWrite;						// used in Progress-method for write operation
	Timer *timerUartIn;
	Timer *timerUartOut;
	uint fullDataSize;						// used in Progress-methods
	int valueOfZ;							// if >= 0 then contains value of Z-register of MC-device
	
	int pagesWrited;						// WriteAll() stores here the count of written pages
	int pagesForWriteTotal;					// WriteAll() stores here the count of pages, witch were supposed to been written down

	std::string startCommand;				// this command will be send to UART on device initialization


	// Set the Z-register
	bool CommandZ(uint z);

	// Reads a data block
	bool CommandR(unsigned char *data, int size, uint offset);

	// Writes a data block
	bool CommandW(unsigned char *data, int size);
	
	// Reads the R0:R1 pair, then one byte into SPMCR and executes the SPM instruction
	bool CommandP(int r0r1, int spmcr);

	// Erases a memory page and mark it available
	bool erasePage(int offset);


	// write binary byte to UART
	bool writeByte(unsigned char val);

	// read binary byte from UART
	uint readByte();

	// Sends one byte in hex format
	bool sendHexByte(unsigned char b);

	// Sends one word in hex format
	bool sendHexWord(unsigned short w);

	// Reads one byte in hex format. returns -1 if there was an error
	int getHexByte();

	// Reads one word in hex format. returns -1 if there was an error
	int getHexWord();

	std::string CharToStrForLog(char ch);


	// returns true for chars 0-9, a-f
	bool isHexChar(char ch);

	// returns binary bytes from hex form (or -1 on error)
	int getHexByteForChars(char hi, char lo);

	// Returns the max size for reading block (0xffff if the fast-mode is enabled or 0xff otherwise)
	uint getMaxReadBlockSize();

};

#endif 
