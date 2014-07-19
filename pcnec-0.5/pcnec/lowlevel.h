/***************************************************************************
                          necpp.h  -  description
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
#ifndef _LOWLEVEL_H
#define _LOWLEVEL_H

/*
  This object represents a single bit in parallel port
*/

struct reg_pp {
  unsigned short base;
  int offset;
  int row;
};

struct reg_pp * rpp_new();
struct reg_pp * rpp_init(struct reg_pp * reg,
                         unsigned short base, int offset, int row);
void rpp_delete(struct reg_pp * reg);

void rpp_set(struct reg_pp * reg);
void rpp_clear(struct reg_pp * reg);
void rpp_cpl(struct reg_pp * reg);
void rpp_write(struct reg_pp * reg, int data);
unsigned char rpp_read(struct reg_pp * reg);

/*
  Object that contains all bits of control for a parallel port
*/
struct nec_serial_pp {
  struct reg_pp *SCK, *MOSI, *MISO, *RESET, *VCC, *VPP5, *VPP10, *LED, *BUZZER;
};

typedef struct nec_serial_pp ASPP;

ASPP * aspp_new();
ASPP * aspp_init(ASPP *aspp, unsigned short base);
void aspp_delete(ASPP *aspp);

void aspp_SCK(ASPP *aspp, int data);
void aspp_MOSI(ASPP *aspp, int data);
int aspp_MISO(ASPP *aspp);
void aspp_RESET(ASPP *aspp, int data);
void aspp_VCC(ASPP *aspp, int data);
void aspp_VPP5(ASPP *aspp, int data);
void aspp_VPP10(ASPP *aspp, int data);
void aspp_LED(ASPP *aspp, int data);
void aspp_LEDcpl(ASPP *aspp);
void aspp_BUZZER(ASPP *aspp, int data);

#endif

