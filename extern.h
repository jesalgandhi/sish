#ifndef EXTERN_H
#define EXTERN_H

#include <stdbool.h>

/* global commandline arg variables */
extern char *c_cmd;
extern bool tracing;

/** opts.c **/
void get_options(int argc, char **argv);


#endif