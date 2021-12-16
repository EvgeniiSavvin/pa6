#include "main.h"

extern char* optarg;

FILE* logPipes;
FILE* logEvents;
int* readPipeMatrix;
int* writePipeMatrix;

static options opts;
static context_t context;

void error_exit(char* cause);

void childJob();

int main(int argc, char* argv[])
{
	if (argc < 2) {
		error_exit("Process number must be set");
	}
	getOptions(&opts, argc, argv);
	//printf("processes: %d, mutexl: %d\n", opts.processes, opts.mutexl);
	opts.processes++;
	
	pid_t pids[opts.processes - 1];

	logPipes = fopen(pipes_log, "wt");
	logEvents = fopen(events_log, "wt");

	readPipeMatrix = malloc(sizeof(int) * opts.processes * opts.processes);
	writePipeMatrix = malloc(sizeof(int) * opts.processes * opts.processes);

	{
		int pipes[2];
		int p = 0;
		for (local_id i = 0; i < opts.processes; i++) {
			for (local_id j = 0; j < opts.processes; j++) {
				if (i == j) {
					readPipeMatrix[i*opts.processes + j] = -1;
					continue;
				}
				if (pipe(pipes) == -1) {
					error_exit("Could not create pipes");
				}
				fcntl(pipes[0], F_SETFL, O_NONBLOCK);
				fcntl(pipes[1], F_SETFL, O_NONBLOCK);
				readPipeMatrix[i*opts.processes + j] = pipes[0];
				writePipeMatrix[i*opts.processes + j] = pipes[1];
				//fprintf(stdout, "Opened pipe from %2d to %2d\n", i, j);
				p++;
			}
		}
		//fprintf(stdout, "Pipes opened: %d\n", p);
	}

	for(local_id i = 1; i < opts.processes; i++) {
		context_t ctx;
		ctx.id = i;
		ctx.processes = opts.processes;
		pids[i-1] = doFork(ctx, childJob);
	}
	//fputs("All children started\n", stdout);

	setupContext(&context, 0, opts.processes);
	useContextPipes(&context, readPipeMatrix, writePipeMatrix);
	Message* msg = malloc(MAX_MESSAGE_LEN);

	for(local_id i = 1; i < opts.processes; i++) {
		receive(&context, i, msg);
		receive(&context, i, msg);
		waitpid(pids[i-1], NULL, 0);
	}
	//fputs("All children terminated\n", stdout);
	freeContext(&context);
	free(msg);
	fclose(logEvents);
	fclose(logPipes);

	return 0;
}

void childJob(context_t* ctx)
{
	const int N = ctx->id * 5;
	char buffer[64];
	for (int n = 1; n <= N;) {
		if (opts.mutexl) {
			switch (processRequests(ctx)) {
				case -1: error_exit("send failed"); break;
				case -2: error_exit("receive failed"); break;
			}
			int request = request_cs(ctx);
			if (request != 0) {
				if (request < 0) {
					error_exit("send failed");
				}
				else {
					//error_exit("request failed");
				}
			}
			else {
				local_id held = 0;
				for (local_id i = 1; i < ctx->processes; i++) {
					if (ctx->forks[i].hold == 1) {
						held++;
					}
				}
				//~ printf("%3d: process %d has %d forks\n", ctx->time, ctx->id, held);
				if (held == ctx->processes - 1) {
					for (local_id i = 1; i < ctx->processes; i++) {
						ctx->forks[i].dirty = 1;
					}
					sprintf(buffer, log_loop_operation_fmt, ctx->id, n, N);
					print(buffer);
					//~ puts(buffer);
					n++;
					if (release_cs(ctx) < 0) {
						error_exit("send failed");
					}
				}
			}
		}
		else {
			sprintf(buffer, log_loop_operation_fmt, ctx->id, n, N);
			print(buffer);
			n++;
		}
	}
}

void error_exit(char* cause)
{
	fputs(cause, stderr);
	fputc('\n', stderr);
	exit(1);
}

void getOptions (options* opts, int argc, char* argv[])
{
	int opt;
	char* program_name = argv[0];
	const struct option lopts[] = {
		{.name = "mutexl", .has_arg = 0, NULL, 'm'},
		{NULL, 0, NULL, 0}
	};
	opts->mutexl = 0;
	while ((opt = getopt_long(argc, argv, "p:hm", lopts, NULL)) != -1) {
		switch (opt) {
		case 'p':
			errno = 0;
			opts->processes = strtol(optarg, NULL, 10);
			if (errno == ERANGE) {
				error_exit("Processes number value out of range");
			}
			if (errno == EINVAL) {
				error_exit("No value for processes number");
			}
			if (opts->processes < 0) {
				error_exit("Process number must be positive or 0");
			}
			break;
		case 'm':
			opts->mutexl = 1;
			break;
		case '?':
			fputs("Unrecognized option\n", stderr);
			// no break
		case 'h':
			printf("Usage: %s -p N\n\t-p N\tset number of child processes to N\n", program_name);
			exit(0);
			break;
		}
	}
}
