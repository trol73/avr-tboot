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

#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef WIN32
	#define TIME_T __int64
#else
	#define TIME_T unsigned long long
	#define __fastcall
#endif

class Timer {
public:
	Timer();

	void __fastcall Start();
	void __fastcall Stop();
	void __fastcall Reset();

	TIME_T __fastcall GetTime();		// get time in microseconds
	
	float __fastcall GetTimeSec() { return GetTime()/1e6; }

	bool __fastcall IsStarted() { return iStarted; }

protected:
#ifdef WIN32
	static TIME_T Frequency;			// the CPU frequency, Hz
#endif
	bool iStarted;							// true, if timer is running
	TIME_T iStartTime;					// starting time
	TIME_T iTotalTime;					// total counted time
	static TIME_T __fastcall GetTicks();	// get current time in CPU (usec)
};




#endif // _TIMER_H_
