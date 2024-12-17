#include <sys/wait.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"


/* initialize global variables */
char *c_cmd = NULL;
bool tracing = false;
bool exit_flag = false;

int
main(int argc, char **argv)
{
	char *line = NULL;
	size_t len, cmd_cnt = 0;
	ssize_t nread;
	char **commands;

	get_options(argc, argv);
	if (install_handlers() == -1) {
		exit(EXIT_FAILURE);
	}

	/*
	TODO single command mode
	 */
	if (c_cmd != NULL) {
		commands = split_str(c_cmd, "|", &cmd_cnt);
		if (cmd_cnt <= 0) {
			fprintf(stderr, "%s: provide at least one command\n",
			        getprogname());
			free_split_str(commands);
			exit(EXIT_FAILURE);
		}

		/*if (exec_commands(commands, cmd_cnt) != 0) {
		    free_split_str(commands);
		    exit(EXIT_FAILURE);
		}*/

		exec_commands(commands, cmd_cnt);
		free_split_str(commands);
		exit(EXIT_SUCCESS);
	}

	/* shell mode */
	while (1) {
		if (exit_flag) {
			break;
		}
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
				free(line);
				break;
			}
			if (errno == EINTR) {
				continue;
			} else {
				fprintf(stderr, "%s: %s\n", getprogname(), strerror(errno));
				free(line);
				return (int)nread;
			}
		}
		if (line[nread - 1] == '\n') {
			line[nread - 1] = '\0';
		}

		commands = split_str(line, "|", &cmd_cnt);

		exec_commands(commands, cmd_cnt);
		free_split_str(commands);
	}

	free(line);
	exit(EXIT_SUCCESS);
}