/***************************************************************************
                          nec78k0.h  -  description
                             -------------------
    begin                : Thu Aug 8 2002
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

#ifndef _NEC78K0_H
#define _NEC78K0_H

#include "lowlevel.h"

void nec78k_Timings(void);
void nec78k_poweroff(ASPP *bus);
void nec78k_poweron(ASPP *bus, unsigned char vpp_pulses);
int nec78k_sync(ASPP *bus);
int nec78k_setosc(ASPP *bus);
int nec78k_seterase(ASPP *bus);
int nec78k_prewrite(ASPP *bus);
int nec78k_erase(ASPP *bus);
int nec78k_write(ASPP *bus, unsigned int addr, unsigned char *data, int len);
int nec78k_cwrite(ASPP *bus, unsigned char *data, int len); // continuous flash writing
int nec78k_program(ASPP *bus, FILE *hexfile, int len);      // main writing routine
int nec78k_blankcheck(ASPP *bus);                           // blank Check
int nec78k_intverify(ASPP *bus);                            // Verify deep level write
int nec78k_verify(ASPP *bus, unsigned char *data, unsigned int last_addr, int len); // Verify programmed data
int nec78k_readsignature(ASPP *bus);                        // Read microcontroller signature

#define MAXUNPGMDATA 10   // Max number of continuous bytes=0xff which can be programmed
#define NEC_ERASETIME (unsigned long)2500000

#endif

typedef struct {
  unsigned long TimeComAck;         // Time between a command and the ACK
  unsigned long TimeAckCom;         // Time between an ACK and successive Command
  unsigned long TimeAckData;        // Time between ACK and Data
  unsigned long TimeDataData;       // Time between two successive data
  unsigned long TimeDataAck;        // Time between data and ACK req
  unsigned long TimeFrqCalc;        // Frequency calculation time
  unsigned long TimeEraseCalc;      // Time to perform erase time calculation
  unsigned long TimePreWrite;       // Prewrite time in microseconds
  unsigned long TimeBlankCheck;     // Blank check time
  unsigned long TimeErase;					// Erase Time in microseconds
	unsigned long ClkWrite;           // Number of clocks/byte written
  unsigned long TimeWrite;          // Write time in microseconds
  unsigned long TimeIntVerify;      // Time to perform internal verifying
  unsigned long TimeVerify;         // Time to perform data verifying

  unsigned int MaxDataWrite;        // Max data length when writing (128/256)
  // Microcontroller Signature
  char sig_Vendor;
  char sig_IDCode;
  char sig_ElectricalInfo;
  unsigned int sig_LastAddr;
  char sig_Device[12];
  char sig_FlashBlocks;
} NEC_PARS;

extern NEC_PARS *nec;
extern char is78k0s;


