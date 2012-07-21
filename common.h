#pragma once

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

static void sigchild(int sig) {
	int sts;
	wait(&sts);
}

static void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	exit(256);
}

static void warning(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, ap);
}

static void xsplice(int from, int to) {
	char buf[64*1024];
	for (;;) {
		int r,n,w;

		r = read(from, buf, sizeof(buf));
		if (r < 0 && errno == EINTR) {
			continue;
		} else if (r <= 0) {
			shutdown(to, SHUT_WR);
			return;
		}

		n = 0;
		while (n < r) {
			w = write(to, buf + n, r - n);
			if (w < 0 && errno == EINTR) {
				continue;
			} else if (w <= 0) {
				shutdown(from, SHUT_RD);
				return;
			}
			n += w;
		}
	}
}
