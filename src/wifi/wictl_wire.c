#include <bits/socket/unix.h>
#include <sys/open.h>
#include <sys/read.h>
#include <sys/socket.h>
#include <sys/connect.h>

#include <nlusctl.h>
#include <util.h>
#include <heap.h>
#include <fail.h>

#include "config.h"
#include "wictl.h"

/* Socket init is split in two parts: socket() call is performed early so
   that it could be used to resolve netdev names into ifis, but connection
   is delayed until send_command() to avoid waking up wimon and then dropping
   the connection because of a local error. */

void init_heap_socket(struct top* ctx)
{
	int fd;

	hinit(&ctx->hp, PAGE);
	uc_buf_set(&ctx->uc, ctx->cbuf, sizeof(ctx->cbuf));

	if((fd = syssocket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		fail("socket", "AF_UNIX", fd);

	ctx->fd = fd;
}

static void connect_socket(struct top* ctx)
{
	int ret;

	struct sockaddr_un addr = {
		.family = AF_UNIX,
		.path = WICTL
	};

	if((ret = sysconnect(ctx->fd, &addr, sizeof(addr))) < 0)
		fail("connect", WICTL, ret);

	ctx->connected = 1;
}

static int heap_left(struct top* ctx)
{
	return ctx->hp.end - ctx->hp.ptr;
}

static void send_command(struct top* ctx)
{
	int wr, fd = ctx->fd;
	char* txbuf = ctx->uc.brk;
	int txlen = ctx->uc.ptr - ctx->uc.brk;

	uc_put_end(&ctx->uc);

	if(!ctx->connected)
		connect_socket(ctx);

	if((wr = writeall(fd, txbuf, txlen)) < 0)
		fail("write", NULL, wr);
}

static struct ucmsg* recv_reply(struct top* ctx)
{
	int rd, fd = ctx->fd;
	char* rxbuf = ctx->hp.ptr;
	struct ucmsg* msg = NULL;

	while(1) {
		char* rbuf = ctx->hp.ptr;
		int rlen = heap_left(ctx);

		if(rlen < PAGE/4) {
			hextend(&ctx->hp, PAGE);
			rlen = heap_left(ctx);
		};

		if((rd = sysread(fd, rbuf, rlen)) < 0)
			fail("recv", NULL, rd);
		else if(rd == 0)
			break;

		ctx->hp.ptr += rd;

		char* rxend = ctx->hp.ptr;
		int rxlen = rxend - rxbuf;

		if((msg = uc_msg(rxbuf, rxlen)))
			break;
	}

	if(!msg)
		fail("recv", "incomplete message", 0);

	return msg;
}

struct ucmsg* send_recv(struct top* ctx)
{
	send_command(ctx);
	return recv_reply(ctx);
}

struct ucmsg* send_check(struct top* ctx)
{
	struct ucmsg* msg;

	msg = send_recv(ctx);

	if(msg->cmd < 0)
		fail(NULL, NULL, msg->cmd);
	if(msg->cmd > 0)
		fail("unexpected notification", NULL, 0);

	return msg;
}

void send_check_empty(struct top* ctx)
{
	struct ucmsg* msg = send_check(ctx);

	if(msg->len > sizeof(msg))
		fail("unexpected reply data", NULL, 0);
}

void init_output(struct top* ctx)
{
	int len = 2048;

	ctx->bo.fd = STDOUT;
	ctx->bo.buf = halloc(&ctx->hp, len);
	ctx->bo.len = len;
	ctx->bo.ptr = 0;
}

void fini_output(struct top* ctx)
{
	bufoutflush(&ctx->bo);
}

void output(struct top* ctx, char* buf, int len)
{
	bufout(&ctx->bo, buf, len);
}
