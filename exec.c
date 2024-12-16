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

int
exec_command(char *command)
{
	pid_t pid;
	char *cmd_src, *cd_target, **tokens;
	int status;
	size_t token_cnt;
	struct passwd *pw;


	(void)pid;

	/* handle builtins */
	if (strlen(command) == BUILTIN_EXIT_SZ &&
	    strncmp(command, BUILTIN_EXIT, BUILTIN_EXIT_SZ) == 0) {
		exit_flag = 1;
		return 0;
	}

	tokens = split_str(command, " \t\n", &token_cnt);
	cmd_src = tokens[COMMAND_IDX];

	if (strlen(cmd_src) == BUILTIN_CD_SZ &&
	    strncmp(cmd_src, BUILTIN_CD, BUILTIN_CD_SZ) == 0) {
		if (token_cnt > 2) {
			free_split_str(tokens);
			fprintf(stderr, "cd: too many args\n");
			return -1;
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
		free_split_str(tokens);
		return 0;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork failed");
		free_split_str(tokens);
		return -1;
	}

	if (pid == 0) {
		if (execvp(cmd_src, tokens) == -1) {

			free_split_str(tokens);
			exit(EXIT_FAILURE);
		}
	} else {
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid failed");
			free_split_str(tokens);
			return -1;
		}

		if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			fprintf(stderr, "command exited with status %d\n",
			        WEXITSTATUS(status));
		}
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
		if (exec_command(commands[i]) != 0) {
			return -1;
		}
	}


	/* exec multiple commands and connect with pipes ... */
	return 0;
}