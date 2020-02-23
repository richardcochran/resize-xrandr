/**
 * @file resize-xrandr.c
 * @brief Resizes a VM guest using xrandr, even without gnome.
 * @note Copyright (C) 2020 Richard Cochran <richardcochran@gmail.com>
 * @note SPDX-License-Identifier: GPL-2.0+
 */
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#define ATOM_NAME "hotplug_mode_update"

#define pr_err(x...)		print(LOG_ERR, x)
#define pr_warning(x...)	print(LOG_WARNING, x)
#define pr_info(x...)		print(LOG_INFO, x)
#define pr_debug(x...)		print(LOG_DEBUG, x)

static int running = 1, use_syslog = 1, verbose = 0;

static void print(int level, char const *format, ...);

static void handle_int_quit_term(int s)
{
	running = 0;
	pr_debug("caught signal %d", s);
}

static int hotplug_active(Display *display)
{
	unsigned long nitems, bytes_after;
	Atom actual_type, property;
	int actual_format, result;
	unsigned char *junk;
	RROutput rr_output;

	rr_output = XRRGetOutputPrimary(display, DefaultRootWindow(display));
	property = XInternAtom(display, ATOM_NAME, False);

	XRRGetOutputProperty(display, rr_output, property, 0, LONG_MAX,
			     False, False, AnyPropertyType, &actual_type,
			     &actual_format, &nitems, &bytes_after, &junk);

	result = actual_type == None ? 0 : 1;
	XFree(junk);
	return result;
}

static void print(int level, char const *format, ...)
{
	va_list ap;
	char buf[1024];
	FILE *f;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	if (verbose) {
		f = level >= LOG_NOTICE ? stdout : stderr;
		fprintf(f, "%s\n", buf);
		fflush(f);
	}
	if (use_syslog) {
		syslog(level, "%s", buf);
	}
}

static void usage(char *progname)
{
	fprintf(stderr,
		"\nusage: %s [options]\n\n"
		" -h        prints this message and exits\n"
		" -m        print messages to stdout\n"
		" -q        do not print messages to the syslog\n"
		"\n",
		progname);
}

int main(int argc, char *argv[])
{
	int c, error_base, event_base;
	Display *display;
	char *progname;
	XEvent event;

	if (SIG_ERR == signal(SIGINT, handle_int_quit_term)) {
		fprintf(stderr, "cannot handle SIGINT\n");
		return -1;
	}
	if (SIG_ERR == signal(SIGQUIT, handle_int_quit_term)) {
		fprintf(stderr, "cannot handle SIGQUIT\n");
		return -1;
	}
	if (SIG_ERR == signal(SIGTERM, handle_int_quit_term)) {
		fprintf(stderr, "cannot handle SIGTERM\n");
		return -1;
	}
	
	/* Process the command line arguments. */
	progname = strrchr(argv[0], '/');
	progname = progname ? 1 + progname : argv[0];
	while (EOF != (c = getopt(argc, argv, "hmq"))) {
		switch (c) {
		case 'h':
			usage(progname);
			return 0;
		case 'm':
			verbose = 1;
			break;
		case 'q':
			use_syslog = 0;
			break;
		case '?':
			usage(progname);
			return -1;
		default:
			usage(progname);
			return -1;
		}
	}

	display = XOpenDisplay(NULL);
	if (!display) {
		pr_err("failed to open display.");
		return -1;
	}
	if (!XRRQueryExtension(display, &event_base, &error_base)) {
		pr_err("failed to query event base.");
		return -1;
	}
	if (!hotplug_active(display)) {
		pr_err("display lacks hotplug mode update.");
		return -1;
	}

	XRRSelectInput(display, DefaultRootWindow(display),
		       RRScreenChangeNotifyMask);

	while (running) {
		while (XPending(display)) {
			XNextEvent(display, &event);
			if ((event.type - event_base) == RRScreenChangeNotify) {
				pr_info("RRScreenChangeNotify");
			} else {
				pr_warning("unexpected event, no change");
				continue;
			}
			XRRUpdateConfiguration(&event);
			system("xrandr --output Virtual-0 --auto");
		}
		usleep(100000);
	}

	XCloseDisplay(display);
	return 0;
}
