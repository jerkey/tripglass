/*
Copyright (C) 2000-2001 José Roberto B. de A. Monteiro <jrm@autsens.com>
                        and Pedro Zorzenon Neto <pzn@autsens.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
 From http://www.linuxdoc.org/HOWTO/mini/IO-Port-Programming.html
 an outb to port 0x80 gives a delay of aprox 1microsec independent
 of your machine type/clock.
 See also documentation in asm/io.h
*/

#include "../config.h"

#include "main.h"
#include "microdelay.h"
#include <unistd.h>

#ifdef HARDWAREMICROSEC
#include <sys/io.h>
#endif

int old_port_value;

#ifdef HARDWAREMICROSEC
void microdelay_init() {
    // permition to write in port 0x80
    // nothing is suposed to use this port
    // however, we will write the same value we read...
    ioperm(0x80,1,1);		
    old_port_value=inb(0x80);	
    return;
}

void microdelay(unsigned int microsec) {
	microsec-=(microsec>>2);	// take care of the loop CPU needings
  while (microsec) {
	 	outb(old_port_value,0x80);
		microsec--;
  }
}

#else
// Use usleep to wait for microsec microseconds.

void microdelay_init() {
	// Do nothing
	return;
}

//#define LOOPSPERUS 75
void microdelay(unsigned int microsec) {
	if (microsec>10000) {
		usleep(microsec);
	} else {
		int i;
		while (microsec) {
			for (i=loopsPerMicroSecond;i;i--);
			microsec--;
		}
	}
}
#endif






