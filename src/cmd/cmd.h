#include <bits/ioctl/tty.h>

struct heap {
	void* brk;
	void* ptr;
	void* end;
};

struct outbuf {
	char* buf;
	int ptr;
	int len;
};

struct tabtab {
	void* buf;
	int size;
	int ptr;
	int idx;
	int count;
};

struct history {
	char* buf;
	int size;

	int len;
	int cur;

	int temp;
};

struct top {
	int argc;
	char** argv;
	char** envp;

	struct heap heap;
	struct outbuf out;
	struct tabtab tts;
	struct history hst;

	int sigfd;
	int cols;
	struct termios tso;
	struct termios tsi;

	char* buf;    /* ~/foo/bar > cat something.tx...........  */
	int sep;      /*         sep^             ptr^       max^ */
	int cur;
	int ptr;
	int max;

	int show;
	int ends;
	int viswi;
	int redraw;

	int esc;
	int tab;
};

#define CTX struct top* ctx __unused

void exit(CTX, int code) noreturn;
void quit(CTX, const char* msg, char* arg, int err) noreturn;

void init_input(CTX);
void fini_input(CTX);
void update_winsz(CTX);
int handle_input(CTX, char* buf, int len);

void prep_prompt(CTX);
void insert(CTX, char* inp, int len);
void replace(CTX, char* buf, int len);

void parse(CTX, char* buf, int len);
void execute(CTX, int argc, char** argv);

int extend(CTX, int len);
void* alloc(CTX, int len);

void single_tab(CTX);
void double_tab(CTX);
void cancel_tab(CTX);
void list_cwd(CTX);

void hist_prev(CTX);
void hist_next(CTX);
void hist_store(CTX);
