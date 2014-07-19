/***************************************************************************
                          lowlevel.c  -  description
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

#define extern
#include <sys/io.h>
#undef extern

#include <stdlib.h>
#include <stdio.h>
#include "lowlevel.h"
#include "microdelay.h"

/* begin of reg_pp functions */
struct reg_pp * rpp_new() {
  return (struct reg_pp *) malloc(sizeof(struct reg_pp));
}

struct reg_pp * rpp_init(struct reg_pp * reg,
                         unsigned short base, int offset, int row) {
  reg->base=base;
  reg->offset=offset;
  reg->row=row;
  return reg;
}

void rpp_delete(struct reg_pp * reg) {
  if (reg) free(reg);
}

// set bit
void rpp_set(struct reg_pp * reg) {
  unsigned char data;
  unsigned short addr;
  addr = reg->base + reg->offset;
  data = inb(addr);
  data |= 1<<reg->row;
  outb(data, addr);
  //  printf("set 0x%x=0x%x (%d,%d)\n", addr, data, reg->offset, reg->row);
}

// clear bit
void rpp_clear(struct reg_pp * reg) {
  unsigned char data;
  unsigned short addr;
  addr = reg->base + reg->offset;
  data = inb(addr);
  data &= ~(1<<reg->row);
  outb(data, addr);
  //  printf("clr 0x%x=0x%x (%d,%d)\n", addr, data, reg->offset, reg->row);
}

// complement bit
void rpp_cpl(struct reg_pp * reg) {
  unsigned char data;
  unsigned short addr;
  addr = reg->base + reg->offset;
  data = inb(addr);
  data^=(1<<reg->row);
  outb(data,addr);
}

// read bit
unsigned char rpp_read(struct reg_pp * reg) {
  unsigned char data;
  unsigned short addr;
  addr = reg->base + reg->offset;
  data = inb(addr);
  data >>= reg->row;
  data &= 1;
  return data;
}

// write bit
void rpp_write(struct reg_pp * reg, int val) {
  if (val)
    rpp_set(reg);
  else
    rpp_clear(reg);
}

/* end of reg_pp functions */

/* begin of nec_serial_pp functions */

ASPP * aspp_new() {
  return (ASPP *) malloc(sizeof(ASPP));
}

ASPP * aspp_init(ASPP *aspp, unsigned short base) {
  aspp->SCK=rpp_init(rpp_new(), base, 2, 0);    // strobe
  aspp->MISO=rpp_init(rpp_new(), base, 1, 7);   // busy
  aspp->MOSI=rpp_init(rpp_new(), base, 2, 1);   // autofd
  aspp->RESET=rpp_init(rpp_new(), base, 2, 3);  // slct in
  aspp->VCC=rpp_init(rpp_new(), base, 0, 0);    // d0
  aspp->VPP5=rpp_init(rpp_new(), base, 0, 1);   // d1
  aspp->VPP10=rpp_init(rpp_new(), base, 0, 2);  // d2
  aspp->LED=rpp_init(rpp_new(), base, 0, 3);    // d3
  aspp->BUZZER=rpp_init(rpp_new(), base, 0, 4); // d4
  return aspp;
}

void aspp_delete(ASPP *aspp) {
  rpp_delete(aspp->SCK);
  rpp_delete(aspp->MISO);
  rpp_delete(aspp->MOSI);
  rpp_delete(aspp->RESET);
  rpp_delete(aspp->VCC);
  rpp_delete(aspp->VPP5);
  rpp_delete(aspp->VPP10);
  rpp_delete(aspp->LED);
  rpp_delete(aspp->BUZZER);
  free(aspp);
}

void aspp_SCK(ASPP *aspp, int data) {
  rpp_write(aspp->SCK, data);
}

int aspp_MISO(ASPP *aspp) {
  return rpp_read(aspp->MISO);
}

void aspp_MOSI(ASPP *aspp, int data) {
  rpp_write(aspp->MOSI, data);
}

void aspp_RESET(ASPP *aspp, int data) {
  rpp_write(aspp->RESET, data);
}

void aspp_VCC(ASPP *aspp, int data) {
  rpp_write(aspp->VCC, ! data);
}

void aspp_VPP5(ASPP *aspp, int data) {
  rpp_write(aspp->VPP5, ! data);
}

void aspp_VPP10(ASPP *aspp, int data) {
  rpp_write(aspp->VPP10, data);
}

void aspp_LED(ASPP *aspp, int data) {
  rpp_write(aspp->LED, ! data);
}

void aspp_LEDcpl(ASPP *aspp) {  // complement LED output
  rpp_cpl(aspp->LED);
}

void aspp_BUZZER(ASPP *aspp, int data) {
  rpp_write(aspp->BUZZER, ! data);
}

/* end of nec_serial_pp functions */

