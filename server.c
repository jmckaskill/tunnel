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

static void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	exit(256);
}

static void usage() {
	fprintf(stderr, "usage: ./server <backend port> <front end port>\n");
	exit(1);
}

static void xsplice(int from, int to) {
	while (splice(from, NULL, to, NULL, 1024*1024*1024, SPLICE_F_MOVE | SPLICE_F_MORE) > 0) {
	}
}

static int listen_tcp(unsigned short port) {
	int fd;
	struct sockaddr_in6 si;
	memset(&si, 0, sizeof(si));
	si.sin6_family = AF_INET6;
	si.sin6_port = htons(port);

	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		die("socket create *:%d failed %s", port, strerror(errno));
	}
	if (bind(fd, (struct sockaddr*) &si, sizeof(si))) {
		die("bind to *:%d failed %s", port, strerror(errno));
	}
	if (listen(fd, SOMAXCONN)) {
		die("listen to *:%d failed %s", port, strerror(errno));
	}

	return fd;
}

int main(int argc, char* argv[]) {
	int fd1, fd2;
	int port1, port2;
	char* end;

	if (argc != 3 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		usage();
	}

	port1 = strtol(argv[1], &end, 10);
	if (*end) usage();

	port2 = strtol(argv[1], &end, 10);
	if (*end) usage();

	fd1 = listen_tcp(port1);
	fd2 = listen_tcp(port2);

	for (;;) {
		int c1, c2;

		/* accept a connection from an incoming client before
		 * accepting the connection from the backend */
		c2 = accept(fd2, NULL, NULL);
		c1 = accept(fd1, NULL, NULL);

		if (!fork()) {
			xsplice(c1, c2);
			exit(0);
		}

		if (!fork()) {
			/* notify the backend that it can connect to its
			 * backend */
			write(c1, "\0", 1);
			xsplice(c2, c1);
			exit(0);
		}

		close(c1);
		close(c2);
	}
}
