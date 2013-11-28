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

#include "device.h"

#include <cstdio>
#include <iostream>

#include "glog/logging.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

#include "rs232.h"
#include "timer.h"
#include "ui.h"
#include "log.h"
#include "strutils.h"


#define FLAG_SUPPORT_EEPROM			(1 << 0)
#define FLAG_FULL_ECHO_MODE			(1 << 1)
#define FLAG_BINARY_MODE			(1 << 2)
#define FLAG_FAST_MODE_READ			(1 << 3)
#define FLAG_FAST_MODE_WRITE		(1 << 4)
#define FLAG_NO_CONFIRM_MODE		(1 << 5)

Device::Device(UI *ui) {
	this->ui = ui;
	this->com = new RS232();
	this->timerRead = new Timer();
	this->timerWrite = new Timer();
	this->timerUartIn = new Timer();
	this->timerUartOut = new Timer();
	this->noWrite = false;
	this->totalBytesIn = 0;
	this->totalBytesOut = 0;
	this->valueOfZ = -1;
	this->bootloaderFlags = 0;
	this->writeDelay = 0;
}


Device::~Device() {
	delete com;
	delete timerRead;
	delete timerWrite;
	delete timerUartIn;
	delete timerUartOut;
}


// Connect to the bootloader
bool Device::Open(const char *comName, int baudRate) {
	// open serial device
	LOG(INFO) << "Open serial device " << comName << " ...";
	if ( !com->Open(comName, baudRate) ) {
		LOG(ERROR) << "can't open device " << comName;
		ui->Error("can't open %s\n", comName);
		return false;
	}
	LOG(INFO) << "Device was successfully opened";
	return true;
}

Timer tmrErase, tmrWrite, tmrFinish;

void Device::Close() {
    com->Flush();
    com->Close();
    printf("TIME %.2f  %.2f  %.2f\n", tmrErase.GetTimeSec(), tmrWrite.GetTimeSec(), tmrFinish.GetTimeSec());
    // TIME 105.20  105.94  70.67
    // TIME 108.38  71.71  107.78
    //  71.61  72.24  71.99
    // TIME 71.95  71.91  71.96
    // TIME 71.68  72.15  0.00
}

// Set the Z-register
bool Device::CommandZ(uint z) {
	VLOG(VLOG_LOADER_COMMANDS) << "command Z " << WordToHex(z);
	if ( valueOfZ == z ) {
		VLOG(VLOG_LOADER_COMMANDS) << "command Z - already set";
		return true;
	}

	if ( !writeByte('Z') ) {
		LOG(ERROR) << "write byte error - Z";
		return false;
	}
	sendHexWord(z);

	if ((bootloaderFlags & FLAG_NO_CONFIRM_MODE) == 0) {
		unsigned char c = readByte();
		if ( !com->IsOk() ) {
			LOG(ERROR) << "COMMAND Z timeout";
			valueOfZ = -1;
			return false;
		}
		if ( c != 0x0D ) {
			LOG(ERROR) << "COMMAND Z invalid response - " << ByteToHex(c) << ' ' << '[' << c << ']';
			valueOfZ = -1;
			return false;
		}
	}

	valueOfZ = z;
	return true;
}


// Reads a data block
bool Device::CommandR(unsigned char *data, int size, uint offset) {
	VLOG(VLOG_LOADER_COMMANDS) << "COMMAND R " << size << ", " << offset;

	if  ( !writeByte('R') ) {
		LOG(ERROR) << "COMMAND R: write byte error - R";
		return false;
	}
	if ( bootloaderFlags & FLAG_FAST_MODE_READ ) {
		if ( !sendHexWord(size) ) {
			LOG(ERROR) << "COMMAND R: write word error - size";
			return false;
		}
	} else {
		if ( !sendHexByte(size) ) {
			LOG(ERROR) << "COMMAND R: write byte error - size";
			return false;
		}
	}
	for ( uint i = 0; i < size; i++ ) {
		int b = getHexByte();
		if ( b < 0 || !com->IsOk( )) {
			valueOfZ = -1;
			LOG(ERROR) << "COMMAND R: can't read byte, offset @" << WordToHex(i);
			return false;
		}
		data[i] = b;
		if ( valueOfZ >= 0 ) {
			valueOfZ++;
		}
		ui->ProgressRead(100*(offset+i)/fullDataSize, timerRead->GetTime());
	}

	if ((bootloaderFlags & FLAG_NO_CONFIRM_MODE) == 0) {
		uint resp = readByte();
		if ( !com->IsOk() ) {
			LOG(ERROR) << "COMMAND R timeout";
			return false;
		}
		if ( resp != 0x0d ) {
			LOG(ERROR) << "COMMAND R: invalid response " << CharToStrForLog(resp);
			return false;
		}
	}
	return true;
}


// Writes a data block
bool Device::CommandW(unsigned char *data, int size) {
	VLOG(VLOG_LOADER_COMMANDS) << "COMMAND W " << size;

	if ( size == 0 ) {
		VLOG(VLOG_LOADER_DETAILS) << "empty page";
		return true;
	}
	if ( size % 2 != 0 ) {
		LOG(ERROR) << "COMMAND W - invalid size";
		return false;
	}
	if ( !writeByte('W') ) {
		LOG(ERROR) << "COMMAND W: write byte error - W";
		return false;
	}
	VLOG(VLOG_LOADER_DETAILS) << "send size/2  " << (size/2);
	if ( !sendHexByte(size/2) ) {
		LOG(ERROR) << "COMMAND W: write word error - size/2";
		return false;
	}
	VLOG(VLOG_LOADER_DETAILS) << "send data block";
	for (int i = 0; i < size; i++) {
		if ( !sendHexByte(data[i]) ) {
			LOG(ERROR) << "COMMAND W: write data byte error, offset = " << i;
			return false;
		}
		if ( valueOfZ >= 0 ) {
			valueOfZ += 2;
		}
LOG(INFO) << "write " << i << " byte from " << size;
	}


	if ((bootloaderFlags & FLAG_NO_CONFIRM_MODE) == 0) {
		uint resp = readByte();
		if ( !com->IsOk() ) {
			LOG(ERROR) << "COMMAND W timeout";
			return false;
		}
		if ( resp != 0x0d ) {
			LOG(ERROR) << "COMMAND W: invalid response " << CharToStrForLog(resp);
			return false;
		}
	}


	return true;
}


// Reads the R0:R1 pair, then one byte into SPMCR and executes the SPM instruction
bool Device::CommandP(int r0r1, int spmcr) {
	VLOG(VLOG_LOADER_COMMANDS) << "COMMAND P " << spmcr;
	if ( !writeByte('P') ) {
		LOG(ERROR) << "write byte error - P";
		return false;
	}
//	if ( !sendHexWord(r0r1) ) {
//		LOG(ERROR) << "COMMAND P: write R0R1 error";
//		return false;
//	}
	if ( !sendHexByte(spmcr) ) {
		LOG(ERROR) << "COMMAND P: write SPMCR error";
		return false;
	}


	if ((bootloaderFlags & FLAG_NO_CONFIRM_MODE) == 0) {
		uint resp = readByte();
		if ( !com->IsOk() ) {
			LOG(ERROR) << "COMMAND P timeout";
			return false;
		}
		if ( resp != 0x0d ) {
			LOG(ERROR) << "COMMAND P: invalid response " << CharToStrForLog(resp);
			return false;
		}
	}
	return true;
}


// Reads a memory block with size <= 0x1000 / 0xff
bool Device::ReadBlock(int offset, unsigned char *data, int size) {
	VLOG(VLOG_LOADER_PAGES) << "read page #" << WordToHex(offset) << " size " << size;
	if ( size > getMaxReadBlockSize() ) {
		LOG(ERROR) << "READ BLOCK - invalid size " << size;
		return false;
	}
	if ( offset < 0 ) {
		LOG(ERROR) << "READ BLOCK - invalid offset " << offset;
		return false;
	}
	if ( !CommandZ(offset) ) {
		LOG(ERROR) << "READ PAGE ERROR - Z";
		return false;
	}
	if ( !CommandR(data, size, offset) ) {
		LOG(ERROR) << "READ PAGE ERROR - R";
		return false;
	}
	VLOG(VLOG_LOADER_PAGES) << "read page completed";
	return true;
}


// Reads all chip memory
bool Device::ReadAll(unsigned char *data, int size) {
	if ( size < 0 ) {
		LOG(ERROR) << "READ ALL - invalid size " << size;
		return false;
	}
	if ( size == 0 ) {
		return true;
	}
	timerRead->Start();
	int offset = 0;
	fullDataSize = size;


	int bytesRemains = size;
	int blockSize;
	int maxBlockSize = getMaxReadBlockSize();
	while ( bytesRemains > 0 ) {
		ui->ProgressRead(100*offset/fullDataSize, timerRead->GetTime());
		blockSize = bytesRemains > maxBlockSize ? maxBlockSize : bytesRemains;
		VLOG(VLOG_LOADER_PAGES) << "read block @" << WordToHex(offset) << ", size " << blockSize;
		if ( !ReadBlock(offset, &data[offset], blockSize) ) {
			return false;
		}
		offset += blockSize;
		bytesRemains -= blockSize;
	}
/*
	unsigned int useBlockSize = pageSize;
	unsigned int totalPages = size/useBlockSize;

	for ( unsigned int page = 0; page < totalPages; page++ ) {
		ui->ProgressRead(100*offset/fullDataSize, timerRead->GetTime());
		VLOG(VLOG_LOADER_PAGES) << "read page " << page << " / " << totalPages;
		if ( !ReadBlock(offset, &data[offset], useBlockSize) ) {
			return false;
		}
		offset += pageSize;
	}
*/
	timerRead->Stop();
	ui->ProgressRead(100, timerRead->GetTime());
	ui->ProgressDone();
	return true;
}


// Erases a memory page and mark it available
bool Device::erasePage(int offset) {

	if ( !CommandZ(offset) ) {
		LOG(ERROR) << "WRITE PAGE - set offset error";
		return false;
	}
	if ((bootloaderFlags & FLAG_FAST_MODE_WRITE) != 0) {
//		return true;
	}
	if ( !noWrite ) {
		// Erase a page
		if ( !CommandP(0x0000, 0x03) ) {	// PGERS & SPMEN
			LOG(ERROR) << "WRITE PAGE - ERASE ERROR";
			return false;
		}
		// Mark this page section as available
//		if ( !CommandP(0x0000, 0x11) ) {	// RWWSRE & SPMEN
//			LOG(ERROR) << "WRITE PAGE - MARK ERROR";
//			return false;
//		}
	}

	// [???] may be it's a some delay? [???] !!!!!
//	if ( GetLoaderOffset() <= 0 ) {
//		LOG(ERROR) << "WRITE PAGE - GET LOADER OFFSET ERROR";
//		return false;
//	}

	return true;
}

// Writes a one memory page
bool Device::WritePage(int offset, unsigned char *data, int size) {
	VLOG(VLOG_LOADER_PAGES) << "write page #" << WordToHex(offset) << " size " << size;

tmrErase.Start();

	erasePage(offset);
tmrErase.Stop();

tmrWrite.Start();
	// Send a data to the page buffer
	if ( !CommandW(data, size) ) {
		LOG(ERROR) << "WRITE PAGE - WRITE BLOCK ERROR";
		return false;
	}

tmrWrite.Stop();
	// [???] may be it's a some delay? [???] !!!
//	if ( GetLoaderOffset() <= 0 ) {
//		LOG(ERROR) << "WRITE PAGE - GET LOADER OFFSET-2 ERROR";
//		return false;
//	}

tmrFinish.Start();
	if ((bootloaderFlags & FLAG_FAST_MODE_WRITE) == 0) {
		if ( !CommandZ(offset) ) {
			LOG(ERROR) << "WRITE PAGE - SET OFFSET-2 ERROR";
			return false;
		}
	}
	if ( !noWrite ) {
		// Write the Page Buffer to Flash memory
// Z used, r0r1 doesn't used
		if ((bootloaderFlags & FLAG_FAST_MODE_WRITE) == 0) {
			if ( !CommandP(0x0000, 0x05) ) {		// PGWRT & SPMEN
				LOG(ERROR) << "WRITE PAGE - WRITE COMMAND ERROR";
				return false;
			}
		}
		// Mark this page section as available
// r0r1 && Z ignored
//		if ( !CommandP(0x0000, 0x11) ) {		// RWWSRE & SPMEN
//			LOG(ERROR) << "WRITE PAGE - MARK AVAILABLE ERROR";
//			return false;
//		}
tmrFinish.Stop();
	}

	// [???] may be it's a some delay? [???]
//	if ( GetLoaderOffset() <= 0 ) {
//		LOG(ERROR) << "WRITE PAGE - get loader offset error";
//		return false;
//	}
	VLOG(VLOG_LOADER_PAGES) << "write page completed";
	return true;
}


// Gets a bootloader start offset
int Device::GetLoaderOffset() {
	VLOG(VLOG_LOADER_COMMANDS) << "get loader offset";

	if ( !writeByte('Q') ) {
		LOG(ERROR) << "write byte error - Q";
		return -1;
	}
	int z = getHexWord();
	if ( valueOfZ >= 0 && z != valueOfZ ) {
		LOG(ERROR) << "FATAL ERROR FOR 'Q' COMAMND! Device returned Z = " << WordToHex(z) << " but cached value = " << WordToHex(valueOfZ);
	} else if ( valueOfZ < 0 ) {
		valueOfZ = z;
	}
	int result = getHexWord();
	int flags = getHexByte();
	uint resp = readByte();
	if ( resp != 0x0D ) {
		LOG(ERROR) << "GET LOADER OFFSET - invalid response " << CharToStrForLog(resp);
		return -1;
	}
	if ( result < 0 ) {
		LOG(ERROR) << "COMMAND Q - invalid offset";
		return -1;
	}
	if ( flags < 0 ) {
		LOG(ERROR) << "COMMAND Q - invalid flags";
		return -1;
	} else {
		bootloaderFlags = flags;
	}
	VLOG(VLOG_LOADER_COMMANDS) << "loader offset = " << WordToHex(result);
	return result;
}


// Writes all memory pages
// If smart == true, then before writing of a page it will be read completely.
// And if changes is not required, the page won't write
// If byte value is less than zero, this byte won't changed
bool Device::WriteAll(int *data, int size, bool smart) {
	pagesWrited = 0;
	int pagesCount = size/pageSize;
	if ( size % pageSize > 0 ) {
		pagesCount++;
	}
	pagesForWriteTotal = pagesCount;
	if ( pagesCount == 0 ) {
		return true;
	}


	// if the "smart" mode is enabled, that read all necessary pages
	unsigned char *readed = 0;
	if ( smart ) {
		 readed = new unsigned char[pagesCount*pageSize];
		 if ( !ReadAll(readed, pagesCount*pageSize) ) {
			 delete[] readed;
			 return false;
		 }
	}
	timerWrite->Start();

	int offset = 0;
	for (int page = 0; page < pagesCount; page++) {
		bool writePage = false;	// need to rewrite this page
		bool needReading = false;// need to read this page

		ui->ProgressWrite(100*page/pagesCount, timerWrite->GetTime());
		// let's check up, whether it's necessary to update this page
		for (uint i = 0; i < pageSize; i++) {
			ui->ProgressWrite(100*page/pagesCount, timerWrite->GetTime());
			int addr = offset + i;
			if ( addr >= size ) {
				needReading = true;
				break;
			}
			if ( data[addr] < 0 ) {
				needReading = true;
				continue;
			}
			if ( smart && data[addr] == readed[addr] ) {
				continue;
			}
			writePage = true;
		}

		if ( writePage ) {
			pagesWrited++;
			// prepare page data before writing
			unsigned char *pageData = new unsigned char[pageSize];
			// if a part of the page-data is not defined, need to read it at first
			if ( needReading ) {
				if ( smart ) {
					// page data already read
					for (uint i = 0; i < pageSize; i++) {
						pageData[i] = readed[offset + i];
					}
				} else {
					if ( !ReadBlock(offset, pageData, pageSize) ) {
printf("READ PAGE ERROR\n");
						delete[] pageData;
						return false;
					}
				}
			}
			// now change all necessary bytes
			for ( uint i = 0; i < pageSize; i++ ) {
				if ( offset + i < size && data[offset + i] >= 0 ) {
					pageData[i] = data[offset + i];
				}
			}
			// writes a page
			if ( !WritePage(offset, pageData, pageSize) ) {
				delete[] pageData;
				return false;
			}
			delete[] pageData;
		} // writePage

		offset += pageSize;
	} // for (page)

	if ( smart ) {
		delete[] readed;
	}
	timerWrite->Stop();
	ui->ProgressWrite(100, timerWrite->GetTime());
	ui->ProgressDone();

	timerRead->Reset();	// for verification
	return true;
}


// Jump to program offset
bool Device::Jump(int offset) {
	if ( !CommandZ(offset) ) {
		return false;
	}
	if ( !writeByte('@') ) {
		LOG(ERROR) << "write byte error - @";
		return false;
	}
	if ( readByte() != 0x0D ) {
		return false;
	}
	return true;
}


// Reset the bootloader and verify the signature
bool Device::Reset() {
	// detect echo mode
	fullEchoMode = false;

	if ( !SendCommandStr(startCommand) ) {
		LOG(ERROR) << "can't send start command " << startCommand;
		return false;
	}

//	int comTimeout = com->GetReadTimeOut();
//	com->SetReadTimeOut(500);
	com->ClearError();
	VLOG(VLOG_LOADER_DETAILS) << "skip bytes begin";
	while ( true ) {
		char ch = com->ReadByte();
		VLOG(VLOG_LOADER_DETAILS) << "char = " << CharToStrForLog(ch);
		if ( !com->IsOk() ) {
			break;
		}
	}
	VLOG(VLOG_LOADER_DETAILS) << "skip bytes end";
	com->ClearError();
//	com->SetReadTimeOut(comTimeout);

	// detect echo mode
	uint foundEchoCnt = 0;
	uint foundNoEchoCnt = 0;
	uint foundError = 0;
	for ( char i = 9; i >= 0; i--) {
		com->ClearError();
		char send = '0' + i;
		com->WriteByte(send);
		if ( !com->IsOk() ) {
			break;
		}
		char resp1 = com->ReadByte();
		if ( !com->IsOk() ) {
			continue;
		}
		if ( resp1 == send ) {
			char resp2 = com->ReadByte();
			if ( resp2 == '!' ) {
				foundEchoCnt++;
			} else {
				foundError++;
			}
		} else if ( resp1 == '!' ) {
			foundNoEchoCnt++;
		}
		if ( foundEchoCnt > 3 ) {
			fullEchoMode = true;
			LOG(INFO) << "echo mode detected";
			break;
		} else if ( foundNoEchoCnt > 4 ) {
			fullEchoMode = false;
			LOG(INFO) << "no echo mode detected";
			break;
		} else if ( i == 0 ) {
			if ( foundEchoCnt == foundNoEchoCnt ) {
				LOG(INFO) << "nothing detected";
				return false;
			}
			fullEchoMode = foundEchoCnt > foundNoEchoCnt;
			if ( fullEchoMode ) {
				LOG(INFO) << "probably echo mode";
			} else {
				LOG(INFO) << "probably not-echo mode: " << foundEchoCnt << " : " << foundNoEchoCnt;
			}
		}
	}

	if ( !writeByte('Q') ) {
		return false;
	}
	char ch1 = readByte();
	char ch2 = readByte();
	char ch3 = readByte();
	char ch4 = readByte();
	char ch5 = readByte();

	if ( !com->IsOk() ) {
		return false;
	}
	if ( isHexChar(ch1) && isHexChar(ch2) && isHexChar(ch3) && isHexChar(ch4) && isHexChar(ch5) ) {
		binaryMode = false;
		char ch6 = readByte();
		char ch7 = readByte();
		char ch8 = readByte();
		char ch9 = readByte();
		char ch10 = readByte();
		if ( isHexChar(ch6) && isHexChar(ch7) && isHexChar(ch8) && isHexChar(ch9) && isHexChar(ch10) ) {
			int hiOffset = getHexByteForChars(ch5, ch6);
			if ( hiOffset < 0 ) {
				return false;
			}
			int lowOffset = getHexByteForChars(ch7, ch8);
			if ( lowOffset < 0 ) {
				return false;
			}
			int flags = getHexByteForChars(ch5, ch6);
			if ( flags < 0 ) {
				return false;
			}
			bootloaderOffset =  (hiOffset << 8) + lowOffset;
			bootloaderFlags = flags;
		}
	} else {
		binaryMode = true;
		bootloaderOffset = (ch3 << 8) + ch4;
		bootloaderFlags = ch5;
	}
	char chEnd = readByte();
	if ( !com->IsOk() ) {
		return false;
	}
	if ( chEnd != 0x0d ) {
		return false;
	}


/*
	valueOfZ = -1;
	// get the bootloader offset
	writeByte('Q');
	char c1 = readByte();
	char c2 = readByte();
	if ( c1 == 'o' && c2 == 'k' ) {	// if a programmer has been started earlier than the device bootloader
		if ( readByte() != 0x0D ) {
printf("!!! 1\n");
			return false;
		}
		writeByte('Q');
		if ( getHexWord() < 0 ) {
printf("!!! 2\n");
			return false;
		}
	} else {	// otherwise read Z
		if ( getHexByte() < 0 ) {
printf("!!! 3\n");
			return false;
		}
	}
	int bootOffset = getHexWord() + 0x0E;	// [!!!] ignore verification
	if ( readByte() != 0x0D ) {
printf("!!! 4\n");
		return false;
	}
	if ( !CommandZ(bootOffset) ) {
printf("!!! 5\n");
		return false;
	}

	writeByte('Q');
	int zz = getHexWord();
printf("JMP  zz = %x\n", zz);
	int bs = getHexWord();
printf("JMP  bs = %x\n", bs);
	if ( readByte() != 0x0D ) {
printf("RB  ? = %i\n", com->IsOk());
		return false;
	}
*/

	if ( bootloaderFlags & FLAG_FULL_ECHO_MODE ) {
		if ( !fullEchoMode ) {
			LOG(ERROR) << "error on echo mode detection - not detected but enabled on device";
		}
	} else {
		if ( fullEchoMode ) {
			LOG(ERROR) << "error on echo mode detection - detected but disabled on device";
		}
	}

	if ( bootloaderFlags & FLAG_BINARY_MODE ) {
		if ( !binaryMode ) {
			LOG(ERROR) << "error on binary mode detection - not detected but enabled on device";
		}
	} else {
		if ( fullEchoMode ) {
			LOG(ERROR) << "error on binary mode detection - detected but disabled on device";
		}
	}

	if ( fullEchoMode ) {
		ui->Info("echo-mode detected");
	}
	if ( binaryMode ) {
		ui->Info("binary-mode detected");
	} else {
		ui->Info("hex-mode detected");
	}
	if ( bootloaderFlags & FLAG_FAST_MODE_READ ) {
		ui->Info("fast read mode enabled");
	}
	if ( bootloaderFlags & FLAG_FAST_MODE_WRITE ) {
		ui->Info("fast write mode enabled");
	}
	if ( bootloaderFlags & FLAG_SUPPORT_EEPROM ) {
		ui->Info("EEPROM support enabled");
	}


	timerUartIn->Reset();
	timerUartOut->Reset();
	totalBytesIn = 0;
	totalBytesOut = 0;


/*
	com->WriteByte('@');
	if ( readByte() != 'o' )
		return false;
	if ( readByte() != 'k' )
		return false;
	if ( readByte() != 0x0D )
		return false;
*/

	int offset = GetLoaderOffset();
	VLOG(VLOG_LOADER_DETAILS) << "bootloader offset: " << offset;
	if ( offset <= 0 ) {
		LOG(ERROR) << "can't get bootloader offset";
		return false;
	}
	return true;
}

bool Device::writeByte(unsigned char val) {
	VLOG(VLOG_UART_TRAFFIC) << "UART WRITE BYTE " << ByteToHex(val) << CharToStrForLog(val);
	timerUartOut->Start();
	if ( writeDelay > 0 ) {
#ifdef WIN32
	//	Sleep(1);
#else
		usleep(writeDelay);
#endif
	}
	com->WriteByte(val);
	timerUartOut->Stop();

	if ( !com->IsOk() ) {
		LOG(ERROR) << "WRITE BYTE ERROR";
		return false;
	}
	if ( fullEchoMode ) {
		uint resp = readByte();
		if ( !com->IsOk() ) {
			LOG(ERROR) << "WRITY BYTE ECHO TIMEOUT";
			return false;
		}
		if ( val != resp ) {
			LOG(ERROR) << "WRITE BYTE ECHO ERROR - put " << CharToStrForLog(val) << " get " << CharToStrForLog(resp);
			return false;
		}
	}

	totalBytesOut++;
	return true;
}


uint Device::readByte() {
	timerUartIn->Start();
	uint ch = com->ReadByte();
	timerUartIn->Stop();
	VLOG(VLOG_UART_TRAFFIC) << "UART READ " << ByteToHex(ch) << CharToStrForLog(ch);
	if ( !com->IsOk() ) {
		VLOG(VLOG_UART_TRAFFIC) << "READ TIMEOUT";
		com->ClearError();
		return 0xffff;
	}
	totalBytesIn++;
	return ch;
}




// Sends one byte in hex format
bool Device::sendHexByte(unsigned char b) {
	VLOG(VLOG_LOADER_DETAILS) << "SEND BYTE " << ByteToHex(b);
	if ( binaryMode ) {
		return writeByte(b);
	}
	unsigned char hi = (b >> 4) & 0x0F;
	unsigned char lo = b & 0x0F;
	return writeByte(hi < 10 ? hi + '0' : hi + 'a' - 10) && writeByte(lo < 10 ? lo + '0' : lo + 'a' - 10);
}


// Sends one word in hex format
bool Device::sendHexWord(unsigned short w) {
	VLOG(VLOG_LOADER_DETAILS) << "SEND WORD " << WordToHex(w);
	return sendHexByte((w >> 8) & 0xFF) && sendHexByte(w & 0xFF);
}


// Reads one byte in hex format. returns -1 if there was an error
int Device::getHexByte() {
	VLOG(VLOG_LOADER_DETAILS) << "Get " << (binaryMode ? "bin" : "hex") << " byte";
	uint hi = readByte();
	if ( !com->IsOk() || hi > 0xff ) {
		LOG(ERROR) << "GET HEX BYTE READ ERROR [hi]";
		return -1;
	}
	if ( binaryMode ) {
		VLOG(VLOG_LOADER_DETAILS) << "get binary byte from device " << ByteToHex(hi);
		return hi;
	}
	uint lo = readByte();
	if ( !com->IsOk() || lo > 0xff ) {
		LOG(ERROR) << "GET HEX BYTE READ ERROR [lo]";
		return -1;
	}
	int result = getHexByteForChars(hi, lo);
	if ( result >= 0 ) {
		VLOG(VLOG_LOADER_DETAILS) << "get hex byte from device " << ByteToHex(result);
	}
	return result;
}


// Reads one byte in hex format. returns -1 if there was an error
int Device::getHexWord() {
	VLOG(VLOG_LOADER_DETAILS) << "Get " << (binaryMode ? "bin" : "hex") << " word";
	int hi = getHexByte();
	if ( hi < 0 || hi > 0xff ) {
		VLOG(VLOG_LOADER_DETAILS) << "Get hex word - FIRST BYTE ERROR";
		return -1;
	}
	int lo = getHexByte();
	if ( lo < 0 || lo > 0xff ) {
		VLOG(VLOG_LOADER_DETAILS) << "Get hex word - SECOND BYTE ERROR";
		return -1;
	}
	int result = (hi << 8) + lo;
	VLOG(VLOG_LOADER_DETAILS) << "get " << (binaryMode ? "bin" : "hex") << " word from device " << WordToHex(result);
	return result;
}

std::string Device::CharToStrForLog(char ch) {
	if ( ch >= 0x20 && ch <= 0x7e ) {
		std::string result;
		result.resize(4);
		result[0] = ' ';
		result[1] = '[';
		result[2] = ch;
		result[3] = ']';
		return result;
	} else if ( ch == '\n' ) {
		return " [\\n]";
	} else if ( ch == '\r' ) {
		return " [\\r]";
	} else if ( ch == '\t' ) {
		return " [\\t]";
	}
	return "";
}


// returns true for chars 0-9, a-f
bool Device::isHexChar(char ch) {
	return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
}

// returns binary bytes from hex form (or -1 on error)
int Device::getHexByteForChars(char hi, char lo) {
	if ( hi >= '0' && hi <= '9' ) {
		hi -= '0';
	} else if ( hi >= 'a' && hi <= 'f' ) {
		hi -= 'a' - 10;
	} else {
		VLOG(VLOG_LOADER_DETAILS) << "Invalid hex byte from device - " << ByteToHex(hi) << ' ' << ByteToHex(lo);
		return -1;
	}
	if ( lo >= '0' && lo <= '9' ) {
		lo -= '0';
	} else if ( lo >= 'a' && lo <= 'f' ) {
		lo -= 'a' - 10;
	} else {
		VLOG(VLOG_LOADER_DETAILS) << "Invalid hex byte from device - " << ByteToHex(hi) << ' ' << ByteToHex(lo);
		return -1;
	}
	return  (hi << 4) + lo;
}

// Returns the max size for reading block (0xffff if the fast-mode is enabled or 0xff otherwise)
uint Device::getMaxReadBlockSize() {
//	return bootloaderFlags & FLAG_FAST_MODE_READ ? 0xffff : 0xff;
	return bootloaderFlags & FLAG_FAST_MODE_READ ? 0x1000 : 0xff;
//	return bootloaderFlags & FLAG_FAST_MODE_READ ? 0x2000 : 0xff;
	//return bootloaderFlags & FLAG_FAST_MODE_READ ? 0x4000 : 0xff;
	//return bootloaderFlags & FLAG_FAST_MODE_READ ? 1024*14 : 0xff;			// TODO !!!!!!!! MOVE TO CONFIG !!!!!!!!!!!!
}


void Device::SetTimeouts(unsigned long writeTimeout, unsigned long readTimeout) {
	com->SetWriteTimeOut(writeTimeout);
	com->SetReadTimeOut(readTimeout);
}


// Sends string to UART
bool Device::SendCommandStr(std::string &cmd) {
	VLOG(VLOG_LOADER_COMMANDS) << "send command to device " << cmd;
	uint pos = 0;
	while ( pos < cmd.length() ) {
		char ch = cmd[pos++];
		if ( ch == '\\' && pos < cmd.length()-1 ) {
			switch ( cmd[pos++] ) {
				case 'n':
					ch = '\n';
					break;
				case 'r':
					ch = '\r';
					break;
				case 't':
					ch = '\t';
					break;
				case '\\':
					ch = '\\';
					break;
				case 'x':
					if ( pos <= cmd.length()-2 ) {
						std::string sv = cmd.substr(pos, 2);
						int code = StrToHexByte(sv);
						if ( code >= 0 && code <= 0xff ) {
							ch = code;
						} else {
							LOG(ERROR) << "invalid hex code: " << sv;
							return false;
						}
						pos += 2;
					} else {
						pos--;
					}
					break;
				default:
					LOG(ERROR) << "invalid ascii sequence in string: " << cmd;
					return false;
			} // switch
		}
		if ( !writeByte(ch) ) {
			return false;
		}
	}
	return true;
}

