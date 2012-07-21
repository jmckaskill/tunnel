#include "common.h"

static void usage() {
	fprintf(stderr, "usage: ./server <backend port> <front end port>\n");
	exit(1);
}

static int listen_tcp(unsigned short port) {
	int fd;
	struct sockaddr_in6 si6;
	struct sockaddr_in si4;
	struct sockaddr* sa = (struct sockaddr*) &si6;
	size_t salen = sizeof(si6);
	int v6only = 0;

	memset(&si6, 0, sizeof(si6));
	si6.sin6_family = AF_INET6;
	si6.sin6_port = htons(port);

	memset(&si4, 0, sizeof(si4));
	si4.sin_family = AF_INET;
	si4.sin_port = htons(port);

	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0 || setsockopt(fd, SOL_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only))) {
		warning("IPv6 only not supported, falling back to IPv4");
		close(fd);
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sa = (struct sockaddr*) &si4;
		salen = sizeof(si4);
	}
	if (bind(fd, sa, salen)) {
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

	signal(SIGCHLD, &sigchild);

	port1 = strtol(argv[1], &end, 10);
	if (*end) usage();

	port2 = strtol(argv[2], &end, 10);
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
