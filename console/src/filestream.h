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

#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_

#include <cstdio>


// file I/O stream
class FileStream {
public:
	enum OpenMode {
		EOpenRead				= 0, // open a file for reading. file must exist
		ECreateWrite			= 1, // create a empty file for writing. existing file must be overwrited
		EAppendWrite			= 2, // append a file. if file not exist, it will be created
		EOpenReadWrite			= 3, // open a existing file for reading and writing
		ECreateReadWrite		= 4, // create a empty file for reading and writing. existing file will be overwrited
		EAppendRead				= 5  // open a file for reading and appending. if file not exist, it will be created
	};
	
	FileStream() { iOpened = false; iError = false; iFile = NULL; };
	FileStream(const char *fileName, OpenMode mode) { Open(fileName, mode); };
	virtual ~FileStream() { Close(); };

	bool Open(const char *fileName, OpenMode mode);

	bool Seek(unsigned long pos);
	unsigned long GetPos();
	bool Close();
	unsigned long GetSize();
	bool Eof();

	unsigned int Read(void *buf, unsigned int size);
	unsigned int Write(void *buf, unsigned int size);

	bool Flush() { return fflush(iFile) == 0; } 

#ifdef WIN32
	bool Reset() { return Seek(0); }
#else
	bool Reset() { return Seek(0); }
//	bool Reset() { return Seek({0}); }
#endif

	unsigned int ReadByte();
	unsigned int ReadWord();
	unsigned int Read3Byte();
	unsigned int ReadDword();
	int ReadInt();


	virtual unsigned int ReadLine(char *str, unsigned int maxLength);

	void WriteByte(unsigned int val);
	void WriteWord(unsigned int val);
	void Write3Byte(unsigned int val);
	void WriteDword(unsigned int val);
	void WriteInt(int val);

	bool IsError() { return iError; }
	bool IsOk() { return !iError; }


protected:
	bool iOpened;
	bool iError;
	FILE *iFile;
};


bool IsDirectoryExists(const char *name);


#endif // _FILESTREAM_H_
