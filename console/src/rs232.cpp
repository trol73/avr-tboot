/*
 * Copyright (c) 2007-2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "rs232.h"


#include <cstdio>

#include "timer.h"

#ifdef __linux__
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <limits.h>
	#include <string.h>
#endif

RS232::RS232() {
	iError = false;
#ifdef WIN32
	readTimeout = 0;
#else
	readTimeout = 1000*1000;
#endif
}


RS232::RS232(const char *name, uint baud, bool parity, unsigned char stopBits) {
	iError = !Open(name, baud, parity, stopBits);
#ifdef WIN32
	readTimeout = 0;
#else
	readTimeout = 1000*1000;
#endif
}


RS232::~RS232() {
	Close();
	iError = false;
}



uint RS232::Read(void *buf, uint size) {
#ifdef WIN32
	DWORD cnt;
	if ( ReadFile(handle, buf, size, &cnt, NULL) == FALSE )
		return -1;
	return cnt;
#else
	return read(handle, buf, size);
#endif
}


uint RS232::Write(void *buf, uint size) {
#ifdef WIN32
	DWORD cnt;
	if ( WriteFile(handle, buf, size, &cnt, NULL) == FALSE )
		return -1;
	return cnt;
#else
	return write(handle, buf, size);
#endif
}

void RS232::Flush() {
#ifdef WIN32
#else
	fsync(handle);
	#ifndef __APPLE__
	    fdatasync(handle);
	#endif
#endif
}


void RS232::Close() {
#ifdef WIN32
	if ( handle != NULL ) {
		CloseHandle(handle);
		handle = NULL;
	}
#else
	if ( handle != -1 ) {
		close(handle);
	#ifndef __APPLE__
		tcsetattr(handle, TCSANOW, &old_port_settings);
	#endif
		handle = -1;
	}
#endif
}


bool RS232::Open(const char *name, uint baudRate, bool parity, unsigned char stopBits) {
	baudrate = baudRate;
#ifdef WIN32
	handle = NULL;
	handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if ( handle == INVALID_HANDLE_VALUE )
		return false;

	// com-port initialisation
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	if( !GetCommState(handle, &dcb) ) {
		Close();
		return false;
	}

	dcb.BaudRate = baudRate;					// baudrate (in bauds)
	dcb.fBinary = true;							// binary mode
	dcb.fOutxCtsFlow = false;					// disable the CTS signal listening
	dcb.fOutxDsrFlow = false;					// disable the DSR signal listening
	dcb.fDtrControl = DTR_CONTROL_DISABLE;	// disable the DTR-line
	dcb.fDsrSensitivity = false;				// ignore state of the DSR-line
	dcb.fNull = false;							// allow zero-bytes input
	dcb.fRtsControl = RTS_CONTROL_DISABLE;	// not use the RTS-line
	dcb.fAbortOnError = false;					// disable abort on error
	dcb.ByteSize = 8;								// 8 bits per byte
	dcb.Parity = parity ? 1 : 0;				// parity checking
	dcb.StopBits = stopBits;					// stop-bits

	// load DCB to port
	if( !SetCommState(handle, &dcb) ) {
		Close();
		return false;
	}

	// set timeouts
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0;			// timeout between to chars
	timeouts.ReadTotalTimeoutMultiplier = 0;	// general reading operations timeout
	timeouts.ReadTotalTimeoutConstant = 0;		// constant for the general reading operations timeout
	timeouts.WriteTotalTimeoutMultiplier = 0;	// general writing operations timeout
	timeouts.WriteTotalTimeoutConstant = 0;	// constant for the general writing operations timeout
	if( !SetCommTimeouts(handle, &timeouts) )	{
		Close();
		return false;
	}

	// setup sizes of TX/RX quenes
	if ( SetupComm(handle, 1024*2, 1024*2) == FALSE ) {
		Close();
		return false;
	};

	// clear the reading buffer
	if ( PurgeComm(handle, PURGE_RXCLEAR) == FALSE ) {
		Close();
		return false;
	};
#else
	int baudr = getBaudrateValue(baudRate);
	if ( baudr == 0 ) {
		perror("invalid serial port baudrate\n");
		return false;
	}
	handle = open(name, O_RDWR | O_NOCTTY | O_NDELAY);// | O_RSYNC | O_SYNC | O_DSYNC);
	if( handle == -1 ) {
		perror("unable to open serial port");
		return false;
	}

	int error = tcgetattr(handle, &old_port_settings);
	if ( error == -1 ) {
		close(handle);
		perror("unable to read portattr");
		return false;
	}
	memset(&new_port_settings, 0, sizeof(new_port_settings));  // clear the new struct

	new_port_settings.c_cflag = baudr | CS8 | CLOCAL | CREAD;
	if ( parity ) {
		new_port_settings.c_cflag |= PARENB;
	}
	new_port_settings.c_iflag = IGNPAR;		// Ignore characters with parity errors.
	new_port_settings.c_oflag = 0;
	new_port_settings.c_lflag = 0;
	new_port_settings.c_cc[VMIN] = 0;      // block untill n bytes are received
	new_port_settings.c_cc[VTIME] = 0;     // block untill a timer expires (n * 100 mSec.)
	error = tcsetattr(handle, TCSANOW, &new_port_settings);
	if ( error == -1 ) {
		close(handle);
		perror("unable to adjust portsettings ");
		return false;
	}

#endif
	updateTimeout();
	return true;
}


#ifndef WIN32
int RS232::getBaudrateValue(int baudrate) {
	switch( baudrate ) {
		case 50:
			return B50;
		case 75:
			return B75;
		case 110:
			return B110;
		case 134:
			return B134;
		case 150:
			return B150;
		case 200:
			return B200;
		case 300:
			return B300;
		case 600:
			return B600;
		case 1200:
			return B1200;
		case 1800:
			return B1800;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
#ifndef __APPLE__
		case 460800:
			return B460800;
		case 500000:
			return B500000;
		case 576000:
			return B576000;
		case 921600:
			return B921600;
		case 1000000:
			return B1000000;
#endif
		default:
			return 0;
	}
}
#endif

// DTR-line control
bool RS232::SetDTR(bool val) {
#ifdef WIN32
	iError = EscapeCommFunction(handle, val ? SETDTR : CLRDTR) != TRUE ? true : false;
	return !iError;
#else
	int controlbits = TIOCM_DTR;
	iError = ioctl(handle, (val ? TIOCMBIS : TIOCMBIC), &controlbits) != 0;
	return !iError;
#endif
}


// RTS-line control
bool RS232::SetRTS(bool val) {
#ifdef WIN32
	iError = EscapeCommFunction(handle, val ? SETRTS : CLRRTS) != TRUE ? true : false;
	return !iError;
#else
	int controlbits;
	iError = ioctl(handle, TIOCMGET, &controlbits) != 0;
	if ( iError )
		return false;
	if ( val ) {
		controlbits |= TIOCM_RTS;
	} else {
		controlbits &= ~TIOCM_RTS;
	}
	iError = ioctl(handle, TIOCMSET, &controlbits) != 0;
	return !iError;
#endif
}


void RS232::WriteByte(uint val) {
	WriteByte(val, writeTimeout);
}

void RS232::WriteByte(uint val, uint timeout) {
	if ( timeout == 0 ) {
		iError = Write(&val, 1) != 1;
		return;
	}
	Timer timer;
	timer.Start();
	do {
		iError = Write(&val, 1) != 1;
#ifdef WIN32
		Sleep(1);
#else
		usleep(1);
#endif
		if ( timer.GetTime() >= timeout ) {
			break;
		}
	} while ( iError );
}


uint RS232::ReadByte() {
	return ReadByte(readTimeout);
}

uint RS232::ReadByte(uint timeout) {
	uint result = 0;
	if ( timeout == 0 ) {
		iError = Read(&result, 1) != 1;
		return result;
	}
	Timer timer;
	timer.Start();
	do {
		iError = Read(&result, 1) != 1;
#ifdef WIN32
		Sleep(1);
#else
		usleep(1);
#endif
		if ( timer.GetTime() >= timeout ) {
			break;
		}
	} while ( iError );
	return result;
}


void RS232::updateTimeout() {
#ifdef WIN32
	readTimeout = 0;
#else
	readTimeout = 1000*1000;
#endif
	if ( baudrate <= 600 ) {
		readTimeout *= 5;
	}
}
