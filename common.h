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
#include <sys/select.h>

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

struct buf {
	char v[64*1024];
	int b, e;
};

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) < (b) ? (b) : (a))
#endif

static int xrecv(int fd, char* p, size_t sz) {
	int n;
	do {
		n = recv(fd, p, sz, 0);
	} while (n == 0 && errno == EINTR);

	if (n < 0 && errno == EWOULDBLOCK) {
		return 0;
	} else if (n <= 0) {
		return -1;
	} else {
		return n;
	}
}

static int xsend(int fd, const char* p, size_t sz) {
	int n;
	do {
		n = send(fd, p, sz, 0);
	} while (n == 0 && errno == EINTR);

	if (n < 0 && errno == EWOULDBLOCK) {
		return 0;
	} else if (n <= 0) {
		return -1;
	} else {
		return n;
	}
}

static int do_recv(int fd, struct buf* b) {
	int n = xrecv(fd, b->v + b->e, ((b->b <= b->e) ? min(b->b + sizeof(b->v) - 1, sizeof(b->v)) : b->b-1) - b->e);
	if (n < 0) return -1;
	b->e += n;

	if (b->e < sizeof(b->v)) {
		return 0;
	}

	n = xrecv(fd, b->v, b->b - 1);
	if (n < 0) return -1;
	b->e = n;

	return 0;
}

static int do_send(int fd, struct buf* b) {
	int n = xsend(fd, b->v + b->b, ((b->e >= b->b) ? b->e : sizeof(b->v)) - b->b);
	if (n < 0) return -1;
	b->b += n;

	if (b->b < sizeof(b->v)) {
		return 0;
	}

	b->b = 0;
	if (!b->e) {
		return 0;
	}

	n = xsend(fd, b->v, b->e);
	if (n < 0) return -1;
	b->b += n;

	return 0;
}

static void join(int fd1, int fd2) {
	fd_set r, w;
	struct buf b1, b2;

	b1.b = b1.e = 0;
	b2.b = b2.e = 0;

	fcntl(fd1, F_SETFL, O_NONBLOCK);
	fcntl(fd2, F_SETFL, O_NONBLOCK);

	for (;;) {
		FD_ZERO(&r);
		FD_ZERO(&w);
		if (b1.b != b1.e) {
			FD_SET(fd2, &w);
		}
		if (b2.b != b2.e) {
			FD_SET(fd1, &w);
		}
		if ((b1.e + 1) % sizeof(b1.v) != b1.b) {
			FD_SET(fd1, &r);
		}
		if ((b2.e + 1) % sizeof(b2.v) != b2.b) {
			FD_SET(fd2, &r);
		}

		select(max(fd1, fd2), &r, &w, NULL, NULL);

		if (FD_ISSET(fd1, &r) && do_recv(fd1, &b1)) {
			break;
		}

		if (FD_ISSET(fd2, &w) && do_send(fd2, &b1)) {
			break;
		}

		if (FD_ISSET(fd2, &r) && do_recv(fd2, &b2)) {
			break;
		}

		if (FD_ISSET(fd1, &w) && do_send(fd1, &b2)) {
			break;
		}
	}

	shutdown(fd1, SHUT_RDWR);
	shutdown(fd2, SHUT_RDWR);
}
