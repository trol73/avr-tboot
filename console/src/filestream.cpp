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

#include "filestream.h"

#include <cstring>
#include <ctime>

#ifdef WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>


unsigned int FileStream::ReadByte() {
	unsigned int result = 0;
	iError = Read(&result, 1) != 1;
	return result;
}



unsigned int FileStream::ReadWord() {
	unsigned int result = 0;
	iError = Read(&result, 2) != 2;
	return result;
}



unsigned int FileStream::Read3Byte() {
	unsigned int result = 0;
	iError = Read(&result, 3) != 3;
	return result;
}



unsigned int FileStream::ReadDword() {
	unsigned int result = 0;
	iError = Read(&result, 4) != 4;
	return result;
}



int FileStream::ReadInt() {
	int result = 0;
	iError = Read(&result, 4) != 4;
	return result;
}


unsigned int FileStream::ReadLine(char *str, unsigned int maxLength) {
	unsigned int len = 0;
	bool firstChar = true;
	while ( len < maxLength && !Eof() ) {
		char c = ReadByte();
		if ( c == 0x0d || c == 0x0a ) {
			str[len] = 0;
			return len;
//		} else if ( c == 0x0a && firstChar ) {
//			firstChar = false;
		} else
			str[len++] = c;
	}
	str[len] = 0;
	return len;
}


void FileStream::WriteByte(unsigned int val) {
	iError = Write(&val, 1) != 1;
}



void FileStream::WriteWord(unsigned int val) {
	iError = Write(&val, 2) != 2;
}



void FileStream::Write3Byte(unsigned int val) {
	iError = Write(&val, 3) != 3;
}


void FileStream::WriteDword(unsigned int val) {
	iError = Write(&val, 4) != 4;
}


void FileStream::WriteInt(int val) {
	iError = Write(&val, 4) != 4;
}


bool FileStream::Open(const char *fileName, OpenMode mode) {
	char rb[] = "rb";
	char wb[] = "wb";
	char ab[] = "ab";
	char rbp[] = "rb+";
	char wbp[] = "wb+";
	char abp[] = "ab+";
	char *modes[6] = {rb, wb, ab, rbp, wbp, abp};
	iFile = fopen(fileName, modes[mode]);
	iOpened = iFile != NULL;
	iError = !iOpened;
	return iOpened;
}



bool FileStream::Seek(unsigned long pos) {
	if ( fseek(iFile, pos, SEEK_SET) != 0 ) {
		iError = true;
		return false;
	}
	return true;
}



unsigned long FileStream::GetPos() {
#ifdef WIN32
	fpos_t p;
	if ( fgetpos(iFile, &p) != 0 )
		iError = true;
	return p;
#else
	return ftell(iFile);
#endif
}



bool FileStream::Close() {
	if ( iOpened ) {
		bool f = fclose(iFile) == 0;
		if ( !f )
			iError = true;
		else
			iOpened = false;
		return f;
	}
	return false;
}



unsigned long FileStream::GetSize() {
	unsigned long p = GetPos();
//	fpos_t p;
//	if ( fgetpos(iFile, &p) != 0 )
//		iError = true;
	if ( fseek(iFile, 0, SEEK_END) != 0 )
		iError = true;
	unsigned long result = GetPos();
	Seek(p);
	return result;
}



bool FileStream::Eof() {
	return feof(iFile);
}


unsigned int FileStream::Read(void *buf, unsigned int size) {
	return (unsigned int)fread(buf, 1, size, iFile);
}



unsigned int FileStream::Write(void *buf, unsigned int size) {
	return (unsigned int)fwrite(buf, 1, size, iFile);
}

bool IsDirectoryExists(const char *absolutePath) {
#ifdef WIN32
	if ( _access( absolutePath, 0 ) == 0 ) {
		struct stat status;
		stat( absolutePath, &status );
		return (status.st_mode & S_IFDIR) != 0;
	}
	return false;
#else
	if ( access( absolutePath, 0 ) == 0 ) {
		struct stat status;
		stat( absolutePath, &status );
		return (status.st_mode & S_IFDIR) != 0;
	}
	return false;
#endif
}

