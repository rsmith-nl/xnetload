/* $Id: data.c,v 1.8 2000/04/14 20:22:50 rsmith Exp rsmith $
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

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>		/* for error reporting */
#include <unistd.h>
#include "data.h"

/* size of buffer to copy /proc/net/xxx to */
#define BUFSIZE 2048

/********** Global variables **********/
int type = 0;			/* What kind of data is gathered */
count_t average = {(float)0,(float)0};   /* average count */
count_t max = {(float)0,(float)0};       /* maximum count */
count_t total = {(float)0,(float)0};     /* total count   */

/********** Static variables **********/
static char *iface_name;   /* Name of the interface to be queried. */
static count_t last = {(float)0,(float)0};	/* Previously read values. */
static float *inarray;     /* Array for receive counts. */
static float *outarray;	   /* Array for transmit counts. */
static int numavg;         /* number of samples to average */

/********** Function definitions **********/

/* return values for read_*  */
#define READ_ALLOC_ERR    -5	/* Not enough memory on the stack */
#define READ_FOPEN_ERR    -4	/* Can't open file */
#define READ_FREAD_ERR    -3	/* Can't read file */
#define READ_IFACE_ERR    -2	/* Interface not found */
#define READ_SCAN_ERR     -1	/* Unknown file layout */
#define READ_BYTES         1	/* Function reads byte counts */
#define READ_PACKETS       2	/* Function reads packet counts */

/* Find the needle in the haystack. The needle must be preceded by
 * whitespace and folowed by a semicolon. Returns a pointer to a location
 * behind the needle. */
static char *afterstr(char *haystack, const char *needle);

/* Read the byte or packet count for the network interface  
   named in `iface' from /proc/net/dev, store it in the  
   variable pointed to by `pcnt', if `pcnt' is not NULL. */
static int read_dev(count_t * pcnt, char *iface);

/********** Function implementations **********/

void report_error(char *msg)
{
  assert (msg!=0);
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

int initialize(char *iface, int num_avg/*  , int kb */)
{
  int r;

  assert (iface!=0);
  assert (num_avg > 0);
  
  /* Initialize global variables. */
  iface_name = iface;
  numavg = num_avg;
  average.in = (float)0;
  average.out = (float)0;
  total.in  = (float)0;
  total.out = (float)0;

  inarray = (float*)malloc(numavg*sizeof(float));
  outarray = (float*)malloc(numavg*sizeof(float));
  if (inarray == 0 || outarray == 0) {
    report_error("Memory allocation failed");
  }
  for (r = 0; r < numavg; r++) {
    inarray[r] = (float)0;
    outarray[r] = (float)0;
  }

  r = read_dev(&last, iface);
  switch (r) {
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

int cleanup(void)
{
  free(inarray);
  free(outarray);
  return (0);
}

void update_avg(int seconds, int zeroOnReset )
{
  count_t current = {0.0,0.0};
  count_t diff;
  int i;
  static int index;

  assert (seconds > 0);

  /* Quit if invalid number of seconds is given */
/*    if (seconds <= 0) */
/*      return; */
  /* Read the data. */
  i = read_dev(&current, iface_name);
  switch (i) {
  case READ_FOPEN_ERR:
    report_error("Could not open /proc/net/dev");
    break;
  case READ_IFACE_ERR:
    exit(0);
    break;
  case READ_SCAN_ERR:
    report_error("Error scanning /proc/net/dev");
    break;
  }

  /* Try to detect counter overrun. current and last are floating point
   * values, but current is filled from a %u, and so capped to UINT_MAX */
  if (current.in < last.in) {
    if ( zeroOnReset ) {
      diff.in  = 0;
      total.in = 0;
      max.in   = 0;
    }
    printf("xnetload warning: incoming counter overrun.\n");
  } else {
    diff.in = current.in - last.in;
  }
  if (current.out < last.out) {
    if ( zeroOnReset ) {
      diff.out  = 0;
      total.out = 0;
      max.out   = 0;
    }
    printf("xnetload warning: outgoing counter overrun.\n");
  } else {
    diff.out = current.out - last.out;
  }

  /* Add that to the total */
  total.in  += diff.in;
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
  average.in = average.out = (float)0;
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

char *afterstr(char *haystack, const char *needle)
{
  char *pch;
  char *ptr = haystack;

  assert (haystack != 0);
  assert (needle != 0);

  do {
    pch = strstr (ptr, needle);
    if (pch) {
      /* Check for leading whitespace, if not begin of */
      if (pch != haystack && isspace((int)(*(pch-1))) == 0) {
          continue;
      }
      /* Check for closing ':' */
      pch += strlen (needle);
      if (*pch == ':') {
        pch++;
        break; /* Found it; break out of the loop. */
      }
      /* Set new start of buffer if not found. */
      ptr = pch;
    }
  } while (pch);
  return pch;
}

int read_dev(count_t *pcnt, char *iface)
{
  FILE *f;
  char buf[BUFSIZE];
  int num, retval;
  char *pch;
  unsigned long int values[16];

  assert (pcnt != 0);
  assert (iface != 0);

  /* Try to open /proc/net/dev for reading. */
  f = fopen("/proc/net/dev", "r");
  if (f == 0) {
    /* Unable to open file. */
    return READ_FOPEN_ERR;
  } 
  /* Read the file into a buffer. */
  num = fread(buf, 1, BUFSIZE - 1, f);
  /* Check for read errors. */
  if (ferror(f)) {
    fclose(f);
    return READ_FREAD_ERR;
  }
  /* Terminate the buffer with 0. */
  buf[num] = 0;
  /* Close the file. */
  fclose(f);

  /* Seek the interface we're looking for. */
/*    pch = strstr(buf, iface); */
  pch = afterstr(buf, iface);
  if (pch == 0) {
    /* Interface not found. */
    return READ_IFACE_ERR;
  }
  /* Skip the interface name. */
/*    pch += strlen(iface); */
  /* Skip the ":" after the interface name. */
/*    pch++; */

  /* There are different layouts for /proc/net/dev:
   * there are several fields for received and transmitted bytes
   * on 2.0.32:          on 2.1.>90            on 2.1.<90
   *              RECEIVE  (and 2.2.x)        
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
           "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
               &values[0], &values[1], &values[2], &values[3],
               &values[4], &values[5], &values[6], &values[7],
               &values[8], &values[9], &values[10], &values[11],
               &values[12], &values[13], &values[14], &values[15]);
  switch (num) {
  case 11:			/* 2.0.xx kernel, can only read packets. */
    retval = READ_PACKETS;
    if (pcnt) {
      pcnt->in = (float) values[0];
      pcnt->out = (float) values[5];
    }
    break;
  case 14:			/* 2.1.<90 kernel, can read byte counts. */
    retval = READ_BYTES;
    if (pcnt) {
      pcnt->in = (float) values[0];
      pcnt->out = (float) values[6];
    }
    break;
  case 16:			/* 2.1.90+ kernel, can read byte counts. */
    retval = READ_BYTES;
    if (pcnt) {
      pcnt->in = (float) values[0];
      pcnt->out = (float) values[8];
    }
    break;
  default:
    /* Unknown layout/scan error. */
    return READ_SCAN_ERR;
  }

  /* probe went OK */
  return retval;
}
