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

#ifndef _RS232_H_

#define _RS232_H_

#ifdef WIN32
	#include <windows.h>
#elif __APPLE__
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <termios.h>
    #include <string.h>
    #include <fcntl.h>
#else
	#include <termios.h>
#endif

#include <sys/types.h>


class RS232 {
public:
	RS232(const char *name, uint baud, bool parity = false, unsigned char stopBits = 0);
	RS232();
	~RS232();

	bool Open(const char *name, uint baud, bool parity = false, unsigned char stopBits = 0);

	uint Read(void *buf, uint size);

	uint Write(void *buf, uint size);

	void Flush();

	void Close();

	// Set/reset the DTR line
	bool SetDTR(bool val);
	
	// Set/reset the RTS line
	bool SetRTS(bool val);
	
	void WriteByte(uint val);
	void WriteByte(uint val, uint timeout);

	uint ReadByte();
	uint ReadByte(uint timeout);
	
	bool IsOk() { return !iError; }
	void ClearError() { iError = false; }
	
	// Set read timeout (microseconds)
	void SetReadTimeOut(int us) { readTimeout = us; };
	
	// Get read timeout (microseconds)
	int GetReadTimeOut() { return readTimeout; };
	
	// Set read timeout (microseconds)
	void SetWriteTimeOut(int us) { writeTimeout = us; };

	// Get read timeout (microseconds)
	int GetWriteTimeOut() { return writeTimeout; };

#ifndef WIN32
	struct termios new_port_settings, old_port_settings;
	static int getBaudrateValue(int baudrate);
#endif

private:
	bool iError;
	int readTimeout;			// read operation timeout, us
	int writeTimeout;			// write operation timeout, us
	uint baudrate;
#ifdef WIN32
	HANDLE handle;				// port file descriptor
#else
	int handle;
#endif

	void updateTimeout();
};

#endif // _RS232_H_
