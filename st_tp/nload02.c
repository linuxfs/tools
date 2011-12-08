/*
 * PFS1node100�ץ�����٥ƥ��� - make test
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
#include <dirent.h>
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
#define IO_SIZE 32768
#define IO_COUNT 128

/* �ץ����ꥹ�ȹ�¤�� */
typedef struct proc_list {
	pid_t     pid;                    /* child pid */
	struct timeval start;             /* copy starttime */
	struct proc_list *next;   /* next list */
} proc_t;

/* �ꥹ�Ȥ���Ƭ�ϻ��Ȳս꤬¿���Τǥ����Х�ˤ��ޤ� */
proc_t *header_proc = NULL;
sigset_t block_mask;
fd_set mask;

/** usage */
PRIVATE void usage(void)
{
	fprintf(stderr, "usage: nload directry\n");
	exit(1);
}

/** syslog�˽��� */
void logging(int prio, const char *fmt, ...)
{
	va_list ap;
	char buf[256];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	fprintf(stdout, "[%d] %s\n", prio, buf);
}

/** proc�ꥹ�Ȥ�PID�Ȱ��פ���ꥹ�Ȥ��֤���̵���ä���NULL���֤� */
PRIVATE proc_t *list_proc_pid(pid_t pid)
{
	proc_t *pos;
	for (pos = header_proc; (pos != NULL) && (pos->pid != pid); pos = pos->next);
	return pos;
}

/** ��ʬ���ꥹ�Ȥ���Ƭ����ɤΰ��֤ˤ��뤫�򸡺��������Σ������Υݥ��󥿤��֤� */
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

/** ����list�򥳥ԡ��ꥹ�Ȥ��������� */
void list_del_proc(proc_t *list)
{
	proc_t *prev;

	/* ������룱�ļ�����õ�� */
	prev = list_prev_address(list);
	/* ������̵�����ϼ�ʬ����Ƭ�ǡ���ʬ������оݤʤΤ����顢���Υꥹ�Ȥ���Ƭ�ˤʤ� */
	if (!prev) {
		header_proc = list->next;
	} else {
		prev->next = list->next;
	}
	free(list);
}

/** �ꥹ�Ȥ��ɲä��ޤ� */
PRIVATE proc_t *list_add_proc(void)
{
	proc_t *new;

	/* �ɲä���ꥹ�ȤΥ������� */
	if ((new = (proc_t *)calloc(1, sizeof(proc_t))) == NULL)
		return NULL;

	new->next = header_proc;
	header_proc = new;
	return new;
}

/** SIG_CHILD�ν��� */
PRIVATE void sigchild_proc(pid_t pid)
{
	proc_t *pos;
	pos = list_proc_pid(pid);
	if (!pos) {
		return;
	}
	list_del_proc(pos);
}

/** �����ʥ�ϥ�ɥ� */
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

/** �����ʥ����� */
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

/** ���ץ������� */
PRIVATE int init_option(int argc, char **argv, char *path)
{
	if (argc < 2)
		usage();
	strncpy(path, argv[1], PATH_LEN);
	return 0;
}


/* readdir test */
PRIVATE int finddir(const char *base)
{
	int ret;
	DIR *dp;
	struct dirent *dir;

	/* OPENDIR����ʤ��Τϵߤ��ʤ��Τǥ��顼 */
	if (!(dp = opendir(base))) {
		ret = errno;
		logging(LOG_ERR, "ERR: opendir %s %s[%d]", base, strerror(ret), ret);
		return - ret;
	}

	/* readdir�Υ���ȥ꤬NULL�ˤʤ�ޤ� */
	while ((dir = readdir(dp)));

	closedir(dp);
	return 0;
}

/* opentest */
PRIVATE int child_test(const char *base)
{
	char path[5][PATH_LEN + 1];
	int ret, fd;

	snprintf(path[0], PATH_LEN, "%s/nload-file.%d", base, getpid());
	snprintf(path[1], PATH_LEN, "%s/nload-rename.%d", base, getpid());
	snprintf(path[2], PATH_LEN, "%s/nload-dir.%d", base, getpid());
	snprintf(path[3], PATH_LEN, "%s/nload-dir.%d/nload", base, getpid());
	snprintf(path[4], PATH_LEN, "%s/nload-dir.%d/nload2", base, getpid());

	/* open */
	if ((fd = open(path[0], O_RDWR|O_CREAT, 0666)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't open %s %s[%d]", path[0], strerror(ret), ret);
		return - ret;
	}

	/* chmod */
	if (fchmod(fd, 0777) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't chmod %s %s[%d]", path[0], strerror(ret), ret);
		return - ret;
	}

	/* rename */
	if (rename(path[0], path[1]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't rename %s to %s %s[%d]", path[0], path[1], strerror(ret), ret);
		return - ret;
	}

	/* opendir & find */
	if ((ret = finddir(base)) < 0)
		 return ret;

	/* unlink */
	if (unlink(path[1]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: unlink %s %s[%d]", path[1], strerror(ret), ret);
		return - ret;
	}
	close(fd);

	/* mkdir */
	if (mkdir(path[2], 0700) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: ll_mkdir mkdir error %s %s[%d]", path[2], strerror(ret), ret);
		return - ret;
	}

	/* chmod */
	if (chmod(path[2], 0777) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't chmod %s %s[%d]", path[2], strerror(ret), ret);
		return - ret;
	}

	/* open */
	if ((fd = open(path[3], O_RDWR|O_CREAT, 0666)) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't open %s %s[%d]", path[3], strerror(ret), ret);
		return - ret;
	}

	/* chmod */
	if (fchmod(fd, 0777) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't chmod %s %s[%d]", path[3], strerror(ret), ret);
		return - ret;
	}

	/* rename */
	if (rename(path[3], path[4]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't rename %s to %s %s[%d]", path[4], path[5], strerror(ret), ret);
		return - ret;
	}

	/* unlink */
	if (unlink(path[4]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: unlink %s %s[%d]", path[5], strerror(ret), ret);
		return - ret;
	}
	close(fd);

	/* mkdir */
	if (mkdir(path[3], 0700) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: ll_mkdir mkdir error %s %s[%d]", path[3], strerror(ret), ret);
		return - ret;
	}

	/* chmod */
	if (chmod(path[3], 0777) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't chmod %s %s[%d]", path[4], strerror(ret), ret);
		return - ret;
	}

	/* rename */
	if (rename(path[3], path[4]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: can't rename %s to %s %s[%d]", path[4], path[5], strerror(ret), ret);
		return - ret;
	}

	/* rmdir */
	if (rmdir(path[4]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: rmdir error %s %s[%d]", path[5], strerror(ret), ret);
		return - ret;
	}

	/* rmdir */
	if (rmdir(path[2]) < 0) {
		ret = errno;
		logging(LOG_ERR, "ERR: rmdir error %s %s[%d]", path[3], strerror(ret), ret);
		return - ret;
	}
	return 0;
}

/* fork���� */
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
		/* �Ҷ��Υץ��� */
		exit(-child_test(base));
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

/** ��������ʬ���������פδؿ��Ǥ�  */
PRIVATE int main_proc(const char *base)
{
	int i, ret;

	/* �ᥤ��롼�� */
	for (i = 0; i < PROCESS; i++) {
		if ((ret == proc(base)) < 0) {
			logging(LOG_ERR, "ERR: catch Exception");
			return ret;
		}
	}
	return 0;
}

/** �ᥤ�������� */
int main(int argc, char **argv)
{
	char base[PATH_LEN + 1];

	/* �����ʥ����ꡢ���ץ���������listen���� */
	if (init_signal() < 0) return 1;
	if (init_option(argc, argv, base) < 0) return 1;
	if (main_proc(base) < 0) return 1;
	return 0;
}
