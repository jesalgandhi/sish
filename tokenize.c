

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extern.h"

void
free_split_str(char **s)
{
	size_t i;
	for (i = 0; s[i] != NULL; i++) {
		free(s[i]);
	}
	free(s);
}

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