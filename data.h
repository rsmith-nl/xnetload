/*  $Id: data.h,v 1.8 2002/07/15 18:12:21 rsmith Exp rsmith $
 * Time-stamp: "2003-03-16 11:26:37 rsmith"
 * --------------------------------------------------------------------
 * This file is part of xnetload, a program to monitor network traffic,
 * and display it in an X window.
 *
 * Copyright (C) 1997 - 2003  R.F. Smith <rsmith@xs4all.nl>
 *
 * You can contact the author at the following address:
 *      email: rsmith@xs4all.nl
 * snail-mail: R.F. Smith
 *             Dr. Hermansweg 36
 *             5624 HR Eindhoven
 *             The Netherlands
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
 * 
 * --------------------------------------------------------------------
 * $Log: data.h,v $
 * Revision 1.8  2002/07/15 18:12:21  rsmith
 * Reformatted with 'indent -kr -i8'.
 *
 * Revision 1.7  2001/04/18 17:41:12  rsmith
 * ZeroOnRequest added to update_avg.
 *
 * Revision 1.6  2000/04/14 20:23:26  rsmith
 * Updated the copyright notice for 2000.
 *
 * Revision 1.5  2000/02/22 17:41:29  rsmith
 * Added `total' count. Patch provided by
 * Paul Schilling <pfschill@bigfoot.com>.
 *
 * Revision 1.4  2000/01/01 21:26:37  rsmith
 * Release 1.7.1b3
 *
 * Revision 1.3  1999/12/27 22:16:45  rsmith
 * Fixed bugs for release 1.7.0b1
 *
 * Revision 1.2  1999/12/27 18:54:22  rsmith
 * Remove references to `where' (for 1.7.0 release).
 *
 * Revision 1.1  1999/05/09 16:38:31  rsmith
 * Put back into RCS.
 *
 * Revision 1.3  1998/06/21 09:55:05  rsmith
 * - Added Tony's patch for IP-acounting rules.
 *
 * Revision 1.2  1998/06/14 12:24:10  rsmith
 * Changed e-mail address.
 *
 * Revision 1.1  1998/04/10 19:53:11  rsmit06
 * Initial revision
 *
 *
 */

#ifndef _DATA_H
#define _DATA_H

/********** Type definitionss **********/

/* contains number of outgoing/incoming packets or bytes */
typedef struct {
	float in;		/* incoming packets/bytes */
	float out;		/* outgoing packets/bytes */
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
