/*
 * Copyright (c) 2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "ui.h"
#include "config.h"

#include <cstdio>
#include <cstdarg>

#ifndef WIN32
	#include <curses.h>
#endif



UI::UI(Config *config) {
	this->cfg = config;
	this->lastPercent = -1;
}

void UI::Error(const char * fmt, ...) {
	char outStr[0xff];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(outStr, sizeof(outStr), fmt, argptr);
	va_end(argptr);

	printf("\nERROR: %s\n", outStr);
}

void UI::Info(const char *fmt, ...) {
	if ( cfg->verbose ) {
		return;
	}
	char outStr[0xff];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(outStr, sizeof(outStr), fmt, argptr);
	va_end(argptr);
	printf("%s\n", outStr);
}

void UI::Warning(const char * fmt, ...) {
	char outStr[0xff];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(outStr, sizeof(outStr), fmt, argptr);
	va_end(argptr);

	printf("WARNING: %s\n", outStr);
}

void UI::Message(const char * fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	vprintf(fmt, argptr);
	va_end(argptr);
}



// Prints the progress-bar
void UI::Progress(const char* msg, int percent, float time) {
	if ( cfg->quellprogress ) {
		return;
	}
	setvbuf(stderr, (char*)NULL, _IONBF, 0);
#ifndef WIN32
//	curs_set(0);
#endif
	char progress[51];
	progress[50] = 0;
	for ( int i = 0; i < 50; i++ ) {
		progress[i] = 2*i <= percent ? '#' : ' ';
	}
	printf("\r%s [ %s ] %i%% %.2fs", msg, progress, percent, time/1e6);
	setvbuf(stderr, (char*)NULL, _IOLBF, 0);
#ifndef WIN32
//	curs_set(2);
#endif
//	printf("%i\n", percent);
}




// Prints the progress-bar for the reading operation
void UI::ProgressRead(int percent, float time) {
	if ( lastPercent == percent && time - lastTime < 0.5) {
		return;
	}
	lastPercent = percent;
	lastTime = time;
	Progress("Reading", percent, time);
	fflush(stdout);
}


// Prints the progress-bar for the writing operation
void UI::ProgressWrite(int percent, float time) {
	if ( lastPercent == percent && time - lastTime < 0.5 ) {
		return;
	}
	lastPercent = percent;
	lastTime = time;
	Progress("Writing", percent, time);
	fflush(stdout);
}

void UI::ProgressDone() {
	if ( !cfg->quellprogress ) {
		printf("\n");
	}
}

