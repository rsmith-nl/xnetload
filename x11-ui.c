/* $Id: x11-ui.c,v 1.11 2000/09/26 19:50:31 rsmith Exp rsmith $
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
 * --------------------------------------------------------------------
 * $Log: x11-ui.c,v $
 * Revision 1.11  2000/09/26 19:50:31  rsmith
 * Merged value-prefix patch from Jesper Dahlberg <jesper@swing.campus.luth.se>.
 *
 * Revision 1.10  2000/06/07 16:49:04  rsmith
 * Fixed bug in get_out_value. Fix contributed by
 * Jesper Dahlberg <jesper@swing.campus.luth.se>
 *
 * Revision 1.9  2000/04/14 20:22:12  rsmith
 * Updated the copyright notice for 2000.
 *
 * Revision 1.8  2000/04/14 19:36:20  rsmith
 * Added -h, -? and --help switches.
 * Shortened the values strings.
 * Shortened the interface string.
 *
 * Revision 1.7  2000/02/22 17:51:56  rsmith
 * Updated fallback resources for greater width.
 *
 * Revision 1.6  2000/02/22 17:49:53  rsmith
 * Implements scaling and totals feature. Initial patch by
 * Paul Schilling <pfschill@bigfoot.com>, some changes by rsmith.
 *
 * Revision 1.5  2000/01/01 21:24:56  rsmith
 * Release 1.7.1b3
 *
 * Revision 1.4  1999/12/27 22:17:45  rsmith
 * Added -kb switch and all that goes with that.
 * Pulled out ip-acct & fixed bugs for release 1.7.0b1
 *
 * Revision 1.3  1999/10/06 20:55:38  rsmith
 * Updated online help for -ni option.
 *
 * Revision 1.2  1999/10/06 20:42:53  rsmith
 * Added -ni option.
 *
 * Revision 1.1  1999/05/09 16:40:15  rsmith
 * Initial revision
 *
 *
 *
 * Revision 1.5  1998/06/21 09:58:23  rsmith
 * Changed get_*_value functions to make the graphs diplay log10
 * of the pachet/byte count.
 *
 * Revision 1.4  1998/06/21 09:12:03  rsmith
 * Added IP-accounting rules patch by Tony Mancill.
 *
 * Revision 1.3  1998/06/14 12:24:59  rsmith
 * Changed e-mail address.
 *
 * Revision 1.2  1998/05/04 18:23:27  rsmit06
 * Updated online help.
 *
 * Revision 1.1  1998/04/10 19:52:29  rsmit06
 * Initial revision. Split the code in user interface and
 * data gathering part.
 *
 * Revision 1.2.1.4  1998/03/09 19:26:42  rsmit06
 * - Added fix for IP-aliases from R. Wegmann
 * - Added support for 2.1.xx kernels from Adrian Bridgett
 * - Added support for using IP-accounting by Tony Mancill
 *
 * Revision 1.2.1.3  1997/12/10 21:25:48  rsmit06
 * - Added grips to label and stripchart
 * - Added maximum packet/s.
 *
 * Revision 1.2.1.2  1997/12/06 16:05:42  rsmit06
 * Added `print_help' function.
 * Cast `recavg' and `transavg' to float in sprintf calls.
 *
 * Revision 1.2.1.1  1997/12/06 16:00:26  rsmit06
 * Changed `device' to `interface'.
 *
 * Revision 1.2  1997/12/06 14:26:34  rsmit06
 * - Removed bug in GetPacketCount that makes it fail on large numbers of
 *   packets. Reported by Rik Hemsley <hemsleyr@keyline.co.uk>.
 * - Added StripChart widgets. Changed form Form to Paned constraint widget.
 * - Added command-line arguments & resources.
 *
 * Revision 1.1  1997/12/06 13:25:38  rsmit06
 * The program now responds to the WM_DELETE_WINDOW message by terminating.
 *
 * Revision 1.0  1997/12/06 10:25:25  rsmit06
 * Initial revision
 *
 *
 */

/* X Toolkit include files */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* Include files for Athena widgets */
#include <X11/Xaw/Label.h>
#include <X11/Xaw/StripChart.h>
#include <X11/Xaw/Paned.h>

/* C library include files */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

/* User include files */
#include "data.h"

/* constants */
#define XtNnoValues     "noValues"
#define XtCnoValues     "NoValues"
#define XtNnoCharts     "noCharts"
#define XtCnoCharts     "NoCharts"
#define XtNnoInterface  "noInterface"
#define XtCnoInterface  "NoInterface"
#define XtNinterface    "interface"
#define XtCinterface    "Interface"
#define XtNupd          "upd"
#define XtCupd          "Upd"
#define XtNavg          "avg"
#define XtCavg          "Avg"
#define XtNkilobytes    "kilobytes"
#define XtCkilobytes    "Kilobytes"
#define XtNscale        "scale"
#define XtCscale        "Scale"
#define XtNhelp         "help"
#define XtChelp         "Help"

/* application resource data type */
typedef struct {
  Boolean no_charts;
  Boolean no_values;
  Boolean no_interface;
  Boolean kilobytes;
  Boolean help;
  char *iface;
  int update;
  int avg;
  int scale;
} appdata_t;

/* static variables */
static appdata_t appdata;
static char *in_alt[2] = {
  "in:  %.1f %sB/s (%.1f %sB/s) [%.1f %sB]",
  "in:  %.1f p/s  max: %.1f p/s"
};
static char *in_str = 0;
static char *out_alt[2] = {
  "out:  %.1f %sB/s (%.1f %sB/s) [%.1f %sB]",
  "out:  %1.1f p/s  max: %.1f p/s"
};
static char *out_str = 0;

/*
 * Definition of the prefixes.  A thousand exa bytes will be
 * a sufficient upper limit for quite a few years.  ;-)
 */
static char *prefix[] = {"", "k", "M", "G", "T", "P", "E"};

struct prefixed_value {
  float value;
  char *prefix;
};

/* application resource list */
static XtResource resources[] =
{
    {
      XtNhelp,
      XtChelp,
      XtRBoolean,
      sizeof(Boolean),
      XtOffsetOf(appdata_t, help),
      XtRBoolean,
      (XtPointer) False
    },
    {
      XtNnoValues,
      XtCnoValues,
      XtRBoolean,
      sizeof(Boolean),
      XtOffsetOf(appdata_t, no_values),
      XtRBoolean,
      (XtPointer) False
    },
    {
      XtNnoCharts,
      XtCnoCharts,
      XtRBoolean,
      sizeof(Boolean),
      XtOffsetOf(appdata_t, no_charts),
      XtRBoolean,
      (XtPointer) False
    },
    {
      XtNnoInterface,
      XtCnoInterface,
      XtRBoolean,
      sizeof(Boolean),
      XtOffsetOf(appdata_t, no_interface),
      XtRBoolean,
      (XtPointer) False
    },
    {
      XtNinterface,
      XtCinterface,
      XtRString,
      sizeof(char *),
      XtOffsetOf(appdata_t, iface),
      XtRImmediate,
      (XtPointer) 0
    },
    {
      XtNupd,
      XtCupd,
      XtRInt,
      sizeof(int),
      XtOffsetOf(appdata_t, update),
      XtRImmediate,
      (XtPointer) 1
    },      
    {
      XtNscale,
      XtCscale,
      XtRInt,
      sizeof(int),
      XtOffsetOf(appdata_t, scale),
      XtRImmediate,
      (XtPointer) 1
    },      
    {
      XtNavg,
      XtCavg,
      XtRInt,
      sizeof(int),
      XtOffsetOf(appdata_t, avg),
      XtRImmediate,
      (XtPointer) 5
    },
    {
      XtNkilobytes,
      XtCkilobytes,
      XtRBoolean,
      sizeof(Boolean),
      XtOffsetOf(appdata_t, kilobytes),
      XtRBoolean,
      (XtPointer) False
    }
};

/* command line options */
static XrmOptionDescRec options[] =
{
  {"-?", "*help", XrmoptionNoArg, "True"},
  {"-h", "*help", XrmoptionNoArg, "True"},
  {"--help", "*help", XrmoptionNoArg, "True"},
  {"-if", "*interface", XrmoptionSepArg, NULL},
  {"-interface", "*interface", XrmoptionSepArg, NULL},
  {"-ni", "*noInterface", XrmoptionNoArg, "True"},
  {"-nointerface", "*noInterface", XrmoptionNoArg, "True"},
  {"-nc", "*noCharts", XrmoptionNoArg, "True"},
  {"-nocharts", "*noCharts", XrmoptionNoArg, "True"},
  {"-nv", "*noValues", XrmoptionNoArg, "True"},
  {"-novalues", "*noValues", XrmoptionNoArg, "True"},
  {"-u", "*upd", XrmoptionSepArg, NULL},
  {"-update", "*upd", XrmoptionSepArg, NULL},
  {"-a", "*avg", XrmoptionSepArg, NULL},
  {"-average", "*avg", XrmoptionSepArg, NULL},
  {"-kb", "*kilobytes", XrmoptionNoArg, "True"},
  {"-kilobytes", "*kilobytes", XrmoptionNoArg, "True"},
  {"-s", "*scale", XrmoptionSepArg, NULL},
  {"-scale", "*scale", XrmoptionSepArg, NULL}
};

/* time at which the program was started  */
static time_t starttime;

/* app-context and widgets */
static XtAppContext app_context;
static Widget toplevel, paned, interface, in, out, str_in, str_out;

/* fallback resources */
static String fallback_resources[] =
{
  "*Label*background: LightGray",
  "*Label*width: 280",
  "*StripChart*width: 280",
  "*StripChart*height: 30",
  NULL
};

/* define actions and things you need for that */
static void xclose(Widget w, XEvent * event, String * params, Cardinal * num);
static XtActionsRec actions[] =
{
  {"xclose", (XtActionProc) xclose}
};

/* Used for registering interest in WM_DELETE message */
static String translations = "<Message>WM_PROTOCOLS:xclose()\n";
static Atom wm_protocol;

/* Gives an explanation of the program's arguments */
void
print_help();

/* Procedure to gather and update the data. The parameters are not used.
 * It formats the information from `average' and `max', the elapsed time 
 * since the program was started, and the interface name,
 * and sets these in the labels in the `interface',
 * 'in' and 'out' widgets. It also registers itself as a timeout routine.
 */
void
refresh(XtPointer data, XtIntervalId * id);

/* Get packets in value for str_in widget. Uses the global `average' */
void
get_in_value(Widget w, XtPointer client_data, XtPointer value);

/* Get packets out value for str_out widget. Uses the global `average' */
void
get_out_value(Widget w, XtPointer client_data, XtPointer value);

/* Determines appropriate prefix to use for 'value' */
struct prefixed_value use_prefix(float value);

/* Main procedure. Checks the arguments and aborts in case of error. 
 * Otherwise it creates and maps the widgets and goes into the message
 * processing loop.
 */
int main(int argc, char *argv[])
{
  /* create toplevel widget */
  toplevel = XtVaAppInitialize(&app_context,	/* application context */
                               "XNetload",	/* application class */
                               options,	/* cmd-line option list */
                               XtNumber(options),
                               &argc, argv,	/* cmd-line args */
                               fallback_resources,	/* fallback resources */
                               NULL);		/* end varargs list */
  /* add actions */
  XtAppAddActions(app_context, actions, XtNumber(actions));
  /* Get application resources, put them into appdata. */
  XtVaGetApplicationResources(toplevel,
                              &appdata,
                              resources,
                              XtNumber(resources),
                              NULL);


  /* check for help flags */
  if (appdata.help == True) {
      print_help();
      return 1;
  }

  /* check if interface was given */
  if (appdata.iface == 0) {
    if (argc > 1) {
      appdata.iface = argv[argc - 1];
    } else {
      fprintf(stderr, "xnetload: No network interface specified.\n");
      fflush(stderr);
      print_help();
      return 1;
    }
  }

  /* Check the command line arguments. */
  if (appdata.avg < 1) {
    report_error("Average count must be > 0");
  }
  if (appdata.update < 1) {
    report_error("Update time must be > 0");
  }
  if (appdata.scale < 1) {
    report_error("Scale must be > 1");
  }

  /* Initialize the data gathering process. */
  initialize(appdata.iface, appdata.avg);
  in_str = in_alt[type];
  out_str = out_alt[type];

  /* Get the starting time. */
  time(&starttime);
  /* Create paned widget. */
  paned = XtVaCreateManagedWidget("paned",
                                  panedWidgetClass,	/* class name */
                                  toplevel,	/* parent */
                                  XtNinternalBorderWidth, 0,
                                  NULL);	/* end varargs list */
  /* create label widget, if configured */
  if (appdata.no_interface == False) {
    interface = XtVaCreateManagedWidget("interface",	/* name */
                                        labelWidgetClass,	/* class name */
                                        paned,	/* parent */
                                        XtNjustify, XtJustifyLeft,
                                        XtNborderWidth, 0,
                                        XtNshowGrip, False,
                                        NULL);	/* end varargs list */
  }  
  /* create label widget, if configured */
  if (appdata.no_values == False) {
    in = XtVaCreateManagedWidget("in ",	/* name */
                                 labelWidgetClass,	/* class name */
                                 paned,	/* parent */
                                 XtNborderWidth, 0,
                                 XtNjustify, XtJustifyLeft,
                                 XtNshowGrip, False,
                                 NULL);	/* end varargs list */
  }
  /* create Stripchart widget if configured */
  if (appdata.no_charts == False) {
    str_in = XtVaCreateManagedWidget("str_in",	/* name */
                                     stripChartWidgetClass,		/* class name */
                                     paned,		/* parent */
                                     XtNjumpScroll, 1,
                                     XtNminScale, 1,
                                     XtNupdate, 1,
                                     NULL);		/* end varargs list */

    XtAddCallback(str_in, XtNgetValue, (XtCallbackProc) get_in_value, NULL);
  }
  /* create label widget, if configured */
  if (appdata.no_values == False) {
    out = XtVaCreateManagedWidget("out ",	/* name */
                                  labelWidgetClass,		/* class name */
                                  paned,	/* parent */
                                  XtNborderWidth, 0,
                                  XtNjustify, XtJustifyLeft,
                                  NULL);	/* end varargs list */
  }
  /* create Stripchart widget if configured */
  if (appdata.no_charts == False) {
    str_out = XtVaCreateManagedWidget("str_out",	/* name */
                                      stripChartWidgetClass,	/* class name */
                                      paned,	/* parent */
                                      XtNjumpScroll, 1,
                                      XtNminScale, 1,
                                      XtNupdate, 1,
                                      /*XtNshowGrip, False, */
                                      NULL);	/* end varargs list */
    XtAddCallback(str_out, XtNgetValue, (XtCallbackProc) get_out_value, NULL);
  }
  /* create windows for widgets and map them */
  XtRealizeWidget(toplevel);
  /* register interest in WM_DELETE_WINDOW message */
  wm_protocol = XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False);
  XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel), &wm_protocol, 1);
  XtOverrideTranslations(toplevel, XtParseTranslationTable(translations));
  /* refresh labels and register timer routine */
  refresh(0, 0);
  /* loop for events   */
  XtAppMainLoop(app_context);
  /* normal program end */
  return 0;
}  

void print_help()
{
  fprintf(stderr, "xnetload "VERSION", Copyright (C) 1997-2000 R.F. Smith\n");
  fprintf(stderr, "Usage: xnetload [Xt args] [options] [interface]\n");
  fprintf(stderr,
          "xnetload understands all Xt standard command-line options.\n");
  fprintf(stderr, "See X(1) for details. ");
  fprintf(stderr, "Additional options are as follows:\n");
  fprintf(stderr, "option name       option value\n");
  fprintf(stderr, "-----------       ------------\n");
  fprintf(stderr, "-?. -h, --help    Display this help.\n");
  fprintf(stderr, "-if <name>,\n");
  fprintf(stderr, "-interface <name> Any device from /proc/net/dev.\n");
  fprintf(stderr, "-nc, -nocharts    Don't use charts.\n");
  fprintf(stderr, "-nv, -novalues    Don't display packets/s counts.\n");
  fprintf(stderr, "-ni, -nointerface Don't display `interface' line.\n");
  fprintf(stderr, "-kb, -kilobytes   Display count in kilobytes.\n");
  fprintf(stderr, "-u,  -update      Number of seconds between screen.\n");
  fprintf(stderr, "                  updates. (default is 1).\n");
  fprintf(stderr, "-s,  -scale       Number to scale kilobyte chart by.\n");
  fprintf(stderr, "-a,  -average     Number of samples to average.\n");
  fprintf(stderr, "                  (default is 5).\n");
  
  fprintf(stderr,
          "The network interface to monitor can also be named on the\n");
  fprintf(stderr,
          "command-line without `-if' preceeding it, for compatiblity \n");
  fprintf(stderr, "with previous versions. ");
  fprintf(stderr,
          "However, the value of the `-if' option has precedence.\n");
}

/* Set the value that the StripChart widget str_in should display. */
void get_in_value(Widget w, XtPointer client_data, XtPointer value)
{
  if (appdata.kilobytes) {
    *(double *)value = (double)average.in/(1024*appdata.scale);
  } else {
    if (average.in < 1.0) {
      *(double *)value = (double)0;
    } else {
      *(double *)value = log10(average.in);
    }
  }
}

/* Set the value that the StripChart widget str_out should display. */
void get_out_value(Widget w, XtPointer client_data, XtPointer value)
{
  if (appdata.kilobytes) {
    *(double *)value = (double)average.out/(1024*appdata.scale);
  } else {
    if (average.out < 1.0) {
      *(double *)value = (double)0;
    } else {
      *(double *)value = log10(average.out);
    }
  }
}

void refresh(XtPointer data, XtIntervalId * id)
{
  long hr, min, sec;		/* time vars  */
  struct prefixed_value pval_average, pval_max, pval_total;
  time_t newtime;
  char *dev_str = "%s up: %i:%02i:%02i";
  char buf[128];
  /* read data from /proc/net/dev */
  update_avg(appdata.update);
  /* get new time */
  time(&newtime);
  /* calculate the number of seconds passed since start */
  sec = newtime - starttime;
  if (sec < 0) {
    /* the clock has been reset! */
    starttime = newtime;
    sec = 0;
  }
  min = sec / 60;		/* calculate minutes  */
  sec %= 60;			/* calculate remaining seconds */
  hr = min / 60;
  min %= 60;
  if (appdata.no_interface == False) {
    /* print the data to strings and set label resources */
    sprintf(buf, dev_str, appdata.iface, hr, min, sec);
    XtVaSetValues(interface, XtNlabel, buf, NULL);
  }
  if (appdata.no_values == False) {
    if (type == BYTES_TYPE) {
      pval_average = use_prefix(average.in);
      pval_max = use_prefix(max.in);
      pval_total = use_prefix(total.in);
      sprintf( buf, in_str
             , pval_average.value, pval_average.prefix
             , pval_max.value, pval_max.prefix
             , pval_total.value, pval_total.prefix);
    } else
      sprintf(buf, in_str, average.in, max.in);
    XtVaSetValues(in, XtNlabel, buf, NULL);
    if (type == BYTES_TYPE) {
      pval_average = use_prefix(average.out);
      pval_max = use_prefix(max.out);
      pval_total = use_prefix(total.out);
      sprintf( buf, out_str
             , pval_average.value, pval_average.prefix
             , pval_max.value, pval_max.prefix
             , pval_total.value, pval_total.prefix);
    } else
      sprintf(buf, out_str, average.out, max.out);
    XtVaSetValues(out, XtNlabel, buf, NULL);
  }
  
  /* register the timer again */
  XtAppAddTimeOut(app_context, 1000*appdata.update, refresh, NULL);
}

/* close action */
void xclose(Widget w, XEvent * event, String * params, Cardinal * num)
{
  cleanup();
  exit(0);
}

struct prefixed_value use_prefix(float value)
{
  int i;
  struct prefixed_value pval;

  pval.value = value;
  for (i = 0; pval.value > 1024.0; i++) {
    pval.value /= 1024.0;
  }
  pval.prefix = prefix[i];

  return pval;
}
