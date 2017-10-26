/* x11-ui.c
 * ------------------------------------------------------------------------
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
#define XtNzeroOnReset	"zeroOnReset"
#define XtCzeroOnReset	"ZeroOnReset"


/**
 * struct _appdata - contains the arguments given to the program
 * @no_charts: true if no chart widgets should be shown
 * @no_values: true if no value widgets should be shown
 * @no_interface: true if the name of the interface should not be shown
 * @kilobytes: true if the display should be in kilobytes instead of log10
 * @zeroonreset: should the counter be reset if the interface goes down
 * @help: show online help instead of running the program
 * @iface: any device from /proc/net/dev to monitor
 * @update: number of seconds between screen updates
 * @avg: number of samples to average before displaying them
 * @scale: number to scale kilobyte chart by
 * 
 * Contains the options given to the program, or the built-in defaults if
 * an option is not set.
 **/
struct _appdata {
	Boolean no_charts;
	Boolean no_values;
	Boolean no_interface;
	Boolean kilobytes;
	Boolean zeroonreset;
	Boolean help;
	char *iface;
	int update;
	int avg;
	int scale;
};

/* static variables */
static struct _appdata appdata;
static char *in_alt[2] = {
	"in:  %.1f %sb/s (%.1f %sb/s) [%.1f %sB]",
	"in:  %.1f p/s  max: %.1f p/s"
};
static char *in_str = 0;
static char *out_alt[2] = {
	"out:  %.1f %sb/s (%.1f %sb/s) [%.1f %sB]",
	"out:  %1.1f p/s  max: %.1f p/s"
};
static char *out_str = 0;

/*
 * Definition of the prefixes.  Exbibytes is a sufficient upper prefix
 * since the counters are in 64 bit format and can thus count up to
 * just short of 16 exbibytes.
 */
static char *prefix_si[] = { "", "k", "M", "G", "T", "P", "E" };
static char *prefix_iec[] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei" };

struct prefixed_value {
	float value;
	char *prefix;
};

/* application resource list */
static XtResource resources[] = {
	{
	 XtNhelp,
	 XtChelp,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, help),
	 XtRBoolean,
	 (XtPointer) False},
	{
	 XtNnoValues,
	 XtCnoValues,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, no_values),
	 XtRBoolean,
	 (XtPointer) False},
	{
	 XtNnoCharts,
	 XtCnoCharts,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, no_charts),
	 XtRBoolean,
	 (XtPointer) False},
	{
	 XtNnoInterface,
	 XtCnoInterface,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, no_interface),
	 XtRBoolean,
	 (XtPointer) False},
	{
	 XtNinterface,
	 XtCinterface,
	 XtRString,
	 sizeof(char *),
	 XtOffsetOf(struct _appdata, iface),
	 XtRImmediate,
	 (XtPointer) 0},
	{
	 XtNupd,
	 XtCupd,
	 XtRInt,
	 sizeof(int),
	 XtOffsetOf(struct _appdata, update),
	 XtRImmediate,
	 (XtPointer) 1},
	{
	 XtNscale,
	 XtCscale,
	 XtRInt,
	 sizeof(int),
	 XtOffsetOf(struct _appdata, scale),
	 XtRImmediate,
	 (XtPointer) 1},
	{
	 XtNavg,
	 XtCavg,
	 XtRInt,
	 sizeof(int),
	 XtOffsetOf(struct _appdata, avg),
	 XtRImmediate,
	 (XtPointer) 5},
	{
	 XtNkilobytes,
	 XtCkilobytes,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, kilobytes),
	 XtRBoolean,
	 (XtPointer) False},
	{
	 XtNzeroOnReset,
	 XtCzeroOnReset,
	 XtRBoolean,
	 sizeof(Boolean),
	 XtOffsetOf(struct _appdata, zeroonreset),
	 XtRBoolean,
	 (XtPointer) False}
};

/* command line options */
static XrmOptionDescRec options[] = {
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
	{"-scale", "*scale", XrmoptionSepArg, NULL},
	{"-zr", "*zeroOnReset", XrmoptionNoArg, "True"},
	{"-zeroonreset", "*zeroOnReset", XrmoptionNoArg, "True"}
};

/* time at which the program was started  */
static time_t starttime;

/* app-context and widgets */
static XtAppContext app_context;
static Widget toplevel, paned, interface, in, out, str_in, str_out;

/* fallback resources */
static String fallback_resources[] = {
	"*Label*background: LightGray",
	"*Label*width: 280",
	"*StripChart*width: 280",
	"*StripChart*height: 30",
	NULL
};

/* Used for registering interest in WM_DELETE message */
static void xclose(Widget w, XEvent * event, String * params,
		   Cardinal * num);
static XtActionsRec actions[] = {
	{"xclose", (XtActionProc) xclose}
};
static String translations = "<Message>WM_PROTOCOLS:xclose()\n";
static Atom wm_protocol;

/* helper functions */
void print_help();
void refresh(XtPointer data, XtIntervalId * id);
void get_in_value(Widget w, XtPointer client_data, XtPointer value);
void get_out_value(Widget w, XtPointer client_data, XtPointer value);
struct prefixed_value use_prefix(float value, bool iec);

/**
 * main - entry point for xnetload
 * @argc: number of command-line arguments
 * @argv: array of command-line argument strings
 * 
 * Parses the command-line arguments and acts on them. Then initializes the
 * data gathering process, and creates the interface widgets. Finally it
 * turns control over to the Xt main loop.
 **/
int main(int argc, char *argv[])
{
	/* create toplevel widget */
	toplevel = XtVaAppInitialize(&app_context,
				     "XNetload",
				     options,
				     XtNumber(options), &argc, argv,
				     fallback_resources,
				     NULL);
	/* add actions */
	XtAppAddActions(app_context, actions, XtNumber(actions));
	/* Get application resources, put them into appdata. */
	XtVaGetApplicationResources(toplevel,
				    &appdata,
				    resources, XtNumber(resources), 
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
			fprintf(stderr,
				"xnetload: No network interface specified.\n");
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
	paned = XtVaCreateManagedWidget("paned", panedWidgetClass,
					toplevel,
					XtNinternalBorderWidth, 0, NULL);
	/* create label widget, if configured */
	if (appdata.no_interface == False) {
		interface = XtVaCreateManagedWidget("interface",
						    labelWidgetClass,
						    paned,
						    XtNjustify, XtJustifyLeft, 
						    XtNborderWidth, 0, 
						    XtNshowGrip, False, NULL);
	}
	/* create label widget, if configured */
	if (appdata.no_values == False) {
		in = XtVaCreateManagedWidget("in ",
					     labelWidgetClass,
					     paned,
					     XtNborderWidth, 0, 
					     XtNjustify, XtJustifyLeft, 
					     XtNshowGrip, False, NULL);
	}
	/* create Stripchart widget if configured */
	if (appdata.no_charts == False) {
		str_in = XtVaCreateManagedWidget("str_in",
						 stripChartWidgetClass,
						 paned,
						 XtNjumpScroll, 1, 
						 XtNminScale, 1, 
						 XtNupdate, appdata.update, 
						 NULL);

		XtAddCallback(str_in, XtNgetValue,
			      (XtCallbackProc) get_in_value, NULL);
	}
	/* create label widget, if configured */
	if (appdata.no_values == False) {
		out = XtVaCreateManagedWidget("out ",
					      labelWidgetClass,
					      paned,
					      XtNborderWidth, 0, 
					      XtNjustify, XtJustifyLeft, NULL);
	}
	/* create Stripchart widget if configured */
	if (appdata.no_charts == False) {
		str_out = XtVaCreateManagedWidget("str_out",
						  stripChartWidgetClass,
						  paned,
						  XtNjumpScroll, 1,
						  XtNminScale, 1,
						  XtNupdate, appdata.update,
						  NULL);
		XtAddCallback(str_out, XtNgetValue,
			      (XtCallbackProc) get_out_value, NULL);
	}
	/* create windows for widgets and map them */
	XtRealizeWidget(toplevel);
	/* register interest in WM_DELETE_WINDOW message */
	wm_protocol =
	    XInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False);
	XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel),
			&wm_protocol, 1);
	XtOverrideTranslations(toplevel,
			       XtParseTranslationTable(translations));
	/* refresh labels and register timer routine */
	refresh(0, 0);
	/* loop for events   */
	XtAppMainLoop(app_context);
	/* normal program end */
	return 0;
}

/**
 * print_help - prints a help message to stderr
 *
 * Prints a help message describing the program's options to stderr.
 **/
void print_help()
{
	fprintf(stderr,
		"xnetload " VERSION
		", Copyright (C) 1997-2002 R.F. Smith <rsmith@xs4all.nl>\n");
	fprintf(stderr,
		"Usage: xnetload [Xt args] [options] [interface]\n");
	fprintf(stderr,
		"xnetload understands all Xt standard command-line options.\n");
	fprintf(stderr, "See X(1) for details. ");
	fprintf(stderr, "Additional options are as follows:\n");
	fprintf(stderr, "option name       option value\n");
	fprintf(stderr, "-----------       ------------\n");
	fprintf(stderr, "-?. -h, --help    Display this help.\n");
	fprintf(stderr, "-if <name>,\n");
	fprintf(stderr,
		"-interface <name> Any device from /proc/net/dev.\n");
	fprintf(stderr, "-nc, -nocharts    Don't use charts.\n");
	fprintf(stderr,
		"-nv, -novalues    Don't display packets/s counts.\n");
	fprintf(stderr,
		"-ni, -nointerface Don't display `interface' line.\n");
	fprintf(stderr, "-kb, -kilobytes   Display count in kilobytes.\n");
	fprintf(stderr,
		"-u,  -update      Number of seconds between screen updates. (default is 1).\n");
	fprintf(stderr,
		"-s,  -scale       Number to scale kilobyte chart by.\n");
	fprintf(stderr,
		"-a,  -average     Number of samples to average (default is 5).\n");
	fprintf(stderr,
		"-zr, -zeroonreset Zero the counters after the interface has gone down.\n\n");
	fprintf(stderr,
		"The network interface to monitor can also be named on the\n");
	fprintf(stderr,
		"command-line without `-if' preceeding it, for compatiblity \n");
	fprintf(stderr, "with previous versions. ");
	fprintf(stderr,
		"However, the value of the `-if' option has precedence.\n");
}

/**
 * get_in_value - callback that provides the value for the str_in widget.
 * @w: not used
 * @client_data: not used
 * @value: pointer to where the the value to display should be written
 * 
 * Formats the value to be displayed by the str_in widget according to the
 * flags in appdata and provides it to the widget. 
 **/
void get_in_value(Widget w, XtPointer client_data, XtPointer value)
{
	if (appdata.kilobytes) {
		*(double *) value =
		    (double) average.in / (1024 * appdata.scale);
	} else {
		if (average.in < 1.0) {
			*(double *) value = (double) 0;
		} else {
			*(double *) value = log10(average.in);
		}
	}
}

/**
 * get_out_value - callback that provides the value for the str_out widget.
 * @w: not used
 * @client_data: not used
 * @value: pointer to where the the value to display should be written.
 * 
 * Formats the value to be displayed by the str_out widget according to the
 * flags in appdata and provides it to the widget. 
 **/
void get_out_value(Widget w, XtPointer client_data, XtPointer value)
{
	if (appdata.kilobytes) {
		*(double *) value =
		    (double) average.out / (1024 * appdata.scale);
	} else {
		if (average.out < 1.0) {
			*(double *) value = (double) 0;
		} else {
			*(double *) value = log10(average.out);
		}
	}
}

/**
 * refresh - gathers data and updates the displays.
 * @data: not used
 * @id: not used
 * 
 * It formats the information from `average' and `max', the elapsed time 
 * since the program was started, and the interface name,
 * and sets these in the labels in the `interface',
 * 'in' and 'out' widgets. It also registers itself as a timeout routine.
 * 
 **/
void refresh(XtPointer data, XtIntervalId * id)
{
	long hr, min, sec;	/* time vars  */
	struct prefixed_value pval_average, pval_max, pval_total;
	time_t newtime;
	char *dev_str = "%s up: %i:%02i:%02i";
	char buf[128];
	/* read data from /proc/net/dev */
	update_avg(appdata.update, appdata.zeroonreset);
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
	sec %= 60;		/* calculate remaining seconds */
	hr = min / 60;
	min %= 60;
	if (appdata.no_interface == False) {
		/* print the data to strings and set label resources */
		sprintf(buf, dev_str, appdata.iface, hr, min, sec);
		XtVaSetValues(interface, XtNlabel, buf, NULL);
	}
	if (appdata.no_values == False) {
		if (type == BYTES_TYPE) {
			pval_average = use_prefix(8 * average.in, false);
			pval_max = use_prefix(8 * max.in, false);
			pval_total = use_prefix(total.in, true);
			sprintf(buf, in_str, pval_average.value,
				pval_average.prefix, pval_max.value,
				pval_max.prefix, pval_total.value,
				pval_total.prefix);
		} else
			sprintf(buf, in_str, average.in, max.in);
		XtVaSetValues(in, XtNlabel, buf, NULL);
		if (type == BYTES_TYPE) {
			pval_average = use_prefix(8 * average.out, false);
			pval_max = use_prefix(8 * max.out, false);
			pval_total = use_prefix(total.out, true);
			sprintf(buf, out_str, pval_average.value,
				pval_average.prefix, pval_max.value,
				pval_max.prefix, pval_total.value,
				pval_total.prefix);
		} else
			sprintf(buf, out_str, average.out, max.out);
		XtVaSetValues(out, XtNlabel, buf, NULL);
	}

	/* register the timer again */
	XtAppAddTimeOut(app_context, 1000 * appdata.update, refresh, NULL);
}

/**
 * xclose - close action callback
 * @w: not used
 * @event: not used
 * @params: not used
 * @num: not used
 * 
 * Closes the program if it receives the WM_DELETE message from the window
 * manager. 
 */
void xclose(Widget w, XEvent * event, String * params, Cardinal * num)
{
	cleanup();
	exit(0);
}

/**
 * use_prefix - select an appropriate prefix for a value.
 * @value: the number that needs a prefix.
 * @iec: true if IEC prefix should be used, false for SI prefix.
 *
 * Reduces the value to the range 0-1024 or 0-1000, depending on whether
 * the unit is bytes or bits respectively, and selects the appropriate
 * prefix (Ki, M etc.).
 **/
struct prefixed_value use_prefix(const float value, const bool iec)
{
	struct prefixed_value pval;

	pval.value = value;
	const float divisor = iec? 1024.0: 1000.0;
	int i;
	for (i = 0; pval.value > divisor; i++) {
		pval.value /= divisor;
	}
        char **prefix = iec? prefix_iec: prefix_si;
	pval.prefix = prefix[i];

	return pval;
}
