#include <sys/wait.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define COMMAND_IDX 0
#define COMMAND_TARGET_IDX 1
#define BUILTIN_CD "cd"
#define BUILTIN_CD_SZ 2
#define BUILTIN_ECHO "echo"
#define BUILTIN_ECHO_SZ 4
#define BUILTIN_EXIT "exit"
#define BUILTIN_EXIT_SZ 4
#define SP_VAL_EXIT_STATUS "$?"
#define SP_VAL_PID "$$"
#define SP_VALS_SZ 2
#define STATUS_CMD_FAILED 127

int last_exit_status;

int
builtin_cd(char **tokens, size_t token_cnt)
{
	char *cd_target;
	struct passwd *pw;

	if (token_cnt > 2) {
		free_split_str(tokens);
		fprintf(stderr, "%s: cd: too many args\n", getprogname());
		return -1;
	}

	if (tracing) {
		fprintf(stderr, "+ %s\n", tokens[COMMAND_IDX]);
	}

	if (token_cnt == 1) {
		if ((cd_target = getenv("HOME")) == NULL) {
			pw = getpwuid(getuid());
			cd_target = pw->pw_dir;
		}
		if (chdir(cd_target) != 0) {
			perror("chdir");
			return -1;
		}
	} else {
		if (chdir(tokens[COMMAND_TARGET_IDX]) != 0) {
			perror("chdir");
			return -1;
		}
	}
	return 0;
}

int
builtin_echo(char **tokens, size_t token_cnt)
{
	size_t i;
	FILE *fp;
	char *val;
	char *varname;

	fp = stdout;
	if (tracing) {
		fp = stderr;
	}

	for (i = COMMAND_TARGET_IDX; i < token_cnt; i++) {
		/* 0th index bc special/env vars always begin with '$' */
		if (tokens[i][0] == '$') {
			if (strcmp(tokens[i], "$?") == 0) {
				fprintf(fp, "%d", last_exit_status);
			} else if (strcmp(tokens[i], "$$") == 0) {
				fprintf(fp, "%d", (int)getpid());
			} else {
				/* +1 offset to retrieve env var name */
				varname = tokens[i] + 1;
				if ((val = getenv(varname)) != NULL) {
					fprintf(fp, "%s", val);
				} else {
					continue;
				}
			}
		} else {
			fprintf(fp, "%s", tokens[i]);
		}
		fprintf(fp, " ");
	}

	fprintf(fp, "\n");
	return 0;
}

int
exec_command(char *command)
{
	pid_t pid;
	char *cmd_src, **tokens;
	int status;
	size_t token_cnt;

	/* exit */
	if (strlen(command) == BUILTIN_EXIT_SZ &&
	    strncmp(command, BUILTIN_EXIT, BUILTIN_EXIT_SZ) == 0) {
		exit_flag = true;
		last_exit_status = EXIT_SUCCESS;
		return 0;
	}

	tokens = split_str(command, " \t\n", &token_cnt);
	cmd_src = tokens[COMMAND_IDX];

	/* cd */
	if (strlen(cmd_src) == BUILTIN_CD_SZ &&
	    strncmp(cmd_src, BUILTIN_CD, BUILTIN_CD_SZ) == 0) {
		if ((last_exit_status = builtin_cd(tokens, token_cnt)) != 0) {
			free_split_str(tokens);
			return -1;
		}
		free_split_str(tokens);
		return 0;
	}

	/* echo */
	if (strlen(cmd_src) == BUILTIN_ECHO_SZ &&
	    strncmp(cmd_src, BUILTIN_ECHO, BUILTIN_ECHO_SZ) == 0) {
		if ((last_exit_status = builtin_echo(tokens, token_cnt)) != 0) {
			free_split_str(tokens);
			return -1;
		}
		free_split_str(tokens);
		return 0;
	}

	/* fork+exec */
	pid = fork();
	if (pid < 0) {
		perror("fork failed");
		free_split_str(tokens);
		return -1;
	}
	if (pid == 0) {
		/* redirect cmd to stderr if -x */
		if (tracing && dup2(STDERR_FILENO, STDOUT_FILENO) == -1) {
			perror("dup2");
			free_split_str(tokens);
			return -1;
		}
		if (tracing) {
			fprintf(stderr, "+ %s\n", command);
		}
		if (execvp(cmd_src, tokens) == -1) {
			free_split_str(tokens);
			fprintf(stderr, "%s: %s: not found\n", getprogname(),
			        tokens[COMMAND_IDX]);
			_exit(STATUS_CMD_FAILED);
		}
	}

	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid failed");
		free_split_str(tokens);
		return -1;
	}

	/* populate exit status of last cmd */
	if (WIFEXITED(status)) {
		last_exit_status = WEXITSTATUS(status);
	} else {
		last_exit_status = STATUS_CMD_FAILED;
		free_split_str(tokens);
		return -1;
	}

	free_split_str(tokens);
	return 0;
}

void
exec_commands(char **commands, int cmd_cnt)
{
	size_t i;

	if (cmd_cnt < 1) {
		return;
	}

	/* exec single command; no pipes
	if (cmd_cnt < 3 && (exec_command(commands[0]) < 0)) {
	    return -1;
	}*/

	/* TESTING */
	for (i = 0; commands[i] != NULL; i++) {
		if (exec_command(commands[i]) != 0) {
			return;
		}
	}


	/* exec multiple commands and connect with pipes ... */
	return;
}