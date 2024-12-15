#ifndef EXTERN_H
#define EXTERN_H

#include <signal.h>
#include <stdbool.h>

/* global commandline arg variables */
extern char *c_cmd;
extern bool tracing;

/* signal var */
extern volatile sig_atomic_t sig_recv;

/** opts.c **/
void get_options(int argc, char **argv);

/** sighandlers.c **/
int install_handlers();

#endif