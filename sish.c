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
bool exit_flag = false;

/* returns arr of trimmed tokens delimited by delimiters; must be freed */
char **
split_str(char *str, const char *delimiters, size_t *count)
{
	size_t size;
	size_t token_count;
	char **tokens;
	char *last;
	char *token;
	char *end;
	char **new_tokens;
	char *str_cpy;

	str_cpy = strdup(str);
	size = 10;
	token_count = 0;
	tokens = malloc(size * sizeof(char *));
	if (tokens == NULL) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}

	token = strtok_r(str_cpy, delimiters, &last);
	while (token != NULL) {
		while (*token == ' ') {
			token++;
		}
		end = token + strlen(token) - 1;
		while (end > token && *end == ' ') {
			*end-- = '\0';
		}

		if (*token == '\0') {
			token = strtok_r(NULL, delimiters, &last);
			continue;
		}

		if (token_count >= size) {
			size *= 2;
			new_tokens = realloc(tokens, size * sizeof(char *));
			if (new_tokens == NULL) {
				perror("realloc failed");
				exit(EXIT_FAILURE);
			}
			tokens = new_tokens;
		}

		tokens[token_count] = strdup(token);
		if (tokens[token_count] == NULL) {
			perror("strdup failed");
			exit(EXIT_FAILURE);
		}
		token_count++;
		token = strtok_r(NULL, delimiters, &last);
	}

	tokens[token_count] = NULL;
	*count = token_count;

	return tokens;
}

int
exec_command(char *command)
{
	pid_t pid;
	char **tokens;
	size_t token_cnt, i;

	(void)pid;

	/* handle builtins */
	if (strncmp(command, "exit", 4) == 0) {
		exit_flag = 1;
		return 0;
	}
	if (strncmp(command, "cd", 2) == 0) {
		/* tokenize command */
		/* chdir to given dir, or to $HOME if not given */
		tokens = split_str(command, " \t\n", &token_cnt);
		free_split_str(tokens);
	}

	tokens = split_str(command, " \t\n", &token_cnt);
	for (i = 0; tokens[i] != NULL; i++) {
		printf("cmdtoken %zu: '%s'\n", i, tokens[i]);
	}
	free_split_str(tokens);

	return 0;
}

int
exec_commands(char **commands, int cmd_cnt)
{
	size_t i;

	if (cmd_cnt < 1) {
		return 0;
	}

	/* exec single command; no pipes
	if (cmd_cnt < 3 && (exec_command(commands[0]) < 0)) {
	    return -1;
	}*/

	/* TESTING */
	for (i = 0; commands[i] != NULL; i++) {
		exec_command(commands[i]);
	}


	/* exec multiple commands and connect with pipes ... */
	return 0;
}

int
main(int argc, char **argv)
{
	char *line = NULL;
	size_t len, cmd_cnt = 0;
	ssize_t nread, i;
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

		/* TESTING */
		for (i = 0; commands[i] != NULL; i++) {
			printf("cmd %zu: '%s'\n", i, commands[i]);
		}

		if (exec_commands(commands, cmd_cnt) != 0) {
			fprintf(stderr, "%s: %s\n", getprogname(), "exec_commands failed");
		}

		free_split_str(commands);
	}

	free(line);
	exit(EXIT_SUCCESS);
}