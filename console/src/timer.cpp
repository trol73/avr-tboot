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

#include "timer.h"

#include <cstddef>

#ifdef WIN32
	#include <windows.h>


	TIME_T Timer::Frequency = 0;

#else
	#include <sys/time.h>
#endif


Timer::Timer() {
	iStarted = false;
	iTotalTime = 0;

#ifdef WIN32
	if ( Frequency == 0 ) {
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		Frequency = f.QuadPart;
	}
#endif
}


TIME_T __fastcall Timer::GetTicks() {   // get current time in CPU (usec)
#ifdef WIN32
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (TIME_T)(t.QuadPart*1e6/Frequency);
#else
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000000 + t.tv_usec;
#endif
}


void __fastcall Timer::Start() {
	if ( iStarted ) {
		return;
	}
	iStartTime = GetTicks();
	iStarted = true;
}



void __fastcall Timer::Stop() {
	if ( !iStarted ) {
		return;
	}
	iTotalTime += GetTicks() - iStartTime;
	iStarted = false;
}



void __fastcall Timer::Reset() {
	iTotalTime = 0;
	if ( iStarted ) {
		iStarted = false;
		Start();
	}
}


TIME_T __fastcall Timer::GetTime() {
	TIME_T result = iTotalTime;
	if ( iStarted ) {
		result += GetTicks() - iStartTime;
	}
	return result;
}



