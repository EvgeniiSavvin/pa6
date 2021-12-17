#include "pa2345.h"
#include "forklib.h"

int request_cs(const void * self)
{
    local_id held;
	context_t* ctx = (context_t*)self;
	fork_t* forks = ctx->forks;
	MessageHeader msg_raw;
	Message* msg = (Message*)&msg_raw;
	msg_raw.s_magic = MESSAGE_MAGIC;
	msg_raw.s_payload_len = 0;
	msg_raw.s_type = CS_REQUEST;
    while(1) {

        while(processRequests(ctx) != 0);

        held = 0;
        for (local_id i = 1; i < ctx->processes; i++) {
            if (forks[i].hold == 1 || forks[i].done == 1) {
                held++;
            }
        }

        if (held == ctx->processes - 1)
            return 0;

        for (local_id i = 1; i < ctx->processes; i++) {
            if (i != ctx->id) {
                if (forks[i].hold == 0 && forks[i].request == 1) {
                    if (send(ctx, i, msg) != 0) {
                        return -1;
                    }
                    forks[i].request = 0;
                }
            }
        }

    }
}

int release_cs(const void * self)
{
    context_t* ctx = (context_t*)self;
    for (local_id i = 1; i < ctx->processes; i++) {
        ctx->forks[i].dirty = 1;
    }
	return 0;
}
