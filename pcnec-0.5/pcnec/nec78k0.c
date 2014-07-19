/***************************************************************************
                          nec78k0.c  -  description
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "microdelay.h"
#include "lowlevel.h"
#include "main.h"
#include "decode.h"
#include "config.h"
#include "nec78k0.h"


NEC_PARS *nec;  // Nec parameters initialized by nec_timings()
char is78k0s=0; // 1 if micro is 78k0s family

void nec_spiwrite(ASPP *bus, unsigned char data) {
  // write 8 bit to 3-wire serial interface at half speed (500Kbps)
  int i;

  for (i=0;i<8;i++) {
    aspp_SCK(bus,0);
    aspp_MOSI(bus,data&0x80);
    microdelay(1);
    aspp_SCK(bus,1);
    microdelay(3);
    data<<=1;
  }
}


unsigned char nec_spiread(ASPP *bus) {
  // read 8 bit from 3-wire serial interface at half speed (500Kbps)
  int i;
  unsigned char data;

  for (i=0;i<8;i++) {
    data<<=1;
    aspp_SCK(bus,0);
    microdelay(1);
    aspp_SCK(bus,1);
    microdelay(1);
    data|=aspp_MISO(bus);
  }
  return data;
}


int nec78k_readsignature(ASPP *bus) {
  // Read uC signature and store it in nec structure
  unsigned char status;
  unsigned int addr;
  int i;
  unsigned int delayComAck=1040000/cpuclock+300;
  unsigned int delayAckData=230000/cpuclock+100;
  unsigned int delayDataData=360000/cpuclock+100;

  if (debugmsg)  fprintf(stderr,"DEBUG: read device signature...\n");
   
  aspp_LEDcpl(bus); // complement LED output
  for (i=0;i<16;i++) {
    microdelay(delayComAck);  // wait before start this procedure
    nec_spiwrite(bus,0xC0);
    microdelay(delayComAck);
    status=nec_spiread(bus);
    microdelay(delayAckData);
    if (status==0x3c)
      break;
  }
  if (i==16) {
    if (debugmsg) fprintf(stderr,"Error sending READSIGNATURE command to device\n");
    return -1;
  }
  // Read Signature.... TODO: odd parity controlling
  nec->sig_Vendor=nec_spiread(bus)&0x7f;  // Vendor Code
  microdelay(delayDataData);
  nec->sig_IDCode=nec_spiread(bus)&0x7f;  // ID Code
  microdelay(delayDataData);
  nec->sig_ElectricalInfo=nec_spiread(bus)&0x7f;  // Electrical Information
  microdelay(delayDataData);
  addr=nec_spiread(bus)&0xff;
  microdelay(delayDataData);
  addr|=(nec_spiread(bus)&0xff)<<7;
  microdelay(delayDataData);
  addr|=(nec_spiread(bus)&0xff)<<14;
  microdelay(delayDataData);
  nec->sig_LastAddr=addr&0xffff;
  for (i=0;i<10;i++) {
    nec->sig_Device[i]=nec_spiread(bus)&0x7f;
    microdelay(delayDataData);
  }
  nec->sig_Device[i]=0;
  microdelay(delayDataData);
  nec->sig_FlashBlocks=nec_spiread(bus)&0x7f;
  if (debugmsg) fprintf(stderr,"DEBUG: Flashblocks=0x%x (must be zero)\n",nec->sig_FlashBlocks);
  microdelay(delayDataData);
  if ((status=nec_spiread(bus))!=0x3c) {
    if (debugmsg) {
      fprintf(stderr,"DEBUG: READSIGNATURE: Error reading ACK after signature: got 0x%x\n",status);
    }                                                                            
//    return -2;  // Received NACK: exit
  }          

  // Ok, now control Vendor and extract Family name and Device, codesize, ....
  if (nec->sig_Vendor != 0x10) {
    fprintf(stderr,"Warning, this is not a Nec microcontroller: Vendor=0x%02x !\n",nec->sig_Vendor);
  }
  if (!strncmp(nec->sig_Device,"D78F",4)) {
    // Flash microcontroller
    if (nec->sig_Device[4]=='9') {
      // 78k/0S
      strcpy(family,"Nec 78k/0S");
    } else if (!strncmp(nec->sig_Device+4,"00",2)) {
      // 78k/0
      strcpy(family,"Nec 78k/0");
    } else {
      // Unknown
      strcpy(family,"Unknown");
    }
    fprintf(stderr,"Recognized %s microcontroller: device name=\"%s\"\n",family,nec->sig_Device);
    return 0;
  }

  fprintf(stderr,"This seems to be NOT a Nec Flash microcontroller:\n  vendor=0x%02x, device name=%s\n",nec->sig_Vendor,nec->sig_Device);
  return 1;
}

void nec78k_Timings() {
  int alpha;

  if (cpuclock<=1280)
    alpha=64;
  else if (cpuclock<=2560)
    alpha=128;
  else if (cpuclock<=5120)
    alpha=256;
  else
    alpha=512;

  if (!strcmp(family,"Nec 78k/0")) { // Nec 78k/0
    if (debugmsg) fprintf(stderr,"DEBUG: Loading Nec 78k/0 timings...\n");
    nec->TimeComAck=900000/cpuclock*3/2;
    nec->TimeAckCom=170000/cpuclock*3/2;
    nec->TimeAckData=230000/cpuclock*3/2;
    nec->TimeDataData=300000/cpuclock+20;
    nec->TimeDataAck=350000/cpuclock*3/2;
    nec->TimeFrqCalc=2200000/cpuclock*2;
    nec->TimeEraseCalc=1200000/cpuclock*2;
    nec->TimePreWrite=((230+alpha)*(nec->sig_LastAddr+1)/cpuclock+1)*1000;
		nec->TimeErase=(NEC_ERASETIME+(690000/cpuclock*(nec->sig_LastAddr+1)));
    nec->ClkWrite=1010;
    nec->TimeWrite=(((nec->ClkWrite+alpha)+2*alpha)*1000/cpuclock+1);
    nec->TimeBlankCheck=(690*(nec->sig_LastAddr+1)/cpuclock+100)*1000/4;
    nec->TimeIntVerify=(840*(nec->sig_LastAddr+1)/cpuclock+1)*1000/4;
    nec->TimeVerify=(258600/cpuclock+1)*1000/5;
    nec->MaxDataWrite=256;
  } else if (!strcmp(family,"Nec 78k/0S")) { // Nec 78k/0S
    if (debugmsg) fprintf(stderr,"DEBUG: Loading Nec 78k/0S timings...\n");
    nec->TimeComAck=1040000/cpuclock*3/2;
    nec->TimeAckCom=210000/cpuclock*3/2;
    nec->TimeAckData=190000/cpuclock*3/2;
    nec->TimeDataData=360000/cpuclock+20;
    nec->TimeDataAck=320000/cpuclock*3/2;
    nec->TimeEraseCalc=20000000/cpuclock*4/3;
    nec->TimeFrqCalc=31600000/cpuclock*2;
    nec->TimePreWrite=((216+alpha)*(nec->sig_LastAddr+1)/cpuclock+1)*1000;
		nec->TimeErase=(NEC_ERASETIME+(175000/cpuclock*(nec->sig_LastAddr+1)));
    nec->ClkWrite=275;
    nec->TimeWrite=(((nec->ClkWrite+alpha)+2*alpha)*1000/cpuclock+1);
    nec->TimeBlankCheck=(175000/cpuclock*(nec->sig_LastAddr+1)+100)/4;
    nec->TimeIntVerify=(230000/cpuclock*(nec->sig_LastAddr+1)+1)/4;
    nec->TimeVerify=(29400000/cpuclock+1)/5;
    nec->MaxDataWrite=128;
  } else {
    printerror("Timings/Parameters not initialized!! Unknown family\n");
  }
}


void nec78k_poweroff(ASPP *bus) {
  // Init the interface (with the correct voltages)
  printf("Power off...\n");
  aspp_RESET(bus,0);
  aspp_SCK(bus,0);
  aspp_VPP10(bus,0);
  aspp_VPP5(bus,0);
  aspp_VCC(bus,0);
  aspp_MOSI(bus,0);
}

void nec78k_poweron(ASPP *bus,unsigned char vpp_pulses) {
  // Enable power supply to Nec 78k0 and 78k0S
	aspp_RESET(bus,0);
  printf("Turn on power supply...\n");
  aspp_VCC(bus,1);
  aspp_LED(bus,1);  // turn on LED
  microdelay(100000);  // Let external reset turn off pull-down
  aspp_SCK(bus,1);
  aspp_VPP5(bus,1);
  aspp_VPP10(bus,1);
  microdelay(5000);
  aspp_RESET(bus,1);
	microdelay(500);	// wait until RESET become 5V
  microdelay(1500);	// wait 2mS from reset==1 and first Vpp pulse
  // Transmit the proper number of Vpp pulses
	while (vpp_pulses>0) {
		aspp_VPP10(bus,0);
		microdelay(30);	// must be 30mS
		aspp_VPP10(bus,1);
		microdelay(30);
		vpp_pulses--;
	}
	if (vpp_pulses) 
		printf("Sent %d Vpp pulse%s to begin communication\n",vpp_pulses,vpp_pulses==1?"":"s");
}


int nec78k_sync(ASPP *bus) {
  // send reset command to syncronyze communication
  int retries;
  unsigned int delayComAck=1240000/cpuclock+40;

  printf("Syncing device...\n");
  aspp_LEDcpl(bus); // complement LED output
  for (retries=0;retries<16; retries++) {
    nec_spiwrite(bus,0);
    microdelay(delayComAck);
    if (nec_spiread(bus)==0x3c)
      break;
    microdelay(delayComAck);
  }
  microdelay(delayComAck);
  return (retries==16); // return 1 if i got acknowledge (ERROR)
}

int nec78k_setosc(ASPP *bus) {
  // set target oscillator frequency
  int retries;
  unsigned char status;

  printf("Set oscillator frequency...\n");
  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    microdelay(50000);  //DEBUG: 1000        
    nec_spiwrite(bus,0x90);
    microdelay(nec->TimeComAck); 
    if ((status=nec_spiread(bus))!=0x3c) {
      if (debugmsg) fprintf(stderr,"DEBUG: Sent SETOSC command: got NACK 0x%x\n",status);
      continue;                  
    } 
    microdelay(nec->TimeAckData);
    nec_spiwrite(bus,(cpuclock/1000)%10);
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,(cpuclock/100)%10);
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,(cpuclock/10)%10);
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,4);
    microdelay(nec->TimeFrqCalc);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckData);
    if (status==0x3c) 
      break;
  }                   
  return (retries==16);
}

int nec78k_seterase(ASPP *bus) {
  // set target erase time
  int retries;

  printf("Set erase time...\n");
  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    nec_spiwrite(bus,0x95);
    microdelay(nec->TimeComAck);
    if (nec_spiread(bus)==0x3c) {
      break;
    }
  }
  if (retries==16) {
    if (debugmsg) fprintf(stderr,"Error sending SETERASE command\n");
    return -1;
  }
  microdelay(nec->TimeAckData);
  for (retries=0;retries<16;retries++) {
    nec_spiwrite(bus,((NEC_ERASETIME/1000000)%10));
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,((NEC_ERASETIME/100000)%10));
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,((NEC_ERASETIME/10000)%10));
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,1);
    microdelay(nec->TimeEraseCalc);
    if (nec_spiread(bus)==0x3c)
      break;
  }
  return (retries==16);
}


int nec78k_prewrite(ASPP *bus) {
  // start prewriting (write 00 to all flash)
  int retries;
  unsigned char status;

  printf("Prewrite device...\n");

  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    nec_spiwrite(bus,0x48);
    microdelay(nec->TimeComAck);
    if (nec_spiread(bus)==0x3c)
      break;
  }
  if (retries==16)
    return -1;
  for (retries=0;retries<16;retries++) {
    if (debugmsg) fprintf(stderr,"prewrite: wait for prewritetime (%luuS)\n",nec->TimePreWrite);
    microdelay(nec->TimePreWrite);
    nec_spiwrite(bus,0x70); // Read status
    microdelay(nec->TimeComAck);
    if (nec_spiread(bus)!=0x3c)
      continue;
    microdelay(nec->TimeAckData);
    status=nec_spiread(bus);
    microdelay(nec->TimeDataData);
    if ((nec_spiread(bus)==0x3c && (status&0x40)==0 && (status&0x4)==0))
      break;  // Ok, prewrite success
  }
  return (retries==16);
}


int nec78k_erase(ASPP *bus) {
  // start erasing flash
  int retries;
  unsigned char status;
  time_t start=time(NULL);  // store start time in seconds

  printf("Erase device...\n");
  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  while (1) {
    if ((time(NULL)-start)>60)
      return -1;
    usleep(1000);
    if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] sending 0x20 command...\n",time(NULL)-start);
    nec_spiwrite(bus,0x20); // Erase command
    microdelay(nec->TimeComAck);
    if (nec_spiread(bus)!=0x3c) // Get command ack
      continue;
    if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] wait for erase time (%d)...\n",(time(NULL)-start),nec->TimeErase);
    microdelay(nec->TimeErase);    // wait erase time
    for (retries=0;retries<16;retries++) {
      // blank check
      if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] waiting BlankCheckTime (%lduS)..\n",time(NULL)-start,nec->TimeBlankCheck);
      microdelay(nec->TimeBlankCheck);   // wait blank check time
      if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] sending 0x70 command...\n",time(NULL)-start);
      nec_spiwrite(bus,0x70);
      microdelay(nec->TimeComAck);
      status=nec_spiread(bus);
      microdelay(nec->TimeAckData);
      if (status!=0x3C) {
        if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] first ACK: NO\n",time(NULL)-start);
        continue;
      }
      status=nec_spiread(bus);// status
      microdelay(nec->TimeDataAck);
      nec_spiread(bus);       // get ACK
      if (status&0x90) {
        if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] status=0x%02x\n",time(NULL)-start,status);
        continue;       // erasing in progress
      } else {
        if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] status=0x%02x\n",time(NULL)-start,status);
        break;
      }
    }
    if ((status&0x09)==0) // neither blank error nor erase error: exit
      break;
    else if ((time(NULL)-start)>60) {
      if (debugmsg) fprintf(stderr,"DEBUG: ERASE: [%02lu] timeout (>60seconds). Status=0x%02x\n",time(NULL)-start,status);       
      return -1;
    } else if ((status&0x08)==8) {
			// erase error... cannot recover
			if (debugmsg) fprintf(stderr,"DEBUG: ERASE: Erase error flag==1: Cannot recover\n");
			return -2;
		}
	}
  return 0;
}


int nec78k_write(ASPP *bus, unsigned int addr, unsigned char *data, int len) {
  // start writing flash
  int retries,i;
  unsigned long delayWrite;
  unsigned char status;
  time_t start=time(NULL);

  if (debugmsg) fprintf(stderr,"DEBUG: write device...\n");

  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] send command 0x40\n",time(NULL)-start);
    nec_spiwrite(bus,0x40);
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckData);
    if (status==0x3c)
      break;
  }
  if (retries==16)
    return -1;
  if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] write %d data at address 0x%08x\n",time(NULL)-start,len,addr);
  nec_spiwrite(bus,(unsigned char)((addr>>16)&0xff)); // write address...
  microdelay(nec->TimeDataData);
  nec_spiwrite(bus,(unsigned char)((addr>>8)&0xff));
  microdelay(nec->TimeDataData);
  nec_spiwrite(bus,(unsigned char)(addr&0xff));
  microdelay(nec->TimeDataData);
  nec_spiwrite(bus,(unsigned char)(len&0xff));    // write len
  for (i=0;i<len;i++) {   // write data....
    if (debugmsg) {
      if ((i%4)==0)
        fprintf(stderr,".");
    }
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,data[i]);
  }
  if (debugmsg) fprintf(stderr,"\n");
  microdelay(nec->TimeDataAck);
  status=nec_spiread(bus);  // read ack: what can i do if NACK?
  delayWrite=nec->TimeWrite+(nec->ClkWrite*len)*1000/cpuclock+10;
  for (retries=1;retries<=16;retries++) {
    if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] wait for write time (%luuS)\n",time(NULL)-start,delayWrite);
    microdelay(delayWrite); // Empiric formula similar to the given by Nec Flashprogramming Datasheet
    if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] send 0x70 command (status request)\n",time(NULL)-start);
    nec_spiwrite(bus,0x70); //Request status
    microdelay(nec->TimeComAck);
    nec_spiread(bus);
    microdelay(nec->TimeAckData);
    status=nec_spiread(bus);  // status
    microdelay(nec->TimeDataAck);
    nec_spiread(bus);
    if (status&0x40) {
      if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] write in progress (status=0x%02x)\n",time(NULL)-start,status);
      continue;       // write in progress....
    } else if (status&0x04) {
      if (debugmsg) fprintf(stderr,"DEBUG: WRITE: [%02lu] write error (status=0x%02x)\n",time(NULL)-start,status);
      return -1;      // write error
    } else
      break;
  }
  return (retries==17);
}


int nec78k_cwrite(ASPP *bus, unsigned char *data, int len) {
  // continuous flash writing
  int retries,i;
  unsigned long delayWrite;
  unsigned char status;
  time_t start=time(NULL);

nec78k_cwrite_top:
  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    nec_spiwrite(bus,0x44);
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckData);
    if (status==0x3c)
      break;
  }
  if (retries==16)
    return -1;
  if (debugmsg)  fprintf(stderr,"DEBUG: CWRITE: [%02lu] continuous write %d data\n",time(NULL)-start,len);
  for (i=0;i<len;i++) {   // write data....
    if (debugmsg) {
      if ((i%4)==0)
        fprintf(stderr,".");
    }
    microdelay(nec->TimeDataData);
    nec_spiwrite(bus,data[i]);
  }
  if (debugmsg) fprintf(stderr,"\n");
  microdelay(nec->TimeDataAck);
  status=nec_spiread(bus);  // read ack: what can i do if NACK?
  if (status!=0x3c) {
    if (debugmsg) fprintf(stderr,"DEBUG: CWRITE: data was written to the SPI, status=0x%02x. Restart cwrite()\n",status);
    microdelay(1000);
    goto nec78k_cwrite_top;
  }

  delayWrite=nec->TimeWrite+(nec->ClkWrite*len)*1000/cpuclock+10;
  for (retries=1;retries<=16;retries++) {
    if (debugmsg) fprintf(stderr,"DEBUG: CWRITE: [%02lu] wait for write time (%luus)\n",time(NULL)-start,delayWrite);
    microdelay(delayWrite); // Empiric formula similar to the given by Nec Flashprogramming Datasheet
    nec_spiwrite(bus,0x70); //Request status
    microdelay(nec->TimeComAck);
    nec_spiread(bus);
    microdelay(nec->TimeAckData);
    status=nec_spiread(bus);  // status
    microdelay(nec->TimeDataAck);
    nec_spiread(bus);
    if (status&0x40) {
      if (debugmsg) fprintf(stderr,"DEBUG: CWRITE: [%02lu] continuous write in progress (status=0x%02x)\n",time(NULL)-start,status);
      continue;       // write in progress....
    } else if (status&0x04) {
      if (debugmsg) fprintf(stderr,"DEBUG: CWRITE: [%02lu] continuous write error (status=0x%02x)\n",time(NULL)-start,status);
      return -1;      // write error
    } else
      break;
  }
  return (retries==17);
}

int nec78k_blankcheck(ASPP *bus) {
  // start erasing flash
  int retries;
  unsigned char status;

  printf("Performing blanck check...\n");
  aspp_LEDcpl(bus); // complement LED output
  for (retries=0;retries<16;retries++) {
    microdelay(nec->TimeAckCom);  // wait before start this procedure
    nec_spiwrite(bus,0x30); // Erase command
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    if (status==0x3c) // Get command ack
      break;
  }
  if (retries==16) {
    fprintf(stderr,"Error performing blank Check\n");
    return -1;  }
  while (1) {
    microdelay(nec->TimeBlankCheck);    // wait 1/8 blanckcheck time
    nec_spiwrite(bus,0x70);
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckData);
    if (status!=0x3C)
        continue;
    status=nec_spiread(bus);// status
    microdelay(nec->TimeDataAck);
    nec_spiread(bus);       // get ACK
    microdelay(nec->TimeAckCom);
    if (status&0x10) {
      if (debugmsg) fprintf(stderr,"DEBUG: blank check in progress....\n");
      continue;       // erasing in progress
    } else {
      if (status&0x01) {
        if (debugmsg) fprintf(stderr,"DEBUG: Device not blank!\n");
        return 1;
      } else {
        if (debugmsg) fprintf(stderr,"DEBUG: Device is blanked\n");
        return 0;
      }
    }
  }
}

int nec78k_intverify (ASPP *bus) {
  // internal verify
  int retries;
  unsigned char status;

  printf("Internal_verifying device...\n");
  aspp_LEDcpl(bus); // complement LED output
  microdelay(nec->TimeAckCom);  // wait before start this procedure
  for (retries=0;retries<16;retries++) {
    nec_spiwrite(bus,0x18);
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckCom);
    if (status==0x3c)
      break;
  }
  if (retries==16)
    return -1;
  for (retries=1;retries<=16;retries++) {
    if (debugmsg) fprintf(stderr,"DEBUG: INTVERIFY: wait for intverify time (%luus)\n",nec->TimeIntVerify);
    microdelay(nec->TimeIntVerify); // Wait for internal verify process
    nec_spiwrite(bus,0x70); //Request status
    microdelay(nec->TimeComAck);
    nec_spiread(bus);
    microdelay(nec->TimeAckData);
    status=nec_spiread(bus);  // status
    microdelay(nec->TimeDataAck);
    nec_spiread(bus);
    if (status&0x20)
      continue;       // internal verify in progress....
    else if (status&0x02)
      return -1;      // internal verify error
    else
      break;
  }
  return (retries==16);
}

int nec78k_verify (ASPP *bus,unsigned char *data, unsigned int last_addr, int len) {
  // verify programmed data; return the incremented address where an error was found, 0 if success.
  int retries;
  unsigned char status;
  int i,j;

  printf("Verifying programmed data...\n");
  aspp_LEDcpl(bus); // complement LED output
  for (retries=0;retries<16;retries++) {
    microdelay(nec->TimeAckCom);  // wait before start this procedure
    nec_spiwrite(bus,0x11);
    microdelay(nec->TimeComAck);
    status=nec_spiread(bus);
    microdelay(nec->TimeAckData);
    if (status==0x3c)
      break;
  }
  if (retries==16) {
    if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Retry exceeded sending VERIFY command\n");
    return 1;
  }
  for (i=0;i<last_addr;i+=len) {
    if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Sending %d bytes at address 0x%08x ...\n",len,i);
    for (j=0;j<len;j++) { // send len bytes
      nec_spiwrite(bus,data[i+j]);
      microdelay(nec->TimeDataData);
    }
    microdelay(nec->TimeDataAck);
    status=nec_spiread(bus);
    if (status!=0x3c) {
      if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Received 0x%02x instead of ACK\n",status);
      return i+1;
    }
    for (retries=1;retries<=16;retries++) {
      if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: wait for verify time (%luus)\n",nec->TimeVerify);
      microdelay(nec->TimeVerify); // Wait for internal verify process
      nec_spiwrite(bus,0x70); //Request status
      microdelay(nec->TimeComAck);
      nec_spiread(bus);
      microdelay(nec->TimeAckData);
      status=nec_spiread(bus);  // status
      microdelay(nec->TimeDataAck);
      nec_spiread(bus);
      if (status&0x20)
        continue;       // verify in progress....
      else if (status&0x02) {
        if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Data incorrect!\n");
        return i+1;      // verify error
      } else
        break;
    }
    if (retries==16) {
      if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Error verifying: retry exceeded!\n");
      return i;
    } else {
      microdelay(nec->TimeAckData);
      if (debugmsg) fprintf(stderr,"DEBUG: VERIFY: Ok, status=0x%02x\n",status);
    }
  }
  return 0;
}



int nec78k_program(ASPP *bus, FILE *infile, int maxlen) {
  // main writing routine, which read hexfile, organize data in block of len bytes and
  // write to flash using write and continuous write routines. Skip 0xFF data, when possible!
  int format;
  unsigned int i,j,k,m;
  int continuous=0;
  unsigned int data_addr=0xffff;
  unsigned int size=nec->sig_LastAddr+1;
  unsigned int last_addr=0;

  format=decodeformat(infile);  // read format of input file (Hex, bin, ....)
  decode(infile,format);        // allocate a block of memory where it store codes

  for (i=0,j=0;j<size;) {
    usleep(100);             // wait time before two successive writing
    for (;j<size && code_ptr[j]==0xff;j++); // skip initial data if 0xff
    if (data_addr!=j)
      continuous=0;
    else
      continuous=1;
    data_addr=j;
    if (data_addr==size)
      break;
    for(k=0;k<maxlen && j<size;k++,j++) {
      if (code_ptr[j]==0xff) {
        for(m=1;m<MAXUNPGMDATA && code_ptr[j+m]==0xff;m++);
        if (m==MAXUNPGMDATA) // too much bytes==0xff => start writing this block
          break;
      }
    }

    fprintf(stderr,"Program %s%d bytes of data from address 0x%08x\n",(continuous==1?"continuously ":""),k,data_addr);
    last_addr=data_addr+k;
    if (continuous && k==maxlen) {
      if (nec78k_cwrite(bus,code_ptr+data_addr,k)) // continue write data from previous address
      { sprintf(buffer,"Error continuous_writing data at start_address 0x%08x !\n",data_addr);
        printerror(buffer); // print error and exit program
      }
    }
    else {
      if (nec78k_write(bus,data_addr,code_ptr+data_addr,k))  // write block from new addr
      { sprintf(buffer,"Error writing data at start_address 0x%08x !\n",data_addr);
        printerror(buffer); // print error and exit program
      }
    }
    i+=k;         // count number of bytes programmed
    data_addr+=k; // update data_addr of new block
  }

  fprintf(stderr,"INFO: internal verify disabled\n");

/*
  if (debugmsg) fprintf(stderr,"DEBUG: PROGRAM: Write finish; Start Internal Verify\n");
  if (nec78k_intverify(bus))
    printerror("Internal verify error!\n");
*/
  if (debugmsg) fprintf(stderr,"DEBUG: PROGRAM: Start Verify....\n");
  microdelay(1000);
  if ((k=nec78k_verify(bus,code_ptr,last_addr,maxlen))) {
    sprintf(buffer,"Error verifying programmed data, at start_address 0x%08x !\n",k-1);
    perror(buffer);
  } else
    fprintf(stderr,"\n*** Microcontroller successfully programmed/verified! ***\n\n");


  return 0;
}



