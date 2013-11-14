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


#ifndef _DATA_FILE_H_
#define _DATA_FILE_H_

#include <string>
#include <vector>
#include <map>

std::string HexToStr(int v, int size);

/**
 * Firmware data class. Loads and saves a data in Intel hex or binary file formats.
 */
class DataFile {
public:
	DataFile();
	~DataFile();

	// Loads a file
	bool Load(const std::string &fileName);

	// Returns binary data size
	int GetSize() { return size; }

	// Saves a data to file (in Intel hex or binary format)
	static bool SaveFile(const std::string &fileName, unsigned char *data, int length);

private:
	// Loads a Intel hex file
	bool ReadHexFile(const std::string &fileName);

	// Loads a binary file
	bool ReadBinFile(const std::string &fileName);

	// Saves a Intel hex file
	static bool SaveHexFile(const std::string &fileName, unsigned char *data, int length);

	// Saves a binary file
	static bool SaveBinFile(const std::string &fileName, unsigned char *data, int length);

	bool error;

	void SetByte(int offset, int val) {
		data[offset] = val;
		if ( val >= 0 )
			definedBytes++;
		if ( offset+1 > size )
			size = offset + 1;
	};


public:
	std::vector<int> data;	// firmware data. value -1 means that to this address of anything it is not necessary to write
		int size;
	int definedBytes;			// defined bytes count (for Intel hex-files may be less that size)
};


#endif	// _DATA_FILE_H_