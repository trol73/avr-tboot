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

#include "datafile.h"

#include "filestream.h"
#include "strutils.h"

int ParseNibble(char c) {
	int result = -1;
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}
	if ( c >= 'a' && c <= 'f' ) {
		return c - 'a' + 10;
	}
	return -1;
}


int HexToByte(const std::string& s) {
	if ( s.length() != 2 ) {
		return -1;
	}
	int hi = ParseNibble(s[0]);
	int lo = ParseNibble(s[1]);
	if ( hi < 0 || lo < 0 ) {
		return -1;
	}
	return (hi << 4) + lo;
}


int HexToWord(const std::string& s) {
	if ( s.length() != 4 ) {
		return -1;
	}
	
	int hi = HexToByte(s.substr(0, 2));
	int lo = HexToByte(s.substr(2, 2));
	if ( hi < 0 || lo < 0 ) {
		return -1;
	}
	return (hi << 8) + lo;
}


char NibbleToHex(unsigned char v) {
	if ( v < 10 ) {
		return '0' + v;
	}
	if ( v < 16 ) {
		return 'A' + v - 10;
	}
	return '!';
}


std::string HexToStr(int v, int size) {	
	if ( size == 1 ) {
		std::string s;
		s.resize(2);
		s[0] = NibbleToHex((v >> 4) & 0x0F);
		s[1] = NibbleToHex(v & 0x0F);
		return s;
	} 
	if ( size == 2 ) {
		return HexToStr((v >> 8) & 0xFF, 1) + HexToStr(v & 0xFF, 1);
	}
	return "!!";
}


DataFile::DataFile() {
	error = false;
	data.resize(1024*1024);
	size = 0;
	for (size_t i = 0; i < data.size(); i++) {
		data[i] = -1;
	}
}


DataFile::~DataFile() {
}


// Loads any file (hex or binary)
bool DataFile::Load(const std::string &fileName) {
	definedBytes = 0;
	std::string ext = StrLower(ExtractFileExt(fileName));
	if ( ext == "hex" ) {
		return ReadHexFile(fileName);
	} else if ( ext == "bin" ) {
		return ReadBinFile(fileName);
	}
	error = true;
	printf("Unknown file format: %s\n", fileName.c_str());
	return false;
}


// Load a Intel hex file
bool DataFile::ReadHexFile(const std::string &fileName) {
	FileStream f;

	if ( !f.Open(fileName.c_str(), FileStream::EOpenRead) ) {
		error = true;
		printf("Can't open source file: %s\n", fileName.c_str());
		return false;
	}

	int line = 0;
	int baseAddres = 0;
	bool linearMode = true;
	while ( !f.Eof() ) {
		char buf[1024];
		f.ReadLine(buf, 1024);
		line++;
		std::string s = StrLower(Trim(buf));
		if ( s.length() == 0 ) {
			continue;
		}
		if ( s[0] != ':' ) {
			error = true;
			printf("%s: error in line %i, ':' expected\n", fileName.c_str(), line);
			return false;
		}
		bool lineError = false;
		if ( s.length() < 11 ) {
			lineError = true;
		}
		int pos = 1;
		int len = HexToByte(s.substr(pos, 2)); pos += 2;
		int address = HexToWord(s.substr(pos, 4)); pos += 4;
		int type = HexToByte(s.substr(pos, 2)); pos += 2;

		// CRC verifycation
		unsigned char crc = 0;
		for (size_t i = 1; i < s.length(); i += 2) {
			int b = HexToByte(s.substr(i, 2));
			if ( b < 0 ) {
				lineError = true;
				break;
			}
			crc += b;
		}
		if ( crc != 0 && (!lineError) ) {
			printf("%s: CRC error in line %i\n", fileName.c_str(), line);
			error = true;
			return false;
		}

		if ( !lineError )	{
			switch ( type )  {
				case 0:
					if ( linearMode ) {
						for (int i = 0; i < len; i++) {
							int b = HexToByte(s.substr(pos, 2)); pos += 2;
							SetByte(address+i, b);
						}
					} else {
						for (int i = 0; i < len; i++) {
							int b = HexToByte(s.substr(pos, 2)); pos += 2;
							SetByte(baseAddres + address+i, b);
						}
					}
					break;

				case 1:
					return !lineError;
				case 2:
					linearMode = false;
					if ( address != 0 ) {
						printf("%s: invalid offset (not zerro) in line %i\n", fileName.c_str(), line);
						error = true;
						return false;
					}					
					baseAddres = HexToWord(s.substr(pos, 4)) << 4;	pos += 4;
//					printf("BA = %x\n", baseAddres);
					break;
//				case 3:
//					break;
				default:
					printf("%s: error in line %i, unknown record type - %i\n", fileName.c_str(), line, type);
					error = true;
					return false;
			}
		} 
		if ( lineError ) {
			error = true;
			printf("%s: error in line %i\n", fileName.c_str(), line);
			return false;
		}

		
	}

	if ( f.IsError() ) {
		error = true;
		printf("File reading error: %s\n", fileName.c_str());
		return false;
	}


	return true;
}


// Loads a binary file
bool DataFile::ReadBinFile(const std::string &fileName) {
	FileStream f;
	if ( !f.Open(fileName.c_str(), FileStream::EOpenRead) ) {
		error = true;
		printf("Can't open source file: %s\n", fileName.c_str());
		return false;
	}
	for ( unsigned long i = 0; i < f.GetSize(); i++)
		SetByte(i, f.ReadByte());
//	while ( !f.Eof() ) 
//		data.push_back(f.ReadByte());

	if ( f.IsError() ) {
		error = true;
		printf("File reading error: %s\n", fileName.c_str());
		return false;
	}


	return true;
}


// Saves any file (hex or binary)
bool DataFile::SaveFile(const std::string &fileName, unsigned char *data, int length) {
	std::string ext = StrLower(ExtractFileExt(fileName));
	if ( ext == "hex" )
		return SaveHexFile(fileName, data, length);
	else if ( ext == "bin" )
		return SaveBinFile(fileName, data, length);	
	printf("Unknown file format: %s\n", fileName.c_str());
	return false;
}


// Saves a Intel hex file
bool DataFile::SaveHexFile(const std::string &fileName, unsigned char *data, int length) {
	FileStream f;
	if ( !f.Open(fileName.c_str(), FileStream::ECreateWrite) ) {
		printf("can't create file: %s\n", fileName.c_str());
		return false;
	}
	int offset = 0;
	while ( offset < length ) {		
		int len = 16;		
		if ( offset + len > length )
			len = length - offset;
		unsigned char crc = 0;
		std::string s = ":";
		s += HexToStr(len, 1);
		crc += len;
		s += HexToStr(offset, 2);
		crc += (offset >> 8) & 0xFF;
		crc += offset & 0xFF;
		s += HexToStr(0, 1);
		for (int i = 0; i < len; i++) {
			s += HexToStr(data[offset+i], 1);
			crc += data[offset+i];
		}
		crc = - crc;
		s += HexToStr(crc, 1);
		s += "\x0D\x0A";
		offset += len;
		f.Write((void*)s.c_str(), s.length());
	}
	std::string s = ":00000001FF\x0D\x0A";
	f.Write((void*)s.c_str(), s.length());

	return true;
}


// Saves a binary file
bool DataFile::SaveBinFile(const std::string &fileName, unsigned char *data, int length) {
	FileStream f;
	if ( !f.Open(fileName.c_str(), FileStream::ECreateWrite) ) {
		printf("can't create file: %s\n", fileName.c_str());
		return false;
	}
	f.Write(data, length);
	return true;
}