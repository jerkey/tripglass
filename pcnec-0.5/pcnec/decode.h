/***************************************************************************
                          decode.h  -  description
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

#ifndef _DECODE_H
#define _DECODE_H

enum FORMAT {F_UNKNOWN=0, F_INTEL};
int decodeformat(FILE *infile); // Return input file format

// Decode input file (Intel-standard, binary, motorola S*, Xcoff, ....)
int decode(FILE *infile, int format);


int hextobin(char *ptr);        // return the number corresponding to ptr[0..1]
int intel_checksum(char *ptr);  // return 0 if checksum of ptr[] is right


#endif
