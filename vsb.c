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
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define dbg(...) if (debug) warnx(__VA_ARGS__)
#define log(...)            warnx(__VA_ARGS__)

static char *prognm;
static int   debug;
static int   dummy[42];
static char *devices[8];
static char *cmds[8][42];
static pid_t pids[8];

int get_term(int id)
{
	int num = id - 1;
	int rc;
	int fd;

	fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (fd < 0)
		err(1, "failed opening PTY");

	rc = grantpt(fd);
	if (rc)
		err(1, "failed granting PTY");

	rc = unlockpt(fd);
	if (rc)
		err(1, "failed unlocking PTY");

	devices[num] = ptsname(fd);
	if (!devices[num])
		err(1, "failed acquiring slave PTY");

	/* https://stackoverflow.com/questions/66105153/non-blocking-pseudo-terminal-recovery-after-pollhup */
	dummy[num] = open(devices[num], O_WRONLY | O_NOCTTY);

	if (cmds[num][0]) {
		char *cmd = cmds[num][0];
		char buf[256] = { 0 };

		for (int i = 0; cmds[num][i]; i++) {
			if (!strcmp(cmds[num][i], "%p"))
				cmds[num][i] = devices[num];
		}

		pids[num] = fork();
		if (pids[num] == -1)
			err(1, "failed forking off child %d: %s", id, cmd);
		if (pids[num] == 0) {
			execvp(cmd, cmds[num]);
			_exit(1); /* we never get here */
		}

		/* parent continues here */
		for (int j = 0; cmds[num][j]; j++) {
			strcat(buf, cmds[num][j]);
			strcat(buf, " ");
		}
		log("execmd %d => %s", id, buf);
	} else {
		log("device %d => %s", id, devices[num]);
	}

	return fd;
}

static void forward(int fd, struct pollfd *pfd, int num)
{
	int bytes;
	char c;

	bytes = read(fd, &c, 1);
	dbg("read %d bytes from %s, forwarding ...", bytes, ptsname(fd) ?: "<nil>");
	for (int i = 0; i < num; i++) {
		if (pfd[i].fd == fd)
			continue;

		if (write(pfd[i].fd, &c, 1) == -1)
			warn("failed forwarding to %s", ptsname(pfd[i].fd) ?: "<nil>");
	}
}

static int process(struct pollfd *pfd, int num)
{
	for (int i = 0; i < num; i++) {
		pfd[i].fd = get_term(i + 1);
		pfd[i].events = POLLHUP | POLLIN;
	}

	while (poll(pfd, num, 1) != -1) {
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

	for (int i = 0; i < num; i++) {
		close(pfd[i].fd);
		close(dummy[i]);
	}

	return 0;
}

static int usage(int rc)
{
	fprintf(rc ? stderr : stdout,
		"Usage: %s [-d] [-n NUM] [-- prog1 -a -r -g %%p -- prog2 -arg %%p [-- ...]]\n"
		"\n"
		"Options:\n"
		" -d         Enable debug messages\n"
		" -n NUM     Number of serial ports to create, default: 0\n"
		"Arguments:\n"
		" prog ARGS  Programs to start for each serial port created.\n"
		"            Separated from %s and other progs with '--'\n"
		"            Use %%p to denote where add the serial port name.\n"
		"\n"
		"Copyright (c) 2022  Addiva Elektronik AB\n", prognm, prognm);
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

	while ((c = getopt(argc, argv, "dhn:")) != EOF) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		default:
			return usage(0);
		}
	}

	c = 0;
	while (optind < argc) {
		if (!strcmp(argv[optind], "--")) {
			optind++;
			num++;
			c = 0;
			continue;
		}

		cmds[num][c++] = argv[optind++];
		cmds[num][c] = NULL;
	}

	if (cmds[num][0])
		num++;

	if (!num)
		return usage(1);

	pfd = calloc(num, sizeof(*pfd));
	if (!pfd)
		err(1, "Failed allocating %d x pollfd", num);

	return process(pfd, num);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
