/*  $Id$
 * --------------------------------------------------------------------
 * This file is part of xnetload, a program to monitor network traffic,
 * and display it in an X window.
 *
 * Copyright (C) 1997, 1999  R.F. Smith <rsmith@xs4all.nl>
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
 * $Log:$
 *
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
  float in;    /* incoming packets/bytes */
  float out;   /* outgoing packets/bytes */
} count_t;

/********** Global variables **********/
extern count_t average;    /* average count */
extern count_t max;        /* maximum count */
/* Values for `type' */
#define BYTES_TYPE     1
#define PACKETS_TYPE   2
extern int type;           /* What kind of data is gathered */
/* Values for `where' */
#define IP_ACCT    1
#define DEV        2
extern int where;          /* Where to gather data from. */

/********** Functions **********/

/* Report a fatal error and exit. */
extern void report_error(char *msg);

/* Initialize the data gathering process. Sets the variables `type' and
   `where', for future reference. Returns `type' */
extern int initialize(char *iface, int num_avg, int try_ip_acct);
extern int cleanup(void);

/* Read the new counts and update the `average' and `max'. */
extern void update_avg(int seconds);

#endif /* _DATA_H */
