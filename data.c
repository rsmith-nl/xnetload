/* $Id: data.c,v 1.1 1999/05/09 16:39:26 rsmith Exp rsmith $
 * ------------------------------------------------------------------------
 * This file is part of xnetload, a program to monitor network traffic,
 * and display it in an X window.
 *
 * Copyright (C) 1997 - 1999  R.F. Smith <rsmith@xs4all.nl>
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
 * Revision 1.1  1999/05/09 16:39:26  rsmith
 * Initial revision
 *
 *
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
 *
 */

#include <syslog.h>		/* for error reporting */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"

/* size of buffer to copy /proc/net/xxx to */
#define BUFSIZE 2048

/********** Global variables **********/
count_t average;		/* average count */
count_t max;			/* maximum count */
int type = 0;			/* What kind of data is gathered */

/********** Static variables **********/
static char *iface_name;	      /* Name of the interface to be queried. */
static count_t last;		         /* Previously read values. */
static float *inarray;    /* Array for receive counts. */
static float *outarray;	/* Array for transmit counts. */
static int where = 0;		      /* Where to gather data from. */
static int numavg;               /* number of samples to average */

/********** Function definitions **********/

/* return values for read_*  */
#define READ_VTYPE_ERR    -6	/* unknown accounting rule type */
#define READ_ALLOC_ERR    -5	/* Not enough memory on the stack */
#define READ_FOPEN_ERR    -4	/* Can't open file */
#define READ_FREAD_ERR    -3	/* Can't read file */
#define READ_IFACE_ERR    -2	/* Interface not found */
#define READ_SCAN_ERR     -1	/* Unknown file layout */
#define READ_BYTES         1	/* Function reads byte counts */
#define READ_PACKETS       2	/* Function reads packet counts */

/* Read the byte or packet count for the network interface  
   named in `iface' from /proc/net/dev, store it in the  
   variable pointed to by `pcnt', if `pcnt' is not NULL. */
static int read_dev(count_t * pcnt, char *iface);

/* Snatched from the 2.0.32 kernel code: 
   /usr/src/linux/include/linux/ip_fw.h line 109 */
#ifndef IP_FW_F_ACCTIN
#define IP_FW_F_ACCTIN  0x1000	/* Account incoming packets only.     */
#endif				/* IP_FW_F_ACCTIN */

#ifndef IP_FW_F_ACCTOUT
#define IP_FW_F_ACCTOUT 0x2000	/* Account outgoing packets only.     */
#endif				/* IP_FW_F_ACCTOUT */

/* Read the byte count for the network interface named in `iface'  
   from /proc/net/ip_acct, store it in the  
   variable pointed to by `pcnt', if `pcnt' is not NULL. */
static int read_ip_acct(count_t * pcnt, char *iface);

/********** Function implementations **********/

void report_error(char *msg)
{
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

int initialize(char *iface, int num_avg, int try_ip_acct)
{
  int r;
  char ipfwstr[80];
  
  /* Initialize global variables. */
  iface_name = iface;
  numavg = num_avg;

  inarray = (float*)calloc(numavg, sizeof(float));
  outarray = (float*)calloc(numavg, sizeof(float));
  if (inarray == 0 || outarray == 0) {
    report_error("Memory allocation failed");
  }

  /* If we want to use ip_accounting, try it. */
  if (try_ip_acct) {
    /* check to see if we are root or setuid root */
    if (geteuid() != (uid_t) 0) {
      report_error("Must run as root to install ip_acct rules");
    }
    /* setup the ip_acct rules via system() */
    /* the "in" rule */
    strcpy(ipfwstr, "/sbin/ipfwadm -A in -i -W ");
    strcpy((ipfwstr + strlen(ipfwstr)), iface);
    if (system(ipfwstr) != 0) {
      report_error("system() call for ipfwadm failed");
    }
    /* the "out" rule */
    strcpy(ipfwstr, "/sbin/ipfwadm -A out -i -W ");
    strcpy((ipfwstr + strlen(ipfwstr)), iface);
    if (system(ipfwstr) != 0) {
      report_error("system() call for ipfwadm failed");
    }
    r = read_ip_acct(&last, iface);
    where = IP_ACCT;
    switch (r) {
    case READ_VTYPE_ERR:
      report_error("Unknown accounting rule type");
      break;
    case READ_ALLOC_ERR:
      report_error("Memory allocation failed");
      break;
    case READ_FOPEN_ERR:
      report_error("Could not open /proc/net/ip_acct");
      break;
    case READ_IFACE_ERR:
      report_error("Interface not found in /proc/net/ip_acct");
      break;
    case READ_SCAN_ERR:
      report_error("Error scanning /proc/net/ip_acct");
      break;
    case READ_BYTES:
      type = BYTES_TYPE;
      break;
    default:
      report_error("Unknown return value from read_ip_acct");
    }
  } else { /* Use proc/net/dev file */
    r = read_dev(&last, iface);
    where = DEV;
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
  }
  return type;
}  

int cleanup(void)
{
  char ipfwstr[80];

  free(inarray);
  free(outarray);
  /* assumption:  'where' and 'iface_name' are set */
  if (where == IP_ACCT) {
    strcpy(ipfwstr, "/sbin/ipfwadm -A in -d -W ");
    strcpy((ipfwstr + strlen(ipfwstr)), iface_name);
    system(ipfwstr);	/* cannot call report_error() - recursion */
    strcpy(ipfwstr, "/sbin/ipfwadm -A out -d -W ");
    strcpy((ipfwstr + strlen(ipfwstr)), iface_name);
    system(ipfwstr);	/* cannot call report_error() - recursion */
  }
  return (0);
}

void update_avg(int seconds)
{
  count_t current;
  count_t diff;
  int i;
  /* Read the data. */
  if (where == IP_ACCT) {
    i = read_ip_acct(&current, iface_name);
  } else {
    i = read_dev(&current, iface_name);
  }
  if (i < 0) {
    if (i == READ_IFACE_ERR) {
      /* Interface has disappeared from the file: we've gone offline. */
      exit(1);
    } else {
      report_error("Terminating");
    }
  }
  /* Quit if invalid number of seconds is given */
  if (seconds <= 0)
    return;
  /* Calculate the difference with the last call */
  diff.in = (current.in - last.in)/seconds;
  diff.out = (current.out - last.out)/seconds;
  /* Update the arrays. */
  memmove(&inarray[1], &inarray[0], (numavg - 1) * sizeof(float));
  inarray[0] = diff.in;
  memmove(&outarray[1], &outarray[0], (numavg - 1) * sizeof(float));
  outarray[0] = diff.out;
  /* Calculate the average */
  average.in = average.out = 0;
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

int read_dev(count_t * pcnt, char *iface)
{
  FILE *f;
  char buf[BUFSIZE];
  int num, retval;
  char *pch;
  unsigned int values[16];
  
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
  pch = strstr(buf, iface);
  if (pch == 0) {
    /* Interface not found. */
    return READ_IFACE_ERR;
  }
  /* Skip the interface name. */
  pch += strlen(iface);
  /* Skip the ":" after the interface name. */
  pch++;

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
  num = sscanf(pch, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
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

int read_ip_acct(count_t * pcnt, char *iface)
{
  FILE *f;
  char buf[BUFSIZE];
  int num, t, len;
  char *pch;
  char *iface2;
  /* Try to open /proc/net/ip_acct for reading. */
  f = fopen("/proc/net/ip_acct", "r");
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
  /* Add a space to the `iface' string, so we can detect the difference
     between e.g. "eth1" and "eth11" */
  len = strlen(iface);
  iface2 = alloca(len + 2);
  if (iface2 == 0)
    return READ_ALLOC_ERR;
  strcpy(iface2, iface);
  iface2[len] = ' ';
  iface2[len + 1] = 0;
  /* Set pointer to start of buffer. */
  pch = &buf[0];
  /* There should be two accounting rules for our interface. */
  for (t = 0; t < 2; t++) {
    unsigned int vtype, v1, v2;
    /* Seek interface string. */
    pch = strstr(pch, iface2);
    if (pch == 0) {
      /* Maybe the accounting rules are not installed? */
      return READ_IFACE_ERR;
    }
    /* Skip interface string and space. */
    pch += len + 1;
    /* Scan for data. */
    num = sscanf(pch, "%*u %x %*u %*u %u %u", &vtype, &v1, &v2);
    if (num != 3) {
      /* Error scanning the string. */
      return READ_SCAN_ERR;
    }
    switch (vtype & (IP_FW_F_ACCTIN | IP_FW_F_ACCTOUT)) {
    case IP_FW_F_ACCTIN:	/* v1 = in count, v2 = in bytes */
      if (pcnt) {
        pcnt->in = (float) v2;
      }
      break;
    case IP_FW_F_ACCTOUT:	/* v1 = out count; v2 = out bytes */
      if (pcnt) {
        pcnt->out = (float) v2;
      }
      break;
    default:
      return READ_VTYPE_ERR;
    }
  }
  return READ_BYTES;
}

