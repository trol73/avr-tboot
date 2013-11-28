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

#ifndef _STRUTILS_H_
#define _STRUTILS_H_


#include <string>


// Strip whitespace (or other characters) from the beginning and end of a string
std::string Trim(const std::string &aStr);

// Return extension of file
std::string ExtractFileExt(const std::string str);

// Convert a string to lowercase
std::string StrLower(const std::string &str);

// Convert a string to uppercase
std::string StrUpper(const std::string &str);

// Return words count in the string for specified delims symbols
int WordCount(const std::string &str, const std::string &wordDelims);

// Return position of the word in string (or -1 if the word doesn't exists)
int WordPosition(const unsigned int n, const std::string &str, const std::string &wordDelims);

// Return extracted word from string by number (1, 2, ...)
std::string ExtractWord(const unsigned int n, const std::string &str, const std::string &wordDelims);



// Return true, if the string is a valid representation of integer
bool VerifyInt(const std::string &str);

// Convert the string to integer
int StrToInt(const std::string &str);

// Return true, if the string is a valid representation of long
bool VerifyLong(const std::string &str);

// Convert the string to long
long StrToLong(const std::string &str);


// Convert integer to string
std::string IntToStr(int v);

// Return true, if string is a valid representation of float
bool VerifyFloat(const std::string &str);

// Convert the string to float
double StrToFloat(const std::string &str);

// Convert float to string
// precision - number of characters after decimal dot
std::string FloatToStr(double v, unsigned char precesion);

std::string ByteToHex(unsigned char val);

std::string WordToHex(unsigned short val);

std::string DWordToHex(unsigned short val);


int StrToHexByte(std::string &str);


#endif // _STRUTILS_H_
