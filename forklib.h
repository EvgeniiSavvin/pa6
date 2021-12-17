#ifndef FORKLIB_H
#define FORKLIB_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ipc.h"
#include "pa2345.h"

typedef struct {
	char hold;
	char dirty;
	char request;
	char done;
} fork_t;

typedef struct {
	local_id id;
	local_id processes;
	local_id done;
	int* inpipes;
	int* outpipes;
	fork_t* forks;
	unsigned time;
} context_t;


pid_t doFork(context_t ctx, void (*job)(context_t*));
void setupContext(context_t* ctx, local_id id, int numberOfProcesses);
void useContextPipes(context_t* ctx, int* readPipeMatrix, int* writePipeMatrix);
void freeContext(context_t* ctx);

void updateContextTimeFromMessage(context_t* ctx, Message* msg);
void incrementContextTime(context_t* ctx);

int processRequests(context_t* ctx);

void printForkStatus(context_t* ctx);

extern FILE* logEvents;
extern FILE* logPipes;
extern int* readPipeMatrix;
extern int* writePipeMatrix;

#endif // FORKLIB_H
