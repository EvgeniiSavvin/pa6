#include "pa2345.h"
#include "ipc.h"
#include "forklib.h"

int request_cs(const void * self)
{
	context_t* ctx = (void*)self;
	fork_t* forks = ctx->forks;
	MessageHeader msg_raw;
	Message* msg = (void*)&msg_raw;
	msg_raw.s_magic = MESSAGE_MAGIC;
	msg_raw.s_payload_len = 0;
	msg_raw.s_type = CS_REQUEST;
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
	return 0;
}

int release_cs(const void * self)
{
	return 0;
}
