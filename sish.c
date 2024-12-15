#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

volatile sig_atomic_t sig_recv = 0;

void
handle_sig(int sig)
{
	(void)sig;
	sig_recv = 1;
}

int
install_handlers()
{
	struct sigaction sa;

	sa.sa_handler = handle_sig;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT handler");
		return -1;
	}
	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		perror("sigaction SIGQUIT handler");
		return -1;
	}
	if (sigaction(SIGTSTP, &sa, NULL) == -1) {
		perror("sigaction SIGTSTP");
		return -1;
	}
	return 0;
}

/* initialize global opt variables */
char *c_cmd = NULL;
bool tracing = false;

int
main(int argc, char **argv)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	get_options(argc, argv);
	if (install_handlers() == -1) {
		exit(EXIT_FAILURE);
	}

	if (c_cmd != NULL) {
	}

	/* shell mode */
	while (1) {
		if (sig_recv) {
			printf("\n");
			sig_recv = 0;
		}
		printf("sish$ ");
		fflush(stdout);


		nread = getline(&line, &len, stdin);
		if (nread == -1) {
			if (feof(stdin)) {
				fprintf(stderr, "%s: EOF detected. Exiting sish\n",
				        getprogname());
				break;
			}
			if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "%s: %s\n", getprogname(), strerror(errno));
				return (int)nread;
			}
		}


		if (line[nread - 1] == '\n') {
			line[nread - 1] = '\0';
		}

		if (strncmp(line, "exit", 4) == 0) {
			break;
		}
	}

	free(line);
	return 0;
}