/*  data.h
 * --------------------------------------------------------------------
 * This file is part of xnetload, a program to monitor network traffic,
 * and display it in an X window.
 *
 * Copyright (C) 1997 - 2003  R.F. Smith <rsmith@xs4all.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _DATA_H
#define _DATA_H

/********** Type definitions **********/

/* contains number of outgoing/incoming packets or bytes */
typedef struct {
	long double in;		/* incoming packets/bytes */
	long double out;	/* outgoing packets/bytes */
} count_t;

/********** Global variables **********/
extern count_t average;		/* average count */
extern count_t max;		/* maximum count */
extern count_t total;		/* total count   */

/* Values for `type' */
#define BYTES_TYPE      0
#define PACKETS_TYPE    1
extern int type;		/* What kind of data is gathered */

/********** Functions **********/

extern void report_error(char *msg);
extern int initialize(char *iface, int num_avg);
extern int cleanup(void);
extern void update_avg(int seconds, int zero_on_reset);

#endif				/* _DATA_H */
