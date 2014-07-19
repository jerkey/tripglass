/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : gio ago  8 17:43:26 CEST 2002
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/io.h>
#include <sys/perm.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nec78k0.h"
#include "microdelay.h"
#include "lowlevel.h"
#include "main.h"

ASPP *bus;  // bus structure (depend of type of interface and parallel port)
unsigned short base=0x378;      // base address of parallel port registers
unsigned int cpuclock=8000;     // target clock in KHz
unsigned char erase=1;          // 1 if device erase is request
unsigned char eraseonly=0;			// 1 if device should not be programmed
unsigned char *code_ptr;          // Programming data used by nec78k_program()
char buffer[BUFFERLEN];        // General purpose buffer
int debug=0;          // command argument used for debugging purposes
char family[32];      // microcontroller family
unsigned char debugmsg=0;        // 1 => print debugging messages
unsigned char vpp_pulses=0;				// Number of pulses on Vdd pin
unsigned int loopsPerMicroSecond=75;	// Used on windows Only (used for 1uS loop)
FILE *hexfile;

void printhelp(char *prgname) {
  printf("%s v%s: Nec 78k0 / 78k0S programming tool\nDeveloped by Paolo Subiaco  http://www.creasol.it\n",PACKAGE,VERSION);
  printf("Usage: %s [OPTIONS] FILE.hex      where option can be:\n",prgname);
  printf("  -h, --help              print this help\n");
  printf("  -b, --base=ADDR         set parallel port address; default 0x%x\n",base);
  printf("  -c, --clock=CLOCK       set target clock in KHz; default %dKHz\n",cpuclock);
  printf("  -D, --debug             enable all debugging messages\n");
  printf("  -e, --eraseonly					erase device only\n");
	printf("  -l loopInstructions     Number of instruction to exec/uS loop (default 75)\n");
  printf("  -n, --noerase           don't erase device before programming\n");
  printf("  -p VPP_Pulses           number of VPP pulses to begin programming\n");
  printf("  -t, --timings           DEBUG only: send clock at 10 KHz and 1 Hz on SCK\n"); 
  printf("Note: this program must be called with ROOT PRIVILEDGES!\n");
}

void printerror(const char *format) {
  nec78k_poweroff(bus); // initialize interface voltages
  aspp_delete(bus);
  fprintf(stderr,format);
  exit(-3);
}

void sendtimings(void) {
  // Send clock at 10 KHz and 1 Hz on SCK output: usable to test usleep and 0x80
  // port speed.
  unsigned int i;

  bus=aspp_init(aspp_new(),base);   // allocate memory for bus structure

  fprintf(stderr,"sendtimings()\n");
  #ifdef HARDWAREMICROSEC
  microdelay_init();
  ioperm(base,3,1);

  fprintf(stderr,"10 KHz using hardware timings...\n");
  for (i=20000;i;i--) {
    aspp_SCK(bus,1);
    microdelay(50);
    aspp_SCK(bus,0);
    microdelay(50);  
  }
  #else
  fprintf(stderr,"10 KHz using software timings()...\n");
  for (i=20000;i;i--) {
    aspp_SCK(bus,1);
    microdelay(50);
    aspp_SCK(bus,0);
    microdelay(50);  
  }
  #endif
  #ifdef HARDWAREMICROSEC
  fprintf(stderr,"1 Hz using hardware timings...\n");
  for (i=5;i;i--) {
    aspp_SCK(bus,1);
    microdelay(500000);
    aspp_SCK(bus,0);
    microdelay(500000);  
  }
  #endif
  fprintf(stderr,"1 Hz using software timings()...\n");
  for (i=5;i;i--) {
    aspp_SCK(bus,1);
    microdelay(500000);
    aspp_SCK(bus,0);
    microdelay(500000);  
  }
}


int main(int argc, char *argv[])
{ int opterr, opt_c;

  #ifdef _LINUX_
  if (geteuid()!=0) {
    fprintf(stderr,"*** ERROR: this program must be called with root priviledges!!\n\n");
    printhelp(argv[0]);
    return -1;
  }
  #endif

  // Parsing options...
  opterr=0;
  while ((opt_c = getopt(argc, argv, "hDtenb:c:d:p:l:")) != -1) {
    switch (opt_c) {
      case 'h': printhelp(argv[0]); return 0;
      case 'D': debugmsg=1;break;
      case 'b': base=(unsigned short)strtoul(optarg, (char **) NULL, 16);break;
      case 'c': cpuclock=(unsigned int)strtoul(optarg, (char **) NULL, 10);break;
      case 'd': debug=(int)strtol(optarg, (char **) NULL, 10);break;
			case 'l': loopsPerMicroSecond=(int)strtol(optarg, (char **) NULL, 10);break;								
      case 'p': vpp_pulses=(int)strtol(optarg, (char **) NULL, 10);
								if (vpp_pulses>15) {
									fprintf(stderr,"*** ERROR: Invalid number of Vpp pulses\n");
									return;
								}
								break;
			case 'e': eraseonly=1;break;
			case 'n': erase=0;break;
      case 't': sendtimings(); return 0;
      
      case '?':
        opterr++;
        if (isprint (optopt))
          fprintf (stderr, "Unknown option '-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character '\\x%x'.\n", optopt);
    }
  }
  if (opterr) {
    printhelp(argv[0]);
    return -1;
  }
  // TODO: getopt_long

  // Now control parameters
  if (cpuclock>=10000 || cpuclock<1000) {
    fprintf(stderr,"*** ERROR setting CPUCLOCK: frequency must set between 1000 and 9999KHz\n\n");
    return -1;
  }
  if (eraseonly==0 && (argc<2 || !(hexfile=fopen(argv[argc-1],"r")))) {
    fprintf(stderr,"*** ERROR: Please specify a valid intel-hex filename\n\n");
    printhelp(argv[0]);
    return -1;
  }

  microdelay_init();    // Initialize IO which permit to obtain a precise 1us delay
  ioperm(base,3,1);

  // Nec
  bus=aspp_init(aspp_new(),base);   // allocate memory for bus structure

startprogramming:	
	nec78k_poweroff(bus); // initialize interface voltages
  usleep(10000);        // wait until poweroff
  aspp_BUZZER(bus,0);   // turn off BUZZER
  aspp_LED(bus,0);      // turn off LED

  nec=(NEC_PARS *)malloc(sizeof(NEC_PARS));
  if (!nec) {
    sprintf(buffer,"%s: Error allocating memory for Nec microcontroller parameters\n",PACKAGE);
    printerror(buffer);
  }
  nec78k_poweron(bus,vpp_pulses);
  usleep(100000); // initialization time
  if (nec78k_sync(bus))
    printerror("Error syncing device!\n");
  if (nec78k_readsignature(bus))    // Read uC signature (uses longest timings to be used with all Nec microcontrollers)
    printerror("Error reading device signature!\n");

  fprintf(stderr,"Info: BaseAddr=0x%x, CpuClk=%dKHz, Vendor=%s, DevName=%s Size=%d\n",base,cpuclock,(nec->sig_Vendor==0x10?"Nec":"Unknown"),nec->sig_Device,nec->sig_LastAddr+1);
  nec78k_Timings();             // Perform timings calculation (device_dependent routine)

  if (debug==2) goto poweroff;

  while (nec78k_setosc(bus)) {
    // printerror("Error setting oscillator frequency!\n");
    nec78k_poweroff(bus);
    usleep(100000);
    nec78k_poweron(bus,vpp_pulses);
    usleep(100000); // initialization time
    nec78k_sync(bus);
  }
  if (debug==3) goto poweroff;
  usleep(10000); // DEBUG: time
  if (nec78k_seterase(bus))
    printerror("Error setting erase time!\n");
  if (debug==4) goto poweroff;

  
	if (erase) {  // if erase==0 => don't spend time performing blank check and erase!
    // perform blank check...
//    fprintf(stderr,"blank check disabled to reduce programming time!\n");
    if (eraseonly==1 || nec78k_blankcheck(bus)) {
      usleep(10000); // DEBUG: time
      if (nec78k_prewrite(bus))
        printerror("Error prewriting device!\n");
      if (debug==5) goto poweroff;
      usleep(10000); // DEBUG: time
      if (nec78k_erase(bus)) {
				// printerror("Error erasing device!\n");
				printf("Error erasing device! Retry from beginning\n");
				goto startprogramming;
			}
      if (debug==6) goto poweroff;
    }
  }

	if (eraseonly) {
		printf("-e switch specified: don't program anything\n");
		goto poweroff;
	}
  // now begin programming
  if (nec78k_program(bus,hexfile,nec->MaxDataWrite))
    printerror("Error writing device!\n");

poweroff:
  if (debugmsg) 
    fprintf(stderr,"DEBUG: enter power down\n");
  nec78k_poweroff(bus); // initialize interface voltages
  aspp_LED(bus,0);
  usleep(100000);
  aspp_RESET(bus,1);  // bring reset high to start program execution

/*
  aspp_BUZZER(bus,1);
  usleep(100000);
  aspp_BUZZER(bus,0);
  usleep(100000);
  aspp_BUZZER(bus,1);
  usleep(100000);
  aspp_BUZZER(bus,0);
  usleep(100000);
  aspp_BUZZER(bus,1);
  usleep(100000);
  aspp_BUZZER(bus,0);
*/
  aspp_delete(bus);
  fclose(hexfile);
  return EXIT_SUCCESS;
}

