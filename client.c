#include "common.h"

static void usage() {
	fprintf(stderr, "usage: ./client <server host> <server port> <backend host> <backend port>\n");
	exit(1);
}

static int dial(const char* host, const char* port) {
	int fd;
	struct addrinfo hints, *ai, *res;

	if (!port) die("need to specify a port for %s", host);

	memset(&hints, 0, sizeof(hints));
	if (getaddrinfo(host, port, &hints, &res)) {
		warning("name lookup %s:%s failed %s", host, port, strerror(errno));
		return -1;
	}

	for (ai = res; ai != NULL; ai = ai->ai_next) {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) continue;

		if (connect(fd, ai->ai_addr, ai->ai_addrlen)) {
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}

	if (fd < 0) {
		warning("failed to connect to %s:%s %s", host, port, strerror(errno));
		return -1;
	}

	return fd;
}

int main(int argc, char* argv[]) {
	if (argc != 5 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		usage();
	}

	signal(SIGCHLD, &sigchild);

	for (;;) {
		char ready;
		int remote = -1;
		int local = -1;

		remote = dial(argv[1], argv[2]);
		if (remote < 0) {
			goto retry;
		}

		/* Wait for the server to indicate there is someone
		 * connected
		 */
		if (read(remote, &ready, sizeof(ready)) != 1) {
			goto retry;
		}

		local = dial(argv[3], argv[4]);
		if (local < 0) {
			goto retry;
		}

#if 0
		if (!fork()) {
			join(remote, local);
			exit(0);
		}
#else
		join(remote, local);
#endif

		close(local);
		close(remote);
		continue;
retry:
		close(local);
		close(remote);
		sleep(15);
	}
}
