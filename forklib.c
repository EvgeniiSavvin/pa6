#include "forklib.h"

void formStartMessage(Message *message, char *text);

void formDoneMessage(Message *message, char *text);

pid_t doFork(context_t ctx, void (*job)(context_t*)) {

    context_t context;

    setupContext(&context, ctx.id, ctx.processes);
    useContextPipes(&context, readPipeMatrix, writePipeMatrix);

    char buffer[MAX_PAYLOAD_LEN];
    Message *msg = malloc(MAX_MESSAGE_LEN);

    sprintf(buffer, log_started_fmt, context.time, context.id, getpid(), getppid(), 0);
    fputs(buffer, logEvents);
    fputs(buffer, stdout);
    formStartMessage(msg, buffer);
    send_multicast(&context, msg);
    for (local_id i = 1; i < context.processes; i++) {
        if (i != context.id) {
            receive(&context, i, msg);
            //~ printf("%3d: %d <--STARTED-- %d\n", context.time, context.id, i);
        }
    }
    sprintf(buffer, log_received_all_started_fmt, context.time, context.id);
    fputs(buffer, logEvents);
    fputs(buffer, stdout);

    job(&context);

    sprintf(buffer, log_done_fmt, context.time, context.id, 0);
    fputs(buffer, logEvents);
    fputs(buffer, stdout);
    formDoneMessage(msg, buffer);
    send_multicast(&context, msg);
    context.done++;

    while (context.done < context.processes - 1) {
        processRequests(&context);
    }

    sprintf(buffer, log_received_all_done_fmt, context.time, context.id);
    fputs(buffer, logEvents);
    fputs(buffer, stdout);

    freeContext(&context);
    free(msg);
    fclose(logEvents);
    fclose(logPipes);
    exit(0);
}

void useContextPipes(context_t *ctx, int *readPipeMatrix, int *writePipeMatrix) {
    for (local_id j = 0; j < ctx->processes; j++) {
        ctx->inpipes[j] = readPipeMatrix[j * ctx->processes + ctx->id];
        ctx->outpipes[j] = writePipeMatrix[ctx->id * ctx->processes + j];
    }
    for (local_id i = 0; i < ctx->processes; i++) {
        for (local_id j = 0; j < ctx->processes; j++) {
            if (i == j) {
                continue;
            }
            if (i != ctx->id) {
                close(writePipeMatrix[i * ctx->processes + j]);
            }
            if (j != ctx->id) {
                close(readPipeMatrix[i * ctx->processes + j]);
            }
        }
    }
    free(readPipeMatrix);
    free(writePipeMatrix);
}

void setupContext(context_t *ctx, local_id id, int numberOfProcesses) {
    ctx->id = id;
    ctx->time = 0;
    ctx->processes = numberOfProcesses;
    ctx->done = 0;
    ctx->inpipes = malloc(sizeof(int) * numberOfProcesses);
    ctx->outpipes = malloc(sizeof(int) * numberOfProcesses);
    ctx->forks = malloc(sizeof(fork_t) * numberOfProcesses);
    for (local_id i = 0; i < numberOfProcesses; i++) {
        if (i < id) {
            ctx->forks[i].hold = ctx->forks[i].dirty = ctx->forks[i].done = 0;
            ctx->forks[i].request = 1;
        }
        if (i >= id) {
            ctx->forks[i].hold = ctx->forks[i].dirty = 1;
            ctx->forks[i].request = ctx->forks[i].done = 0;
        }
    }
}

void freeContext(context_t *ctx) {
    for (local_id i = 0; i < ctx->processes; i++) {
        if (i != ctx->id) {
            close(ctx->inpipes[i]);
            close(ctx->outpipes[i]);
        }
    }
    free(ctx->inpipes);
    free(ctx->outpipes);
    free(ctx->forks);
    ctx->id = -1;
}

void formStartMessage(Message *message, char *text) {
    size_t length = strlen(text);
    message->s_header.s_magic = MESSAGE_MAGIC;
    message->s_header.s_payload_len = length;
    message->s_header.s_type = STARTED;
    memcpy(message->s_payload, text, length);
}

void formDoneMessage(Message *message, char *text) {
    size_t length = strlen(text);
    message->s_header.s_magic = MESSAGE_MAGIC;
    message->s_header.s_payload_len = length;
    message->s_header.s_type = DONE;
    memcpy(message->s_payload, text, length);
}

void updateContextTimeFromMessage(context_t *ctx, Message *msg) {
    if (ctx->time < msg->s_header.s_local_time)
        ctx->time = msg->s_header.s_local_time;
    ctx->time++;
}

void incrementContextTime(context_t *ctx) {
    ctx->time++;
}

void printForkStatus(context_t* ctx) {
    printf("process %d\n", ctx->id);
    for (char c = 0; c < 3; c++) {
        for (local_id i = 1; i < ctx->processes; i++) {
            char v;
            switch (c) {
                case 0:
                    v = ctx->forks[i].hold;
                    break;
                case 1:
                    v = ctx->forks[i].dirty;
                    break;
                case 2:
                    v = ctx->forks[i].request;
                    break;
            }
            printf("%1d ", v);
        }
        fputc('\n', stdout);
    }
}

int processRequests(context_t *ctx) {
    Message *msg = malloc(sizeof(MessageHeader) + 100);
    msg->s_header.s_magic = MESSAGE_MAGIC;
    msg->s_header.s_type = STARTED;
    msg->s_header.s_payload_len = 0;
    for (local_id i = 1; i < ctx->processes; i++) {
        if (i != ctx->id && ctx->forks[i].done != 1) {
            while (receive(ctx, i, msg) == 0) {
                switch (msg->s_header.s_type) {
                    case CS_REQUEST:
                        ctx->forks[i].request = 1;
                        break;
                    case CS_REPLY:
                        ctx->forks[i].hold = 1;
                        ctx->forks[i].dirty = 0;
                        break;
                    case DONE:
                        ctx->forks[i].done = 1;
                        ctx->done++;
                        break;
                }
            }
            if (ctx->forks[i].request == 1 && ctx->forks[i].hold == 1 && ctx->forks[i].dirty == 1) {
                ctx->forks[i].hold = ctx->forks[i].dirty = 0;
                msg->s_header.s_type = CS_REPLY;
                msg->s_header.s_payload_len = 0;
                if (send(ctx, i, msg) != 0) {
                    free(msg);
                    return -2;
                }
            }
        }
    }
    free(msg);
    return 0;
}
