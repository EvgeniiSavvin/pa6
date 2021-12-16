#ifndef MAIN_H
#define MAIN_H

#define _DEFAULT_SOURCE

#include <unistd.h> // for pipe(), getopt()
#include <getopt.h>
#include <fcntl.h>
#include <stdlib.h> // for strtol()
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "forklib.h"

typedef struct {
	int processes;
	char mutexl;
} options;

void getOptions (options* opts, int argc, char* argv[]);

#endif // MAIN_H
