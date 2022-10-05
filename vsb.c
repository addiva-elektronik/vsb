/* Virtual serial bus using Linux PTYs
 *
 * Copyright (C) 2022  Addiva Elektronik AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define _XOPEN_SOURCE 600	/* Unlock PTY functions */
#define _DEFAULT_SOURCE		/* Same, newer GLIBC */
#define _GNU_SOURCE		/* Same, older GLIBC */

#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#define dbg(...) if (debug) warnx(__VA_ARGS__)
#define log(...)            warnx(__VA_ARGS__)

static char *prognm;
static int   debug;

int get_term(int id)
{
	int fd;
	int res;
	char *name;

	fd = posix_openpt(O_RDWR);
	if (fd < 0)
		err(1, "Failed opening PTY");

	res = grantpt(fd);
	if (res)
		err(1, "Failed granting PTY");

	res = unlockpt(fd);
	if (res)
		err(1, "Failed unlocking PTY");

	name = ptsname(fd);
	if (!name)
		err(1, "Failed acquiring slave PTY");
	log("device %d => %s", id, name);

	return fd;
}

static void forward(int fd, struct pollfd *pfd, int num)
{
	int bytes;
	char c;

	bytes = read(fd, &c, 1);
	dbg("Read %d bytes from %s, forwarding ...", bytes, ptsname(fd) ?: "<nil>");
	for (int i = 0; i < num; i++) {
		if (pfd[i].fd == fd)
			continue;

		if (write(pfd[i].fd, &c, 1) == -1)
			warn("Failed forwarding to %s", ptsname(pfd[i].fd) ?: "<nil>");
	}
}

static int process(struct pollfd *pfd, int num)
{

	while (poll(pfd, num, -1) != -1) {
		for (int i = 0; i < num; i++) {
			if (!pfd[i].revents)
				continue;

			if (pfd[i].revents & POLLIN)
				forward(pfd[i].fd, pfd, num);
			if (pfd[i].revents & POLLHUP) {
//				log("Hang-up detected on %s", ptsname(pfd[i].fd) ?: "<nil>");
				pfd[i].revents &= ~POLLHUP;
			}
		}
	}

	return 0;
}

static int usage(int rc)
{
	fprintf(rc ? stderr : stdout,
		"Usage: %s [-d] [-n NUM]\n"
		"\n"
		"Options:\n"
		" -f         Enable debug messages\n"
		" -n NUM     Number of serial ports to create, default: 0\n"
		"\n"
		"Copyright (c) 2022  Addiva Elektronik AB\n", prognm);
	return rc;
}

int main(int argc, char **argv)
{
	struct pollfd *pfd;
	int num = 0;
	int c;

	prognm = rindex(argv[0], '/');
	if (prognm)
		prognm++;
	else
		prognm = argv[0];

	while ((c = getopt(argc, argv, "hn:")) != EOF) {
		switch (c) {
		case 'n':
			num = atoi(optarg);
			break;
		default:
			return usage(0);
		}
	}

	if (!num)
		return usage(1);

	pfd = calloc(num, sizeof(*pfd));
	if (!pfd)
		err(1, "Failed allocating %d x pollfd", num);

	for (int i = 0; i < num; i++) {
		pfd[i].fd = get_term(i + 1);
		pfd[i].events = POLLIN;
	}

	return process(pfd, num);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
