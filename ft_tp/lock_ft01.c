#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#define LOCK_LOG "/var/pfs/lock_dump"
#define FILENAME "file"

struct unlock_t {
	uint64_t seek_pt;
	uint64_t len;
	struct unlock_t *next;
	char   pad[4];
};

int fd;
pid_t lockd_pid;
char buf[4095];
struct unlock_t *head = NULL;

/* UnLock */
int unlock(uint64_t seek_pt, uint64_t len)
{
	/* set Lock Table F_UNLCK */
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* UnLock */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
		return -1;
	}
	return 0;
}

void list_add(uint64_t seek_pt, uint64_t len)
{
	struct unlock_t *new;

	if ((new = (struct unlock_t *)calloc(1, sizeof(struct unlock_t))) == NULL) {
		perror("calloc");
		exit(1);
	}

	if (head) new->next = head;
	else new->next = NULL;
	head = new;

	new->seek_pt = seek_pt;
	new->len = len;
}

void list_del(void)
{
	struct unlock_t *pos;
	while (head) {
		pos = head->next;
		unlock(head->seek_pt, head->len);
		free(head);
		head = pos;
	}
}

/* open file */
void open_file()
{
	/* write read open */
	if ((fd = open(FILENAME, O_RDWR|O_CREAT, 0666)) < 0) {
		perror("OPEN");
		exit(1);
	}
}

/* write Lock */
int lock_w(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	list_add(seek_pt, len);
	return 0;
}

/* read Lock */
int lock_r(uint64_t seek_pt, uint64_t len)
{
	struct flock lock;
	lock.l_type = F_RDLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = len;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return -1;
	}
	list_add(seek_pt, len);
	return 0;
}

void usage(char *pg)
{
	printf("usage: %s pfs_lkmgr-pid\n", pg);
	exit(1);
}

void catlog()
{
	int ld;
	memset(buf, 0, 4095);

	kill(lockd_pid, SIGUSR1);

	sleep(1);
	if ((ld = open(LOCK_LOG, O_RDONLY, 0)) < 0) {
		perror("Can't open logfile");
		exit(1);
	}

	while (read(ld, buf, 4096) > 0) {
		printf("%s", buf);
	}
	close (ld);
	list_del();
	printf("\n");
	fflush(stdout);
}

void title()
{
	static int num = 1;
	printf("\nlock test %d\n", num++);
}

void test001_008()
{
	/* 001 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	catlog();

	/* 002 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	catlog();

	/* 003 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	catlog();

	/* 004 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	catlog();

	/* 005 */
	title();
	lock_r(8192, 4096);
	lock_r(0, 4096);
	catlog();

	/* 006 */
	title();
	lock_r(8192, 4096);
	lock_w(0, 4096);
	catlog();

	/* 007 */
	title();
	lock_w(8192, 4096);
	lock_r(0, 4096);
	catlog();

	/* 008 */
	title();
	lock_w(8192, 4096);
	lock_w(0, 4096);
	catlog();
}

void test009_016()
{
	/* 009 */
	title();
	lock_r(0, 8192);
	lock_r(8192, 4096);
	catlog();

	/* 010 */
	title();
	lock_r(0, 8192);
	lock_w(8192, 4096);
	catlog();

	/* 011 */
	title();
	lock_w(0, 8192);
	lock_r(8192, 4096);
	catlog();

	/* 012 */
	title();
	lock_w(0, 8192);
	lock_w(8192, 4096);
	catlog();

	/* 013 */
	title();
	lock_r(4096, 8192);
	lock_r(0, 4096);
	catlog();

	/* 014 */
	title();
	lock_r(4096, 8192);
	lock_w(0, 4096);
	catlog();

	/* 015 */
	title();
	lock_w(4096, 8192);
	lock_r(0, 4096);
	catlog();

	/* 016 */
	title();
	lock_w(4096, 8192);
	lock_w(0, 4096);
	catlog();
}

void test017_024()
{
	/* 017 */
	title();
	lock_r(0, 8192);
	lock_r(4096, 8192);
	catlog();

	/* 018 */
	title();
	lock_r(0, 8192);
	lock_w(4096, 8192);
	catlog();

	/* 019 */
	title();
	lock_w(0, 8192);
	lock_r(4096, 8192);
	catlog();

	/* 020 */
	title();
	lock_w(0, 8192);
	lock_w(4096, 8192);
	catlog();

	/* 021 */
	title();
	lock_r(0, 0);
	lock_r(4096, 0);
	catlog();

	/* 022 */
	title();
	lock_r(0, 0);
	lock_w(4096, 0);
	catlog();

	/* 023 */
	title();
	lock_w(0, 0);
	lock_r(4096, 0);
	catlog();

	/* 024 */
	title();
	lock_w(0, 0);
	lock_w(4096, 0);
	catlog();
}

void test025_032()
{
	/* 025 */
	title();
	lock_r(4096, 0);
	lock_r(0, 8192);
	catlog();

	/* 026 */
	title();
	lock_r(4096, 0);
	lock_w(0, 8192);
	catlog();

	/* 027 */
	title();
	lock_w(4096, 0);
	lock_r(0, 8192);
	catlog();

	/* 028 */
	title();
	lock_w(4096, 0);
	lock_w(0, 8192);
	catlog();

	/* 029 */
	title();
	lock_r(0, 0);
	lock_r(0, 4096);
	catlog();

	/* 030 */
	title();
	lock_r(0, 0);
	lock_w(0, 4096);
	catlog();

	/* 031 */
	title();
	lock_w(0, 0);
	lock_r(0, 4096);
	catlog();

	/* 032 */
	title();
	lock_w(0, 0);
	lock_w(0, 4096);
	catlog();
}

void test033_040()
{
	/* 033 */
	title();
	lock_r(0, 0);
	lock_r(0, 0);
	catlog();

	/* 034 */
	title();
	lock_r(0, 0);
	lock_w(0, 0);
	catlog();

	/* 035 */
	title();
	lock_w(0, 0);
	lock_r(0, 0);
	catlog();

	/* 036 */
	title();
	lock_w(0, 0);
	lock_w(0, 0);
	catlog();

	/* 037 */
	title();
	lock_r(0, 0);
	lock_r(4096, 4096);
	catlog();

	/* 038 */
	title();
	lock_r(0, 0);
	lock_w(4096, 4096);
	catlog();

	/* 039 */
	title();
	lock_w(0, 0);
	lock_r(4096, 4096);
	catlog();

	/* 040 */
	title();
	lock_w(0, 0);
	lock_w(4096, 4096);
	catlog();
}

void test041_048()
{
	/* 041 */
	title();
	lock_r(0, 4096);
	lock_r(0, 0);
	catlog();

	/* 042 */
	title();
	lock_r(0, 4096);
	lock_w(0, 0);
	catlog();

	/* 043 */
	title();
	lock_w(0, 4096);
	lock_r(0, 0);
	catlog();

	/* 044 */
	title();
	lock_w(0, 4096);
	lock_w(0, 0);
	catlog();

	/* 045 */
	title();
	lock_r(4096, 0);
	lock_r(0, 0);
	catlog();

	/* 046 */
	title();
	lock_r(4096, 0);
	lock_w(0, 0);
	catlog();

	/* 047 */
	title();
	lock_w(4096, 0);
	lock_r(0, 0);
	catlog();

	/* 048 */
	title();
	lock_w(4096, 0);
	lock_w(0, 0);
	catlog();
}

void test049_056()
{
	/* 049 */
	title();
	lock_r(4096, 4096);
	lock_r(0, 0);
	catlog();

	/* 050 */
	title();
	lock_r(4096, 4096);
	lock_w(0, 0);
	catlog();

	/* 051 */
	title();
	lock_w(4096, 4096);
	lock_r(0, 0);
	catlog();

	/* 052 */
	title();
	lock_w(4096, 4096);
	lock_w(0, 0);
	catlog();

	/* 053 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(4096, 4096);
	catlog();

	/* 054 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(4096, 4096);
	catlog();

	/* 055 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(4096, 4096);
	catlog();

	/* 056 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(4096, 4096);
	catlog();
}

void test057_064()
{
	/* 057 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(4096, 4096);
	catlog();

	/* 058 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(4096, 4096);
	catlog();

	/* 059 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(4096, 4096);
	catlog();

	/* 060 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(4096, 4096);
	catlog();

	/* 061 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(0, 12288);
	catlog();

	/* 062 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(0, 12288);
	catlog();

	/* 063 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(0, 12288);
	catlog();

	/* 064 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(0, 12288);
	catlog();
}

void test065_072()
{
	/* 065 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(0, 12288);
	catlog();

	/* 066 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(0, 12288);
	catlog();

	/* 067 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(0, 12288);
	catlog();

	/* 068 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(0, 12288);
	catlog();

	/* 069 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(4096, 12288);
	catlog();

	/* 070 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(4096, 12288);
	catlog();

	/* 071 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(4096, 12288);
	catlog();

	/* 072 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(4096, 12288);
	catlog();
}

void test073_080()
{
	/* 073 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(4096, 12288);
	catlog();

	/* 074 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(4096, 12288);
	catlog();

	/* 075 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(4096, 12288);
	catlog();

	/* 076 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(4096, 12288);
	catlog();

	/* 077 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 078 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 079 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 080 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 16384);
	catlog();
}

void test081_088()
{
	/* 081 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 082 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 083 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 084 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 085 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 086 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 087 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 088 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 16384);
	catlog();
}

void test089_096()
{
	/* 089 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 090 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 091 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 16384);
	catlog();

	/* 092 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 16384);
	catlog();

	/* 093 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 094 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 095 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 096 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 20480);
	catlog();
}

void test097_104()
{
	/* 097 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 098 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 099 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 100 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 101 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 102 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 103 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 104 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 20480);
	catlog();
}

void test105_112()
{
	/* 105 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 106 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 107 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 20480);
	catlog();

	/* 108 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 20480);
	catlog();

	/* 109 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 110 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 111 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 112 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 24576);
	catlog();
}

void test113_120()
{
	/* 113 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 114 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 115 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 116 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 117 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 118 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 119 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 120 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 24576);
	catlog();
}

void test121_128()
{
	/* 121 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 122 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 123 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(4096, 24576);
	catlog();

	/* 124 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(4096, 24576);
	catlog();

	/* 125 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 126 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 127 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 128 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 20480);
	catlog();
}

void test129_136()
{
	/* 129 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 130 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 131 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 132 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 133 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 134 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 135 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 136 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 20480);
	catlog();
}

void test137_144()
{
	/* 137 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 138 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 139 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(8192, 20480);
	catlog();

	/* 140 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(8192, 20480);
	catlog();

	/* 141 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 142 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 143 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 144 */
	title();
	lock_r(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(0, 32768);
	catlog();
}

void test145_152()
{
	/* 145 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 146 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 147 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 148 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_r(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 149 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 150 */
	title();
	lock_r(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 151 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 152 */
	title();
	lock_w(0, 8192);
	lock_r(12288, 8192);
	lock_w(24576, 8192);
	lock_w(0, 32768);
	catlog();
}

void test153_160()
{
	/* 153 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 154 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_r(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 155 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_r(0, 32768);
	catlog();

	/* 156 */
	title();
	lock_w(0, 8192);
	lock_w(12288, 8192);
	lock_w(24576, 8192);
	lock_w(0, 32768);
	catlog();

	/* 157 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 158 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 159 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 160 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();
}

void test161_168()
{
	/* 161 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 162 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 163 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 164 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 165 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 166 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 167 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 168 */
	title();
	lock_r(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();
}

void test169_176()
{
	/* 169 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 170 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 171 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 172 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 173 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 174 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 175 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 176 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();
}

void test177_184()
{
	/* 177 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 178 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 179 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 180 */
	title();
	lock_r(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 181 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 182 */
	title();
	lock_w(0, 4096);
	lock_r(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 183 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 184 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_r(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();
}

void test185_188()
{
	/* 185 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 186 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_r(24576, 4096);
	lock_w(0, 28672);
	catlog();

	/* 187 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_r(0, 28672);
	catlog();

	/* 188 */
	title();
	lock_w(0, 4096);
	lock_w(8192, 4096);
	lock_w(16384, 4096);
	lock_w(24576, 4096);
	lock_w(0, 28672);
	catlog();
}

void test_start()
{
	test001_008();
	test009_016();
	test017_024();
	test025_032();
	test033_040();
	test041_048();
	test049_056();
	test057_064();
	test065_072();
	test073_080();
	test081_088();
	test089_096();
	test097_104();
	test105_112();
	test113_120();
	test121_128();
	test129_136();
	test137_144();
	test145_152();
	test153_160();
	test161_168();
	test169_176();
	test177_184();
	test185_188();
}

/* main */
int main(int argc, char **argv)
{

	if (argc != 2)
		usage(argv[0]);

	lockd_pid = atoi(argv[1]);
	printf("pfs_lkmgr-pid = %d\n", lockd_pid);

	open_file();
	test_start();
	close(fd);
	return 0;
}

