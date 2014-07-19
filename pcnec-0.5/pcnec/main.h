/***************************************************************************
                          main.h  -  description
                             -------------------
    begin                : Fri Aug 9 2002
    copyright            : (C) 2002 by Paolo Subiaco - http://www.creasol.it
    email                : psubiaco@creasol.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MAIN_H
#define _MAIN_H

// Define your platform
#define _LINUX_
//#define _CYGWIN_

extern unsigned int cpuclock;   // target cpu clock in KHz
extern unsigned char *code_ptr; // Place where store code data used to program micro
extern int debug;   // used for debugging purposes
extern unsigned int loopsPerMicroSecond;

#define BUFFERLEN 512
extern char buffer[];         // General purpose buffer
extern char family[];         // microcontroller family
extern unsigned char debugmsg;	// 1 => print debugging messages
void printerror(const char *);      // print error, then exit


 
#endif
