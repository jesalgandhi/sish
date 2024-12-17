#include <sys/fcntl.h>
#include <sys/wait.h>

#include <ctype.h>
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

#define REDIRECTS_ARR_SZ 4
#define REDIR_STDOUT_IDX 0
#define APPEND_STDOUT_IDX 1
#define REDIR_STDIN_IDX 2

#define STATUS_CMD_FAILED 127

int last_exit_status;

void
free_redirs(char **redirs)
{
	size_t i;
	for (i = 0; i < REDIRECTS_ARR_SZ; i++) {
		free(redirs[i]);
	}
	free(redirs);
}

char *
extract_word(char *s)
{
	char *p;
	size_t len;
	char *ret;

	p = s;
	while (*p && !isspace((unsigned char)*p)) {
		p++;
	}
	len = (size_t)(p - s);
	if (len == 0) {
		return NULL;
	}
	ret = (char *)malloc(len + 1);
	if (!ret) {
		return NULL;
	}
	strncpy(ret, s, len);
	ret[len] = '\0';
	return ret;
}

/* general pseudocode:
  int i = 0
  space_after = false
  redirs[4] = [NULL,NULL,NULL,NULL]

  for i in str:
    t1 = s[i]
    t2 = s[i+1]
    if t1 == '>' and t2 not NULL and t2 == '>':
      if (t2 + 1) == " ":
        populate redirs with substring from (t2+2) to next space/NULL
      else:
        populate redirs with substring from (t2+1) to next space/NULL

    else if t1 == '>':
      if (t1 + 1) == " ":
        populate redirs with substring from (t1+2) to next space/NULL
      else:
        populate redirs with substring from (t1+1) to next space/NULL
    else if t1 == '<':
      if (t1 + 1) == " ":
          populate redirs with substring from (t1+2) to next space/NULL
      else:
        populate redirs with substring from (t1+1) to next space/NULL
*/
char **
retrieve_redirects(char *command)
{
	char **redirs;
	char *s;
	char *start;
	redirs = (char **)malloc(sizeof(char *) * REDIRECTS_ARR_SZ);
	if (!redirs) {
		return NULL;
	}

	/* init all to NULL */
	redirs[REDIR_STDOUT_IDX] = NULL;
	redirs[APPEND_STDOUT_IDX] = NULL;
	redirs[REDIR_STDIN_IDX] = NULL;
	redirs[REDIRECTS_ARR_SZ - 1] = NULL;

	s = command;
	while (*s) {
		if (*s == '>') {
			if (*(s + 1) == '>') {
				/* >> */
				if (*(s + 2) && isspace((unsigned char)*(s + 2))) {
					start = s + 3;
				} else {
					start = s + 2;
				}

				if (redirs[REDIR_STDOUT_IDX]) {
					free(redirs[REDIR_STDOUT_IDX]);
					redirs[REDIR_STDOUT_IDX] = NULL;
				}
				if (redirs[APPEND_STDOUT_IDX]) {
					free(redirs[APPEND_STDOUT_IDX]);
					redirs[APPEND_STDOUT_IDX] = NULL;
				}

				redirs[APPEND_STDOUT_IDX] = extract_word(start);
				s += 2;
			} else {
				/* > */
				if (*(s + 1) && isspace((unsigned char)*(s + 1))) {
					start = s + 2;
				} else {
					start = s + 1;
				}

				if (redirs[REDIR_STDOUT_IDX]) {
					free(redirs[REDIR_STDOUT_IDX]);
					redirs[REDIR_STDOUT_IDX] = NULL;
				}
				if (redirs[APPEND_STDOUT_IDX]) {
					free(redirs[APPEND_STDOUT_IDX]);
					redirs[APPEND_STDOUT_IDX] = NULL;
				}

				redirs[REDIR_STDOUT_IDX] = extract_word(start);
				s++;
			}
		} else if (*s == '<') {
			/* < */
			if (*(s + 1) && isspace((unsigned char)*(s + 1))) {
				start = s + 2;
			} else {
				start = s + 1;
			}

			if (redirs[REDIR_STDIN_IDX]) {
				free(redirs[REDIR_STDIN_IDX]);
				redirs[REDIR_STDIN_IDX] = NULL;
			}

			redirs[REDIR_STDIN_IDX] = extract_word(start);
			s++;
		} else {
			s++;
		}
	}

	return redirs;
}

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
		fprintf(stderr, "+ %s %s \n", tokens[COMMAND_IDX],
		        tokens[COMMAND_TARGET_IDX]);
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

void
remove_redir_tokens(char **tokens, int token_cnt)
{
	int remove_count, i, j;

	for (i = 0; i < token_cnt; i++) {
		if ((strncmp(tokens[i], ">", 1) == 0) ||
		    (strncmp(tokens[i], ">>", 2) == 0) ||
		    (strncmp(tokens[i], "<", 1) == 0)) {
			remove_count = 1;
			if (i + 1 < token_cnt) {
				remove_count = 2;
			}

			for (j = i; j + remove_count < token_cnt; j++) {
				tokens[j] = tokens[j + remove_count];
			}
			token_cnt -= remove_count;
			tokens[token_cnt] = NULL;
			i--;
		}
	}
}

/* place spaces between redirect operators if non-existent */
char *
normalize_command(char *cmd)
{
	size_t len, out_i;
	char *out;

	len = strlen(cmd);
	out = malloc(len * 3 + 1);
	if (!out) {
		return NULL;
	}

	out_i = 0;
	for (size_t i = 0; i < len; i++) {
		if (cmd[i] == '>') {
			if (i + 1 < len && cmd[i + 1] == '>') {
				if (out_i > 0 && !isspace((unsigned char)out[out_i - 1])) {
					out[out_i++] = ' ';
				}
				out[out_i++] = '>';
				out[out_i++] = '>';
				i++;
				if (i + 1 < len && !isspace((unsigned char)cmd[i + 1])) {
					out[out_i++] = ' ';
				}
			} else {
				if (out_i > 0 && !isspace((unsigned char)out[out_i - 1])) {
					out[out_i++] = ' ';
				}
				out[out_i++] = '>';
				if (i + 1 < len && !isspace((unsigned char)cmd[i + 1])) {
					out[out_i++] = ' ';
				}
			}
		} else if (cmd[i] == '<') {
			if (out_i > 0 && !isspace((unsigned char)out[out_i - 1])) {
				out[out_i++] = ' ';
			}
			out[out_i++] = '<';
			if (i + 1 < len && !isspace((unsigned char)cmd[i + 1])) {
				out[out_i++] = ' ';
			}
		} else {
			out[out_i++] = cmd[i];
		}
	}
	out[out_i] = '\0';
	return out;
}

int
run_command_in_child(char *command)
{
	char **tokens, **redirs;
	char *norm_cmd, *in_file = NULL, *out_file = NULL, *cmd_src;
	int in_fd = -1, out_fd = -1, flags;
	bool append_mode = false;
	size_t token_cnt;

	if (strstr(command, ">") || strstr(command, "<")) {
		redirs = retrieve_redirects(command);
		if (redirs[REDIR_STDIN_IDX]) {
			in_file = redirs[REDIR_STDIN_IDX];
		}
		if (redirs[APPEND_STDOUT_IDX]) {
			out_file = redirs[APPEND_STDOUT_IDX];
			append_mode = true;
		} else if (redirs[REDIR_STDOUT_IDX]) {
			out_file = redirs[REDIR_STDOUT_IDX];
		}
	} else {
		redirs = NULL;
	}

	norm_cmd = normalize_command(command);
	tokens = split_str(norm_cmd, " \t\n", &token_cnt);
	free(norm_cmd);

	if (token_cnt == 0) {
		if (redirs) {
			free_redirs(redirs);
		}
		free_split_str(tokens);
		fprintf(stderr, "%s: empty command\n", getprogname());
		_exit(STATUS_CMD_FAILED);
	}

	cmd_src = tokens[0];

	if (strlen(cmd_src) == BUILTIN_CD_SZ &&
	    strncmp(cmd_src, BUILTIN_CD, BUILTIN_CD_SZ) == 0) {
		fprintf(stderr, "%s: cd not supported in pipeline\n", getprogname());
		_exit(STATUS_CMD_FAILED);
	}

	if (strlen(cmd_src) == BUILTIN_ECHO_SZ &&
	    strncmp(cmd_src, BUILTIN_ECHO, BUILTIN_ECHO_SZ) == 0) {
		if (builtin_echo(tokens, token_cnt) != 0) {
			fprintf(stderr, "%s: echo failed\n", getprogname());
			_exit(STATUS_CMD_FAILED);
		}
		_exit(0);
	}

	if (redirs) {
		remove_redir_tokens(tokens, (int)token_cnt);
	}

	if (token_cnt == 0) {
		if (redirs) {
			free_redirs(redirs);
		}
		free_split_str(tokens);
		fprintf(stderr, "%s: redirection operators require file args\n",
		        getprogname());
		_exit(STATUS_CMD_FAILED);
	}

	if (in_file) {
		in_fd = open(in_file, O_RDONLY);
		if (in_fd < 0) {
			perror(in_file);
			_exit(STATUS_CMD_FAILED);
		}
		dup2(in_fd, STDIN_FILENO);
		close(in_fd);
	}

	if (out_file) {
		flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);
		out_fd = open(out_file, flags, 0666);
		if (out_fd < 0) {
			perror(out_file);
			_exit(STATUS_CMD_FAILED);
		}
		dup2(out_fd, STDOUT_FILENO);
		close(out_fd);
	}

	if (tracing) {
		fprintf(stderr, "+ %s\n", command);
	}

	execvp(cmd_src, tokens);
	fprintf(stderr, "%s: %s: not found\n", getprogname(), cmd_src);
	_exit(STATUS_CMD_FAILED);
}

int
exec_command(char *command)
{
	pid_t pid;
	int status;
	bool background = false;
	char *norm_cmd, **tokens;
	size_t token_cnt;

	if (strlen(command) == BUILTIN_EXIT_SZ &&
	    strncmp(command, BUILTIN_EXIT, BUILTIN_EXIT_SZ) == 0) {
		exit_flag = 1;
		last_exit_status = EXIT_SUCCESS;
		if (tracing) {
			fprintf(stderr, "+ %s\n", command);
		}
		return 0;
	}

	norm_cmd = normalize_command(command);
	tokens = split_str(norm_cmd, " \t\n", &token_cnt);
	free(norm_cmd);

	if (token_cnt == 0) {
		free_split_str(tokens);
		return 0;
	}

	if (strcmp(tokens[token_cnt - 1], "&") == 0) {
		background = true;
		tokens[token_cnt - 1] = NULL;
		token_cnt--;
	}

	if (token_cnt > 0 && strlen(tokens[0]) == BUILTIN_CD_SZ &&
	    strncmp(tokens[0], BUILTIN_CD, BUILTIN_CD_SZ) == 0) {
		last_exit_status = builtin_cd(tokens, token_cnt);
		free_split_str(tokens);
		return last_exit_status == 0 ? 0 : -1;
	}

	if (token_cnt > 0 && strlen(tokens[0]) == BUILTIN_ECHO_SZ &&
	    strncmp(tokens[0], BUILTIN_ECHO, BUILTIN_ECHO_SZ) == 0) {
		last_exit_status = builtin_echo(tokens, token_cnt);
		free_split_str(tokens);
		return last_exit_status == 0 ? 0 : -1;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork failed");
		free_split_str(tokens);
		return -1;
	}
	if (pid == 0) {
		run_command_in_child(command);
	}
	free_split_str(tokens);

	if (background) {
		printf("%d\n", (int)pid);
		last_exit_status = 0;
		return 0;
	} else {
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid failed");
			return -1;
		}
		if (WIFEXITED(status)) {
			last_exit_status = WEXITSTATUS(status);
		} else {
			last_exit_status = STATUS_CMD_FAILED;
		}
		return (last_exit_status == 0) ? 0 : -1;
	}
}

void
exec_commands(char **commands, int cmd_cnt)
{
	bool background = false;
	int i, j, status;
	int *pipes;
	int final_status = 0;
	pid_t pid;

	if (cmd_cnt < 1) {
		return;
	}

	if (cmd_cnt == 1) {
		char *cmd = commands[0];
		size_t len = strlen(cmd);
		if (len > 0 && cmd[len - 1] == '&') {
			background = true;
			commands[0][len - 1] = '\0';
			while (len > 0 && isspace((unsigned char)cmd[len - 1])) {
				commands[0][len - 1] = '\0';
				len--;
			}
		}
		if (!background) {
			exec_command(commands[0]);
		} else {
			pid = fork();
			if (pid < 0) {
				perror("fork failed");
				return;
			}
			if (pid == 0) {
				run_command_in_child(commands[0]);
			}
			printf("%d\n", (int)pid);
			last_exit_status = 0;
		}
		return;
	}

	pipes = malloc(sizeof(int) * 2 * (cmd_cnt - 1));
	if (!pipes) {
		return;
	}

	for (i = 0; i < cmd_cnt - 1; i++) {
		if (pipe(&pipes[i * 2]) < 0) {
			perror("pipe");
			free(pipes);
			return;
		}
	}

	for (i = 0; i < cmd_cnt; i++) {
		pid = fork();
		if (pid < 0) {
			perror("fork");
			free(pipes);
			return;
		}
		if (pid == 0) {
			if (i > 0) {
				dup2(pipes[(i - 1) * 2], STDIN_FILENO);
			}
			if (i < cmd_cnt - 1) {
				dup2(pipes[i * 2 + 1], STDOUT_FILENO);
			}
			for (j = 0; j < 2 * (cmd_cnt - 1); j++) {
				close(pipes[j]);
			}
			run_command_in_child(commands[i]);
		} else {
			if (i == cmd_cnt - 1 &&
			    commands[i][strlen(commands[i]) - 1] == '&') {
				background = true;
				commands[i][strlen(commands[i]) - 1] = '\0';
				while (
					strlen(commands[i]) > 0 &&
					isspace(
						(unsigned char)commands[i][strlen(commands[i]) - 1])) {
					commands[i][strlen(commands[i]) - 1] = '\0';
				}
				printf("%d\n", (int)pid);
			}
		}
	}

	for (i = 0; i < 2 * (cmd_cnt - 1); i++) {
		close(pipes[i]);
	}
	free(pipes);

	if (!background) {
		for (i = 0; i < cmd_cnt; i++) {
			if (wait(&status) == -1) {
				perror("wait");
				final_status = STATUS_CMD_FAILED;
			} else {
				if (WIFEXITED(status)) {
					int s = WEXITSTATUS(status);
					if (s != 0 && final_status == 0) {
						final_status = s;
					}
				} else {
					final_status = STATUS_CMD_FAILED;
				}
			}
		}
		last_exit_status = final_status;
	}
}
