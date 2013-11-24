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

#include "strutils.h"

#include <cstdlib>
#include <cstdio>


using namespace std;



// Strip whitespace (or other characters) from the beginning and end of a string
string Trim(const string &str) {
	if ( str.length() == 0 )
		return str;
	size_t beg = str.find_first_not_of(" \a\b\f\n\r\t\v");
	size_t end = str.find_last_not_of(" \a\b\f\n\r\t\v");
	// if string contains spaces only
	if ( beg == string::npos )
		return "";
	return string(str, beg, end+1-beg);
}



// Return extension of file
string ExtractFileExt(const string str) {
	size_t posDot = str.find_last_of(".");
	if ( posDot == string::npos )
		return "";
	size_t posSlash = str.find_last_of("\\/");
	if ( posSlash == string::npos || posSlash < posDot )
		return string(str, posDot+1);
	return "";
}


// Convert a string to lowercase
string StrLower(const string &str) {
	std::string result;

	result.resize(str.length());
	for (size_t i = 0; i < str.length(); i++)
		result[i] = tolower(str[i]);
	return result;
};


// Convert a string to uppercase
string StrUpper(const string &str) {
	std::string result;

	result.resize(str.length());
	for (size_t i = 0; i < str.length(); i++)
		result[i] = toupper(str[i]);
	return result;
};



// Return words count in the string for specified delims symbols
int WordCount(const std::string &str, const std::string &wordDelims) {
	int res = 0;
	int i = 0;
	int len = (int)str.length();

	while ( i < len ) {
		while ( (i < len) && (wordDelims.find(str[i]) != string::npos) )
			i++;
		if (i < len)
			res++;
		while ( (i < len) && (wordDelims.find(str[i]) == string::npos) )
			i++;
	};
	return res;
}



// Return position of the word in string
int WordPosition(const unsigned int n, const std::string &str, const std::string &wordDelims) {
	int count = 0;
	unsigned int i = 0;
	int res = -1;
	unsigned int len = str.length();

	while ( (i < len) && (count != n) ) {
		// skiping delims in end of the string
		while ( (i <= len) && (wordDelims.find(str[i]) != string::npos) )
			i++;
		// it's beginтing of a new word
		if ( i < len )
			count++;
		// it's a end of current word
		if ( count != n )
			while ( (i < len) && (wordDelims.find(str[i]) == string::npos) )
				i++;
		else
			res = i;
	};
	return res;
}



// Return extracted word from string by number (1, 2, ...)
std::string ExtractWord(const unsigned int n, const std::string &str, const std::string &wordDelims) {
	int i = WordPosition(n, str, wordDelims);
	string res("");

	if (i >= 0)  {
		// finding end of current word
		while ( (i < str.length() ) && (wordDelims.find(str[i]) == string::npos) ) {
			res += str[i++];
		};
	}
	return res;
}



// Return true, if the string is a valid representation of integer
bool VerifyInt(const std::string &str) {
	if ( str.length() == 0 ) {
		return false;
	}
	char *stopstring;
	strtol(str.c_str(), &stopstring, 10);
	return ( stopstring[0] == 0 );
}


// Convert string to integer
int StrToInt(const std::string &str) {
	return atoi(str.c_str());
}


// Convert integer to string
std::string IntToStr(int v) {
	char s[16];
#ifdef WIN32
	sprintf_s(s, "%i", v);
#else
	sprintf(s, "%i", v);
#endif
	return s;
}


// Return true, if string is a valid float representation
bool VerifyFloat(const std::string &str) {
	if ( str.length() == 0 )
		return false;
	char *stopstring;
	strtod(str.c_str(), &stopstring);
	return ( stopstring[0] == 0 );
}


// Convert string to float
double StrToFloat(const std::string &str) {
	return atof(str.c_str());
}


// Convert float to string
// precision - number of characters after decimal dot
std::string FloatToStr(double v, unsigned char precesion) {
	char mask[10];
	char result[64];
#ifdef WIN32
	sprintf_s(mask, "%%.%if", precesion);
	sprintf_s(result, mask, v);
#else
	sprintf(mask, "%%.%if", precesion);
	sprintf(result, mask, v);
#endif
	return result;
}

std::string ByteToHex(unsigned char val) {
	unsigned char hi = (val >> 4) & 0x0F;
	unsigned char lo = val & 0x0F;

	std::string result;
	result.resize(2);
	result[0] = hi < 10 ? hi + '0' : hi + 'a' - 10;
	result[1] = lo < 10 ? lo + '0' : lo + 'a' - 10;
	return result;
}

std::string WordToHex(unsigned short val) {
	unsigned short hi = val >> 8;
	unsigned short lo = val & 0xFF;
	return ByteToHex(hi) + ByteToHex(lo);
}

std::string DWordToHex(unsigned int val) {
	unsigned int hi = val >> 16;
	unsigned int lo = val & 0xFFFFF;
	return WordToHex(hi) + WordToHex(lo);
}


int StrToHexByte(std::string &str) {
	if ( str.length() != 2 ) {
		return -1;
	}
	char hi = str[0];
	char lo = str[1];
	if ( hi >= '0' && hi <= '9' ) {
		hi -= '0';
	} else if ( hi >= 'a' && hi <= 'f' ) {
		hi -= 'a' - 10;
	} else if ( hi >= 'A' && hi <= 'F' ) {
		hi -= 'A' - 10;
	} else {
		return -1;
	}
	if ( lo >= '0' && lo <= '9' ) {
		lo -= '0';
	} else if ( lo >= 'a' && lo <= 'f' ) {
		lo -= 'a' - 10;
	} else if ( lo >= 'A' && lo <= 'F' ) {
		lo -= 'A' - 10;
	} else {
		return -1;
	}
	return (hi << 4) + lo;
}
