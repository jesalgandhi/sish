#ifndef EXTERN_H
#define EXTERN_H

#include <signal.h>
#include <stdbool.h>

/* global commandline arg variables */
extern char *c_cmd;
extern bool tracing;

/* other globals */
extern bool exit_flag;
extern int last_exit_status;

/* signal var */
extern volatile sig_atomic_t sig_recv;

/** opts.c **/
void get_options(int argc, char **argv);

/** sighandlers.c **/
int install_handlers();

/** tokenize.c **/
void free_split_str(char **s);
char **split_str(char *str, const char *delimiters, size_t *count);

/** exec.c **/
int exec_command(char *command);
void exec_commands(char **commands, int cmd_cnt);

#endif