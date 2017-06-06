/*
 * Copyright 2017 Joey Hewitt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * Implementation of syslog() libc functions, which writes to stderr instead of syslog.
 * stderr is dup'd at the time of openlog(), so the process may change its descriptors, but
 * we will still log where we would have in the beginning.
 *
 * Only ident, optional PID, and the main message are written -- timestamp, facility, and priority are ignored.
 *
 */

#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

#define SELFTAG "syslog2stderr"

static int g_logfd = STDERR_FILENO;
static const char *g_ident = NULL;
static int g_options = 0;
static int g_logmask = 0xff;

void openlog(const char *ident, int option, int facility __attribute((unused))) {
	g_logfd = dup(STDERR_FILENO); /* copy current stderr */
	g_ident = ident;
	g_options = option;
}

void syslog(int priority, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}

void vsyslog(int priority, const char *format, va_list ap) {
	if ((LOG_MASK(LOG_PRI(priority)) & g_logmask) == 0) {
		return;
	}

	/* prepare output line, so it can be written in one call and not interleave with concurrent messages */
	char *buf = 0;
	size_t buflen = 0;
	FILE *msgf = open_memstream(&buf, &buflen);
	if (!msgf) {
		dprintf(g_logfd, SELFTAG": error logging to stderr: %m\n");
		return;
	}

	/* print header - trying to follow glibc format */
	if (g_ident) {
		fputs(g_ident, msgf);
	}
	if (g_options & LOG_PID) {
		fprintf(msgf, "[%d]", getpid());
	}
	if (g_ident) {
		fputs(": ", msgf);
	}

	/* printf with caller's args */
	vfprintf(msgf, format, ap);

	fputc('\n', msgf);

	/* now dump the whole line to the output fd */
	fclose(msgf);
	write(g_logfd, buf, buflen);
	free(buf);
}

int setlogmask(int pmask) {
	int oldmask = g_logmask;

	if (pmask != 0) {
		g_logmask = pmask;
	}

	return oldmask;
}

void closelog() {
	close(g_logfd);
	g_logfd = STDERR_FILENO;
}
