/*
 * PFS1node100プロセス負荷テスト - iotest
 * 2005.02.16  presented by Yoshihiro Takahashi
 *             initial commit
 */

#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE /* for strsignal */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h> /* isalpha */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h> /* wait */
#include <signal.h>
#include <fcntl.h>
#include <unistd.h> /* exit */
#include <syslog.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>

#ifndef PRIVATE
#define PRIVATE static
#endif
#define PROCESS 100
#define PATH_LEN 4096
#define IO_SIZE 16384
#define IO_COUNT 128

/* プロセスリスト構造体 */
typedef struct proc_list {
	pid_t     pid;                    /* child pid */
	struct timeval start;             /* copy starttime */
	struct proc_list *next;   /* next list */
} proc_t;

/* リストの先頭は参照箇所が多いのでグローバルにします */
proc_t *header_proc = NULL;
sigset_t block_mask;
fd_set mask;

/** usage */
PRIVATE void usage(void)
{
	fprintf(stderr, "usage: nload directry\n");
	exit(1);
}

/** syslogに出力 */
void logging(int prio, const char *fmt, ...)
{
	va_list ap;
	char buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	fprintf(stdout, "[%d] %s\n", prio, buf);
}

/** procリストのPIDと一致するリストを返す、無かったらNULLを返す */
PRIVATE proc_t *list_proc_pid(pid_t pid)
{
	proc_t *pos;
	for (pos = header_proc; (pos != NULL) && (pos->pid != pid); pos = pos->next);
	return pos;
}

/** 自分がリストの先頭からどの位置にいるかを検索し、その１個前のポインタを返す */
PRIVATE proc_t *list_prev_address(proc_t *list)
{
	proc_t *prev, *pos;
	prev = NULL;

	if (list == NULL) {
		return NULL;
	}
	for (pos = header_proc; (pos != NULL) && (pos != list); pos = pos->next)
		prev = pos;
	if (pos == NULL) {
		return NULL;
	}
	return prev;
}

/** 引数listをコピーリストから削除する */
void list_del_proc(proc_t *list)
{
	proc_t *prev;

	/* 削除する１個手前を探す */
	prev = list_prev_address(list);
	/* 手前が無い時は自分が先頭で、自分が削除対象なのだから、次のリストが先頭になる */
	if (!prev) {
		header_proc = list->next;
	} else {
		prev->next = list->next;
	}
	free(list);
}

/** リストを追加します */
PRIVATE proc_t *list_add_proc(void)
{
	proc_t *new;

	/* 追加するリストのメモリを確保 */
	if ((new = (proc_t *)calloc(1, sizeof(proc_t))) == NULL)
		return NULL;

	new->next = header_proc;
	header_proc = new;
	return new;
}

/** SIG_CHILDの処理 */
PRIVATE void sigchild_proc(pid_t pid)
{
	proc_t *pos;
	pos = list_proc_pid(pid);
	if (!pos) {
		return;
	}
	list_del_proc(pos);
}

/** シグナルハンドラ */
PRIVATE void do_signal(int signr)
{
	pid_t pid;
	switch (signr) {
	case SIGCHLD:
		while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
			sigchild_proc(pid);
		}
		return;
	case SIGTERM:
		logging(LOG_NOTICE, "NOTICE: catch TERMINATE signal");
		exit(0);
	default:
		logging(LOG_WARNING, "WARNING: unknown signal [%d]", signr);
		break;
	}
}

/** シグナル設定 */
PRIVATE int init_signal(void)
{
	int ret;
	struct sigaction act, pipes;
	sigset_t full_mask;

	if (sigemptyset(&block_mask)) {
		ret = errno;
		fprintf(stderr, "cannot clear signal mask %s[%d]\n", strerror(ret), ret);
		return - ret;
	}

	if (sigaddset(&block_mask, SIGHUP) ||  /* Terminal Hungup signal */
	    sigaddset(&block_mask, SIGTERM) || /* kill signal */
	    sigaddset(&block_mask, SIGCHLD)) { /* fork process exit */
		ret = errno;
		fprintf(stderr, "cannot set signal mask %s[%d]\n", strerror(ret), ret);
		return - ret;
	}

	if (sigfillset(&full_mask)) {
		ret = errno;
		fprintf(stderr, "cannot fill signal mask %s[%d]\n", strerror(ret), ret);
		return - ret;
	}

	if (sigdelset(&full_mask, SIGILL) ||
	    sigdelset(&full_mask, SIGTRAP) ||
	    sigdelset(&full_mask, SIGABRT) ||
	    sigdelset(&full_mask, SIGBUS) ||
	    sigdelset(&full_mask, SIGSEGV)) {
		ret = errno;
		fprintf(stderr, "cannot reset signal mask %s[%d]\n", strerror(ret), ret);
		return - ret;
	}

	if (sigprocmask(SIG_UNBLOCK, &full_mask, NULL)) {
		ret = errno;
		fprintf(stderr, "cannot set signal proc mask %s[%d]\n", strerror(ret), ret);
		return - ret;
	}

	act.sa_handler = do_signal;
	act.sa_flags = 0;
	act.sa_mask = block_mask;

	pipes.sa_handler = SIG_IGN;
	pipes.sa_flags = 0;
	pipes.sa_mask = block_mask;

	if (sigaction(SIGHUP,  &act, NULL) ||   /* Terminal Hungup signal */
	    sigaction(SIGTERM, &act, NULL) ||   /* kill signal */
	    sigaction(SIGCHLD, &act, NULL)) {   /* fork process exit */
		ret = errno;
		fprintf(stderr, "cannot set signal action %s[%d]\n", strerror(ret), ret);
		return - ret;
	}
	return 0;
}

/** オプション処理 */
PRIVATE int init_option(int argc, char **argv, char *path)
{
	if (argc < 2)
		usage();
	strncpy(path, argv[1], PATH_LEN);
	return 0;
}

/* 親プロセスの初期処理 */
PRIVATE int init_open_main(const char *base)
{
	char path[PATH_LEN + 1];
	int ret, fd;

	snprintf(path, PATH_LEN, "%s/nload.main", base);
	if ((fd = open(path, O_RDWR|O_CREAT, 0666)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't open %s %s[%d]", path, strerror(ret), ret);
		return - ret;
	}
	return fd;
}


/* 子プロセスの初期処理 */
PRIVATE int init_open(const char *base)
{
	char path[PATH_LEN + 1];
	int ret, fd;

	snprintf(path, PATH_LEN, "%s/nload.%d", base, getpid());
	if ((fd = open(path, O_RDWR|O_CREAT, 0666)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't open %s %s[%d]", path, strerror(ret), ret);
		return - ret;
	}
	return fd;
}

/* 子プロセスの初期処理 */
PRIVATE uint64_t *init_mem()
{
	uint64_t *buf;
	int i, ret, count = (IO_SIZE / sizeof(uint64_t));

	if ((buf = (uint64_t *)malloc(IO_SIZE)) == NULL) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't malloc %s[%d]", strerror(ret), ret);
		return NULL;
	}
	for (i = 0; i < count; i++)
		buf[i] = i;

	return buf;
}

/* 子プロセスのwrite処理 */
PRIVATE int ll_write(int fd, uint64_t *buf)
{
	int ret, i;
	size_t size;

	size = ((IO_SIZE * IO_COUNT) * (getpid() % PROCESS));
	if ((ret = lseek(fd, size, SEEK_SET)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't lseek SEEK_SET %d != %d %s[%d]", ret, size, strerror(ret), ret);
		close(fd);
		return - ret;
	}

	for (i = 0; i < IO_COUNT; i++) {
		if ((size = write(fd, buf, IO_SIZE)) != IO_SIZE) {
			ret = errno;
			logging(LOG_ERR, "ERR: can't write %d / %d %s[%d]", size, IO_SIZE, strerror(ret), ret);
			close(fd);
			return - ret;
		}
	}
	close(fd);
	return 0;
}

/* 子プロセスのread処理 */
PRIVATE int ll_read(int fd, uint64_t *buf)
{
	int ret, i, h;
	size_t size;

	size = ((IO_SIZE * IO_COUNT) * (getpid() % PROCESS));
	if ((ret = lseek(fd, size, SEEK_SET)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't lseek SEEK_SET %d != %d %s[%d]", ret, size, strerror(ret), ret);
		close(fd);
		return - ret;
	}
	for (i = 0; i < IO_COUNT; i++) {
		if ((size = read(fd, buf, IO_SIZE)) != IO_SIZE) {
			ret = errno;
			logging(LOG_ERR, "ERR: can't read %d / %d %s[%d]", size, IO_SIZE, strerror(ret), ret);
			close(fd);
			return - ret;
		}
		size = (IO_SIZE / sizeof(uint64_t));
		for (h = 0; h < (size); h++) {
			if (buf[h] != h) {
				logging(LOG_ERR, "ERR: wrong data!! buf[%d] = %d != %d", h, buf[h], h);
				close(fd);
				return - ENOSTR;
			}
		}
	}
	close(fd);
	return 0;
}

/* 子プロセス処理 */
PRIVATE void child_proc(const char *base)
{
	uint64_t *buf;
	int fd;

	for (;;) {
		/* 別ファイルへのI/O */
		if ((fd = init_open(base)) < 0) {
			sleep(300);
			continue;
		}
		if ((buf = init_mem()) == NULL) {
			close(fd);
			sleep(300);
			continue;
		}
		if (ll_write(fd, buf) < 0) {
			free(buf);
			sleep(60);
			continue;
		}
		if ((fd = init_open(base)) < 0) {
			free(buf);
			sleep(300);
			continue;
		}
		if (ll_read(fd, buf) < 0) {
			free(buf);
			sleep(60);
			continue;
		}
		free(buf);

		/* 同一ファイル別領域へのI/O */
		if ((fd = init_open_main(base)) < 0) {
			sleep(300);
			continue;
		}
		if ((buf = init_mem()) == NULL) {
			close(fd);
			sleep(300);
			continue;
		}
		if (ll_write(fd, buf) < 0) {
			free(buf);
			sleep(60);
			continue;
		}
		if ((fd = init_open(base)) < 0) {
			free(buf);
			sleep(300);
			continue;
		}
		if (ll_read(fd, buf) < 0) {
			free(buf);
			sleep(60);
			continue;
		}
		free(buf);
	}
	return;
}

/* fork処理 */
PRIVATE int proc(const char *base)
{
	proc_t *pos;
	int ret;

	if ((pos = list_add_proc()) == NULL) {
		logging(LOG_ERR, "ERR: can't make proc memory");
		return - ENOMEM;
	}
	pos->pid = fork();
	switch (pos->pid) {
	case 0:
		/* 子供のプロセス */
		child_proc(base);
		exit(0);
	case - 1:
		ret = errno;
		logging(LOG_ERR, "ERR: can't fork %s[%d]", strerror(errno), errno);
		list_del_proc(pos);
		return - ret;
	default:
		gettimeofday (&pos->start, NULL);
	}
	return 0;
}

/** 受信振り分け処理。要の関数です  */
PRIVATE int main_proc(const char *base)
{
	int i, fd, ret;

	if ((fd = init_open_main(base)) < 0) return fd;
	close(fd);
	/* メインループ */
	for (i = 0; i < PROCESS; i++) {
		if ((ret == proc(base)) < 0) {
			logging(LOG_ERR, "ERR: catch Exception");
			return ret;
		}
	}
	return 0;
}

/** メイン初期処理 */
int main(int argc, char **argv)
{
	char base[PATH_LEN + 1];

	/* シグナル設定、オプション処理、listen設定 */
	if (init_signal() < 0) return 1;
	if (init_option(argc, argv, base) < 0) return 1;
	if (main_proc(base) < 0) return 1;
	return 0;
}
