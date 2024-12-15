#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

/* initialize global opt variables */
char *c_cmd = NULL;
bool tracing = false;

/* returns arr of commands delimited by '|' */
char **
pipe_split(char *line)
{
	size_t size = 10;
	size_t command_count = 0;
	char **commands = malloc(size * sizeof(char *));
	char *last, *command;

	if (commands == NULL) {
		perror("malloc failed");
		exit(1);
	}

	for ((command = strtok_r(line, "|", &last)); command != NULL;
	     (command = strtok_r(NULL, "|", &last)), command_count++) {
		if (command_count >= size) {
			size *= 2;
			char **new_commands = realloc(commands, size * sizeof(char *));
			if (new_commands == NULL) {
				perror("realloc failed");
				exit(1);
			}
			commands = new_commands;
		}

		while (*command == ' ') {
			command++;
		}
		char *end = command + strlen(command) - 1;
		while (end > command && *end == ' ') {
			*end-- = '\0';
		}
		commands[command_count] = strdup(command);
		if (commands[command_count] == NULL) {
			perror("strdup failed");
			exit(1);
		}
	}

	commands[command_count] = NULL;
	return commands;
}

int
main(int argc, char **argv)
{
	char *line = NULL;
	size_t len, i = 0;
	ssize_t nread;
	char **commands;

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

		commands = pipe_split(line);

		for (i = 0; commands[i] != NULL; i++) {
			printf("Command %zu: '%s'\n", i, commands[i]);
			free(commands[i]);
		}

		free(commands);

		if (strncmp(line, "exit", 4) == 0) {
			break;
		}
	}

	free(line);
	return 0;
}