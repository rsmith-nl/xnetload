/* $Id: data.c,v 1.13 2002/07/15 18:11:37 rsmith Exp $
 * ------------------------------------------------------------------------
 * This file is part of xnetload, a program to monitor network traffic,
 * and display it in an X window.
 *
 * Copyright (C) 1997 - 2000  R.F. Smith <rsmith@xs4all.nl>
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
 * ------------------------------------------------------------------------
 * $Log: data.c,v $
 * Revision 1.13  2002/07/15 18:11:37  rsmith
 * Reformatted with 'indent -kr -i8'.
 * Added structured comments.
 *
 * Revision 1.12  2001/06/26 13:22:43  rsmith
 * Remove overflow messages (pointed out by adrian.bridgett@iname.com).
 *
 * Revision 1.11  2001/06/02 10:45:48  rsmith
 * Added 'debug' macro. Added dynamic memory allocation for read
 * buffer. Removed old commented-out code.
 *
 * Revision 1.10  2001/04/21 07:33:03  rsmith
 * Forgotten credits:
 * Zeroonreset patch by William Burrow <aa126@fan.nb.ca>
 * Bug in detecting interfaces discovered by Sietse Visser
 * <sietse.visser@sysman.nl>
 *
 * Revision 1.9  2001/04/18 17:43:54  rsmith
 * ZeroOnRequest added to update_avg.
 * Added a function to search for the exact interface name.
 * Added assertions to aid in debugging.
 *
 * Revision 1.8  2000/04/14 20:22:50  rsmith
 * Updated the copyright notice for 2000.
 *
 * Revision 1.7  2000/04/14 19:34:04  rsmith
 * Fixed overflow diff count bug pointed out by Adrian Bridgett
 *
 * Revision 1.6  2000/02/22 18:09:25  rsmith
 * Changed the calculation of the `total' count.
 *
 * Revision 1.5  2000/02/22 17:47:37  rsmith
 * Implements the `total' count. Initial patch by
 * Paul Schilling <pfschill@bigfoot.com>, update by rsmith.
 *
 * Revision 1.4  2000/01/01 21:26:05  rsmith
 * Release 1.7.1b3
 *
 * Revision 1.3  1999/12/27 22:16:23  rsmith
 * Pulled out ip-acct & fixed bugs for release 1.7.0b1
 *
 * Revision 1.2  1999/06/09 18:40:29  rsmith
 * Enlarged the file buffer for /proc/net/xxx to 2048 bytes.
 *
 * Revision 1.1  1999/05/09 16:39:26  rsmith
 * Initial revision
 *
 * Revision 1.6  1998/06/21 09:56:44  rsmith
 * - Added Tony's patch for emitting IP-accounting rules.
 *
 * Revision 1.5  1998/06/14 12:23:32  rsmith
 * Changed e-mail address.
 *
 * Revision 1.4  1998/05/24 19:55:07  rsmit06
 * Fixed a string overrun bug in read_ip_acct.
 * Thanks to Don May <d.may@computer.org>, who reported it.
 *
 * Revision 1.3  1998/05/06 21:19:17  rsmit06
 * Changed update_avg to make the program die if the interface disappears from
 * the /proc file. Braced the IP_FW_* defines in an #ifndef/#endif, so they
 * can be easily set from a makefile.
 *
 * Revision 1.2  1998/05/04 18:56:39  rsmit06
 * Changed the scanning of /proc/net/ip_acct, according to the kernel code.
 *
 * Revision 1.1  1998/04/10 19:53:32  rsmit06
 * Initial revision
 *
 */

#include <assert.h>		/* for error checking */
#include <ctype.h>		/* for isspace() */
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>		/* for error reporting */
#include <unistd.h>
#include "data.h"

/* initial size of buffer to copy /proc/net/xxx to */
#define BUFSIZE 1024

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef NDEBUG
/* __FUNCTION__ is a GCC feature. */
#undef debug
#define debug(TXT) fprintf(stderr,"%s %s(): %s\n",__FILE__,__FUNCTION__,TXT)
#else
#undef debug
#define debug(TXT) (void)0
#endif				/* NDEBUG */

/********** Global variables **********/
int type = 0;			/* What kind of data is gathered */
count_t average = { (double) 0, (double) 0 };	/* average count */
count_t max = { (double) 0, (double) 0 };	/* maximum count */
count_t total = { (double) 0, (double) 0 };	/* total count   */

/********** Static variables **********/
static char *iface_name;	/* Name of the interface to be queried. */
static count_t last = { (double) 0, (double) 0 };	/* Previously read values. */
static double *inarray;		/* Array for receive counts. */
static double *outarray;		/* Array for transmit counts. */
static int numavg;		/* number of samples to average */

/********** Function definitions **********/

/* return values for read_*  */
#define READ_MEM_ERR      -6	/* Not enough memory on the heap */
#define READ_FOPEN_ERR    -4	/* Can't open file */
#define READ_FREAD_ERR    -3	/* Can't read file */
#define READ_IFACE_ERR    -2	/* Interface not found */
#define READ_SCAN_ERR     -1	/* Unknown file layout */
#define READ_BYTES         1	/* Function reads byte counts */
#define READ_PACKETS       2	/* Function reads packet counts */

/* Find the needle in the haystack. The needle must be preceded by
 * whitespace and folowed by a semicolon. Returns a pointer to a location
 * behind the needle. */
static char *findif(char *buffer, const char *interf);

/* Read the byte or packet count for the network interface  
   named in `iface' from /proc/net/dev, store it in the  
   variable pointed to by `pcnt', if `pcnt' is not NULL. */
static int read_dev(count_t * pcnt, char *iface);

/********** Function implementations **********/

/**
 * report_error - report an error and terminate the program
 * @msg: message explaining the error
 *
 * Logs an error to the system logfile, writes it stderr and terminates the
 * program. 
 **/
void report_error(char *msg)
{
	assert(msg != NULL);
	/* cleanup for ip_acct */
	cleanup();
	/* Open connection to system logfile. */
	openlog("xnetload", LOG_CONS, LOG_USER);
	/* Write a log message. */
	syslog(LOG_ERR, "%s.\n", msg);
	/* Close the logfile. */
	closelog();
	/* Write the message to stderr. */
	fprintf(stderr, "xnetload: %s.\n", msg);
	/* Terminate the program. */
	exit(1);
}

/**
 * initialize - initializes global variables
 * @iface: the interface to monitor
 * @num_avg: the number of seconds to average
 * 
 * Initializes the global variables, and allocates memory for the arrays of
 * recorded values.
 **/
int initialize(char *iface, int num_avg)
{
	int r;

	assert(iface != NULL);
	assert(num_avg > 0);

	/* Initialize global variables. */
	iface_name = iface;
	numavg = num_avg;
	average.in = (double) 0;
	average.out = (double) 0;
	total.in = (double) 0;
	total.out = (double) 0;

	inarray = (double *) malloc(numavg * sizeof(double));
	outarray = (double *) malloc(numavg * sizeof(double));
	if (inarray == 0 || outarray == 0) {
		report_error("Memory allocation failed");
	}
	for (r = 0; r < numavg; r++) {
		inarray[r] = (double) 0;
		outarray[r] = (double) 0;
	}

	r = read_dev(&last, iface);
	switch (r) {
	case READ_MEM_ERR:
		report_error("Not enough memory to read /proc/net/dev");
		break;
	case READ_FOPEN_ERR:
		report_error("Could not open /proc/net/dev");
		break;
	case READ_IFACE_ERR:
		report_error("Interface not found in /proc/net/dev");
		break;
	case READ_SCAN_ERR:
		report_error("Error scanning /proc/net/dev");
		break;
	case READ_BYTES:
		type = BYTES_TYPE;
		break;
	case READ_PACKETS:
		type = PACKETS_TYPE;
		break;
	default:
		report_error("Unknown return value from read_dev");
	}
	return type;
}

/**
 * cleanup - perfoms cleanup actions before the program is halted
 *
 * Frees the arrays that were allocated at startup.
 **/
int cleanup(void)
{
	free(inarray);
	free(outarray);
	return (0);
}

/**
 * update_avg - read new data and update the global variables
 * @seconds: number of seconds since last invocation
 * @zero_on_reset: should the totals be zeroed on a reset
 *
 * Reads the new numbers, recalculates the global variables and updates
 * them, so the interface can display the new values.
 **/
void update_avg(int seconds, int zero_on_reset)
{
	count_t current = { 0.0, 0.0 };
	count_t diff;
	int i;
	static int index;

	if (seconds < 0)
		seconds = -seconds;

	/* Read the data. */
	i = read_dev(&current, iface_name);
	switch (i) {
	case READ_FOPEN_ERR:
		report_error("Could not open /proc/net/dev");
		break;
	case READ_IFACE_ERR:
		exit(0);
		break;
	case READ_MEM_ERR:
		report_error("Not enough memory to read /proc/net/dev");
		break;
	case READ_SCAN_ERR:
		report_error("Error scanning /proc/net/dev");
		break;
	}

	/* Try to detect counter overrun. current and last are floating
	 * point values, but current is filled from a %u, and so capped to
	 * UINT_MAX */
	if (current.in < last.in) {	/* wraparound or broken connection */
		if (zero_on_reset || average.in == 0) {	/* prob. wraparound */
			diff.in = total.in = max.in = 0;
		}
	} else {
		diff.in = current.in - last.in;
	}
	if (current.out < last.out) {
		if (zero_on_reset || average.out == 0) {
			diff.out = total.out = max.out = 0;
		}
	} else {
		diff.out = current.out - last.out;
	}

	/* Add that to the total */
	total.in += diff.in;
	total.out += diff.out;

	/* Calculate the difference per update */
	diff.in /= seconds;
	diff.out /= seconds;
	/* Update the arrays. */
	inarray[index] = diff.in;
	outarray[index++] = diff.out;
	if (index == numavg) {
		index = 0;
	}

	/* Calculate the average */
	average.in = average.out = (double) 0;
	for (i = 0; i < numavg; i++) {
		average.in += inarray[i];
		average.out += outarray[i];
	}
	average.in /= numavg;
	average.out /= numavg;

	/* Update the maximum. */
	if (average.in > max.in)
		max.in = average.in;
	if (average.out > max.out)
		max.out = average.out;

	/* Store current count for further reference. */
	memcpy(&last, &current, sizeof(count_t));
}

/**
 * findif - finds the desired interface in the buffer
 * @buffer: the text to scan
 * @if: the interface to find
 *
 * Find the correct interface in the buffer. The interface string can be
 * preceeded by whitespace, and must be followed by a colon. Returns a
 * pointer to te location behind the colon, or NULL if the interface can't
 * be located.
 **/
char *findif(char *buffer, const char *interf){
	char *pch = 0;
	char *ptr = buffer;

	assert(buffer != NULL);
	assert(interf != NULL);

	do {
		pch = strstr(ptr, interf);
		if (pch) {
			/* Check for leading whitespace, if not begin of */
			if (pch != buffer
			    && isspace((int) (*(pch - 1))) == 0) {
				pch += strlen(interf);
				ptr = pch;
				continue;
			}
			/* Check for closing ':' */
			pch += strlen(interf);
			if (*pch == ':') {
				pch++;
				break;	/* Found it; break out of the loop. */
			}
			/* Set new start of buffer if not found. */
			ptr = pch;
		}
	} while (pch);
	return pch;
}

/**
 * read_dev - reads the values for the selected @iface.
 * @pcnt: location where the read values are written to
 * @iface: the interface to scan for.
 *
 * 
 **/
int read_dev(count_t * pcnt, char *iface)
{
	FILE *f;
	static int bufsize = BUFSIZE;
	static char *buf = 0;
	char *newbuf;
	int num, retval;
	char *pch;
	unsigned long long int values[16];

	assert(pcnt != NULL);
	assert(iface != NULL);

	/* create buffer if it doesn't exist */
	if (buf == 0) {
		buf = malloc(bufsize);
		if (buf == 0)
			return READ_MEM_ERR;
	}

	/* Try to open /proc/net/dev for reading. */
	f = fopen("/proc/net/dev", "r");
	if (f == 0) {
		/* Unable to open file. */
		return READ_FOPEN_ERR;
	}
	/* Read the file into a buffer. */
      read_again:
	num = fread(buf, 1, bufsize - 1, f);
	if (ferror(f)) {	/* check for read errors */
		fclose(f);
		return READ_FREAD_ERR;
	} else if (!feof(f)) {	/* check if we need a larger buffer */
		bufsize *= 2;
		newbuf = realloc(buf, bufsize);
		if (newbuf == 0) {
			bufsize /= 2;
			fclose(f);
			return READ_MEM_ERR;
		}
		buf = newbuf;
		rewind(f);
		debug("buffer enlarged, read again");
		goto read_again;
	} else if (num < bufsize / 2 - 1) {
		newbuf = realloc(buf, bufsize / 2);
		if (newbuf) {
			buf = newbuf;
			bufsize /= 2;
		}
	}

	/* Terminate the buffer with 0. */
	buf[num] = 0;
	/* Close the file. */
	fclose(f);

	/* Seek the interface we're looking for. */
	pch = findif(buf, iface);
	if (pch == 0) {
		/* Interface not found. */
		return READ_IFACE_ERR;
	}
	/* There are different layouts for /proc/net/dev:
	 * there are several fields for received and transmitted bytes
	 * on 2.0.32:            on 2.1.>90         on 2.1.<90
	 *              RECEIVE  (and 2.2.x, 2.4.x)        
	 *  > 1 packets         >  1 bytes          > 1 bytes
	 *    2 errs               2 packets          2 packets
	 *    3 drop               3 errs             3 errs
	 *    4 fifo               4 drop             4 drop
	 *    5 frame              5 fifo             5 fifo
	 *                         6 frame            6 frame
	 *                         7 compressed
	 *                         8  multicast
	 *              TRANSMIT
	 *  > 6 packets         >  9 bytes          > 7 bytes
	 *    7 errs              10 packets          8 packets
	 *    8 drop              11 errs             9 errs
	 *    9 fifo              12 drop            10 drop
	 *   10 colls             13 fifo            11 fifo
	 *   11 carrier           14 colls           12 colls
	 *                        15 carrier         13 carrier
	 *                        16 compressed      14 multicast
	 */
	num = sscanf(pch,
		     "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
		     &values[0], &values[1], &values[2], &values[3],
		     &values[4], &values[5], &values[6], &values[7],
		     &values[8], &values[9], &values[10], &values[11],
		     &values[12], &values[13], &values[14], &values[15]);
	switch (num) {
	case 11:		/* 2.0.xx kernel, can only read packets. */
		retval = READ_PACKETS;
		if (pcnt) {
			pcnt->in = (double) values[0];
			pcnt->out = (double) values[5];
		}
		break;
	case 14:		/* 2.1.<90 kernel, can read byte counts. */
		retval = READ_BYTES;
		if (pcnt) {
			pcnt->in = (double) values[0];
			pcnt->out = (double) values[6];
		}
		break;
	case 16:		/* 2.1.90+, 2.2.x or 2.4.x kernel, can read byte counts. */
		retval = READ_BYTES;
		if (pcnt) {
			pcnt->in = (double) values[0];
			pcnt->out = (double) values[8];
		}
		break;
	default:
		/* Unknown layout/scan error. */
		return READ_SCAN_ERR;
	}

	/* probe went OK */
	return retval;
}
