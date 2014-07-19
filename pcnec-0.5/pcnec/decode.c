/***************************************************************************
                          decode.c  -  description
                             -------------------
    begin                : Sun Aug 11 2002
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
#include "decode.h"
#include "main.h"
#include "config.h"
#include "nec78k0.h"

// return the hex number expressed by ptr[0],ptr[1] ; negative number if error
int hextobin(char *ptr) {
  int i,j;
  char ch;

  ch=ptr[0];
  if (ch>='0' && ch<='9')
    i=ch-'0';
  else if (ch>='A' && ch<='F')
    i=ch+10-'A';
  else if (ch>='a' && ch<='f')
    i=ch+10-'a';
  else
    return -1;

  ch=ptr[1];
  if (ch>='0' && ch<='9')
    j=ch-'0';
  else if (ch>='A' && ch<='F')
    j=ch+10-'A';
  else if (ch>='a' && ch<='f')
    j=ch+10-'a';
  else
    return -2;
  return (i<<4)+j;
}


int intel_checksum(char *ptr) {
  int checksum;
  int i,j;
  int rlen;

  if (ptr[0]!=':')
    return -1;            // this is not an intelhex format
  rlen=hextobin(ptr+1);// read intelhex record length
  if (rlen<0)
    return -2;            // Error decodifig record length
  checksum=rlen;
  rlen+=5;  // add len,address[2],command,checksum
  if ((ptr[rlen*2+1]!='\n') && (ptr[rlen*2+1]!='\r'))
    return -3;            // Invalid record length

  for (i=1,ptr+=3;i<rlen;i++,ptr+=2) {
    if ((j=hextobin(ptr))<0)
      return -4;
    checksum+=j;
  }
  return checksum&0xff;    // correct checksum must be 0!
}


int decodeformat(FILE *infile) {
  // Decode file format : return 0 if format is unknown
  int errors;

  if (!fgets(buffer,BUFFERLEN,infile))
    printerror("Error reading 1st line of input file!\n");
  if (!strncmp(buffer,":",1)) {
    // it seems to be Intel Format: verify checksum.....
    errors=0;
    if (!intel_checksum(buffer)) {
      while (hextobin(buffer+7)!=0) { // find first command line
        if (!fgets(buffer,BUFFERLEN,infile)) {
          // Error reading file: intelhex record within 0x00 command not found!
          errors++;
          break;
        }
      }
      if (errors==0) {
        fprintf(stderr,"Input file type IntelHex, start address=0x%04x\n",hextobin(buffer+5)+(hextobin(buffer+3)<<8));
        fseek(infile,(-1)*strlen(buffer),SEEK_CUR);
        return F_INTEL;
      }
    }
  }

  // Other format?
  fseek(infile, 0, SEEK_SET);

  printerror("Invalid input file format: is NOT Intel Hex!\n");
  return F_UNKNOWN; // never return anything at this point... printerror terminate program execution.
}

int decode(FILE *infile, int format) {
  //  allocate a block of memory SIZE bytes long, and store it the image of code memory
  unsigned int size;
  unsigned int rec_addr;
  unsigned int rec_len;
  unsigned int rec_cmd;
  unsigned int i,j;

  size=nec->sig_LastAddr+1;
  code_ptr=(unsigned char*)malloc(size);
  if (!code_ptr) {
    sprintf(buffer,"Error allocating memory block [%dk] to store code memory!\n",size/1024);
    printerror(buffer);
  }
  for (i=0;i<size;i++)
    code_ptr[i]=0xff;
  i=0;

  switch (format) {
    case F_INTEL:
      // read buffer for residual data
      while (fgets(buffer,BUFFERLEN,infile)) {
        if (intel_checksum(buffer))
          continue;
        rec_len=hextobin(buffer+1);
        rec_addr=(hextobin(buffer+3)<<8)+hextobin(buffer+5);
        rec_cmd=hextobin(buffer+7);
        if (rec_cmd==0) {
          // data record
          if (rec_addr+rec_len>size)
            printerror("Error: data in hexfile exceed the maximun block size of microcontroller");
          for(j=0;j<rec_len;j++,i++)
            code_ptr[rec_addr+j]=hextobin(buffer+9+2*j);
        }
      }
  }

  fprintf(stderr,"%d bytes of code read from input file\n",i);
  return 0;
}


