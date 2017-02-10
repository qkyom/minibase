#include <bits/socket.h>
#include <bits/socket/unix.h>
#include <bits/errno.h>
#include <bits/fcntl.h>
#include <bits/major.h>
#include <sys/recv.h>
#include <sys/socket.h>
#include <sys/listen.h>
#include <sys/bind.h>
#include <sys/open.h>
#include <sys/fstat.h>
#include <sys/write.h>
#include <sys/close.h>

#include <format.h>
#include <string.h>
#include <fail.h>

#include "vtmux.h"

static void reply(int fd, int errno, char* msg)
{
	char repbuf[256];

	char* p = repbuf;
	char* e = repbuf + sizeof(repbuf);

	if(errno)
		p = fmtint(p, e, -errno);
	if(errno && msg)
		p = fmtstr(p, e, " ");
	if(msg)
		p = fmtstr(p, e, msg);

	syswrite(fd, repbuf, p - repbuf);
}

static struct vtd* grab_dev_slot(void)
{
	int i;

	for(i = 0; i < nvtdevices; i++)
		if(vtdevices[i].fd <= 0)
			break;
	if(i >= INPUTS)
		return NULL;
	if(i == nvtdevices)
		nvtdevices++;

	return &vtdevices[i];
}

static int check_managed_dev(int fd, int* dev, int tty)
{
	struct stat st;
	long ret;

	if((ret = sysfstat(fd, &st)) < 0)
		return ret;

	int maj = major(st.st_rdev);
	int fmt = st.st_mode && S_IFMT;

	if(fmt != S_IFCHR)
		return -EACCES;
	if(maj != DRI_MAJOR && maj != INPUT_MAJOR)
		return -EACCES;

	*dev = st.st_rdev;

	if(*dev != st.st_rdev)
		return -EINVAL; /* 64-bit rdev, drop it */

	return 0;
}

/* Device id is used as a key for cmd_close, so reject requests
   to open the same device twice. For DRI devices, multiple fds
   would also mess up mastering. Inputs would be ok, but there's
   still no point in opening them more than once. */

static int check_for_duplicate(struct vtd* vd, int dev, int tty)
{
	int i;

	for(i = 0; i < nvtdevices; i++) {
		struct vtd* vx = &vtdevices[i];

		if(vx == vd)
			continue;
		if(vx->tty != tty)
			continue;
		if(vx->dev != dev)
			continue;

		return -ENFILE;
	}

	return 0;
}

static void cmd_open(int rfd, char* arg, int tty)
{
	struct vtd* vd;
	int dfd, ret;

	if(!(vd = grab_dev_slot()))
		return reply(rfd, -EMFILE, NULL);

	if((dfd = sysopen(arg, O_RDWR | O_NOCTTY | O_CLOEXEC)) < 0)
		return reply(rfd, dfd, NULL);

	if((ret = check_managed_dev(dfd, &vd->dev, tty)) < 0)
		goto close;
	if((ret = check_for_duplicate(vd, vd->dev, tty)) < 0)
		goto close;

	vd->fd = dfd;
	vd->tty = tty;
	return reply(rfd, 0, NULL);
close:
	vd->dev = 0;
	sysclose(dfd);
	return reply(rfd, ret, NULL);
}

static struct vtd* find_device_slot(int tty, int dev)
{
	int i;

	for(i = 0; i < nvtdevices; i++) {
		struct vtd* vx = &vtdevices[i];

		if(vx->tty != tty)
			continue;
		if(vx->dev != dev)
			continue;

		return vx;
	};

	return NULL;
}

static void cmd_close(int rfd, char* arg, int tty)
{
	int dev;
	char* p;
	struct vtd* vd;

	if(!(p = parseint(arg, &dev)) || *p)
		return reply(rfd, -EINVAL, NULL);

	if(!(vd = find_device_slot(tty, dev)))
		return reply(rfd, -EBADF, NULL);

	disable(vd, 1);

	if(vd == &vtdevices[nvtdevices-1])
		nvtdevices--;

	return reply(rfd, 0, NULL);
}

static void cmd_switch(int fd, char* arg)
{
	int vt;
	char* p;
	long ret;

	if(!(p = parseint(arg, &vt)) || *p)
		return reply(fd, -EINVAL, "Bad request");

	if((ret = switchto(vt)))
		return reply(fd, ret, NULL);

	return reply(fd, 0, NULL);
}

static void cmd_spawn(int fd, char* arg)
{
	long ret = spawn(arg);

	return reply(fd, ret, NULL);
}

/* Greeter is always ci=0 here. All other clients are only
   allowed to open/close fds, but greeter should be able
   to spawn clients and such. */

static void handlecmd(int ci, int fd, char* cmd)
{
	char* arg = cmd + 1;
	struct vtx* cvt = &consoles[ci];

	if(*cmd == '@')
		return cmd_open(fd, arg, cvt->tty);
	if(*cmd == '#')
		return cmd_close(fd, arg, cvt->tty);

	if(ci) goto out;

	if(*cmd == '=')
		return cmd_switch(fd, arg);
	if(*cmd == '+')
		return cmd_spawn(fd, arg);
out:
	return reply(fd, -EINVAL, "Invalid command");
}

/* For any sane clients, there should be exactly one pending
   command at a time, because the client is expected to wait
   for reply. But, the protocol does not prevent sending cmds
   in bulk, and dropping the loop here does not save us much. */

void handlectl(int ci, int fd)
{
	char cmdbuf[256];
	int cmdsize = sizeof(cmdbuf) - 1;
	int rd;

	while((rd = sysrecv(fd, cmdbuf, cmdsize, MSG_DONTWAIT)) > 0) {
		cmdbuf[rd] = '\0';
		handlecmd(ci, fd, cmdbuf);
	} if(rd < 0 && rd != -EAGAIN) {
		warn("recv", NULL, rd);
	}
}
