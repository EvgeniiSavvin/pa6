#define _DEFAULT_SOURCE

#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "ipc.h"
#include "forklib.h"

ssize_t send_low (int descriptor, const Message * msg);
ssize_t receive_low (int descriptor, Message * msg);

int send (void * self, local_id dst, const Message * msg)
{
	context_t* ctx = self;
	incrementContextTime(ctx);
	((Message*)(void*)msg)->s_header.s_local_time = ctx->time;
	if (send_low(ctx->outpipes[dst], msg) < 0)
		return -1;
	return 0;
}

int send_multicast (void * self, const Message * msg)
{
	context_t* ctx = self;
	for (local_id i = 0; i < ctx->processes; i++) {
		if (i != ctx->id) {
			if (send(self, i, msg) != 0) {
				return -1;
			}
		}
	}
	return 0;
}

int receive(void * self, local_id from, Message * msg)
{
	context_t* ctx = self;
	if (receive_low(ctx->inpipes[from], msg) == -1) {
		return -1;
	}
	updateContextTimeFromMessage(ctx, msg);
	return 0;
}

int receive_any(void * self, Message * msg)
{
	context_t* ctx = self;
	int unread;
	for (local_id i = 0; i < ctx->processes; i++) {
		ioctl(ctx->inpipes[i], FIONREAD, &unread);
		if (unread > 0) {
			if (receive(self, i, msg) != 0) {
				return -1;
			}
			return 0;
		}
	}
	return 1;
}

ssize_t send_low (int descriptor, const Message * msg)
{
	return write(descriptor, msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
}

ssize_t receive_low (int descriptor, Message * msg)
{
	Message buffer;
    ssize_t result;

	while (1){
        result = read(descriptor, &buffer, sizeof(MessageHeader));
        if (result > 0) break;
        if (result <= 0) return -1;
    }
	ssize_t length = buffer.s_header.s_payload_len;
	if (length > 0) {
		while (read(descriptor, &buffer.s_payload, length) <= 0);
	}

	memcpy(msg, &buffer, sizeof(MessageHeader) + length);
	return sizeof(MessageHeader) + length;
}
