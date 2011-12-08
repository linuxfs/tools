/*
 * １つのファイルに対して複数ノードからのI/Oを行うTP
 * 各ノードは、決められた領域にだけI/Oを出します。
 */

#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#define FILE_NAME "/mnt/pfs-mirror/y-takahashi/lock_test2"

/*
 * IO_SIZE  １回当たりのIOSIZEを指定します。デフォルトは32768(32K)
 *          これはPFSのキャッシュが32Kなのに起因します。
 * IO_POINT 各ノードがI/Oを行う領域です。デフォルトは8
 *          つまり１ノードは８箇所×32K＝256Kの領域に対してI/Oを行う
 *          512ノード一斉アクセスの場合は128Mのファイルに対してI/Oを行う
 */
#define IO_SIZE   32768
#define IO_POINT  128
/*
#define IO_POINT  8
*/
#define ERR_CODE  -1
#define TO_RETURN 1

uint64_t seek_max;
uint64_t seek_min;
uint64_t *buf;

void usage(void)
{
	fprintf(stderr, "usage: rikaken -r node(0x---)\n");
	exit(TO_RETURN);
}

/* Lock */
int lock(int fd, off_t seek_pt)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = IO_SIZE;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return ERR_CODE;
	}
	return 0;
}

int write_record(int fd, off_t seek_pt)
{
	ssize_t size;

	if (lseek(fd, seek_pt, SEEK_SET) < 0) {
		perror("lseek");
		return ERR_CODE;
	}
	size = write(fd, buf, IO_SIZE);
	if (size != IO_SIZE) {
		perror("WRITE");
		return ERR_CODE;
	}
	return 0;
}

int read_record(int fd, off_t seek_pt)
{
	uint64_t i;
	ssize_t size;

	if (lseek(fd, seek_pt, SEEK_SET) < 0) {
		perror("lseek");
		return ERR_CODE;
	}
	size = read(fd, buf, IO_SIZE);
	if (size != IO_SIZE) {
		perror("READ");
		return ERR_CODE;
	}
	for (i = 0; i < (IO_SIZE / sizeof(uint64_t)); i++) {
		if (buf[i] != i) {
			fprintf(stderr, "WARNING:DATA is wrong !!\n");
			fprintf(stderr, "\tThis data is %"PRIu64" !!  But true data is %"PRIu64" !!\n", buf[i], i);
			return ERR_CODE;
		}
	}
	return 0;
}

/* UnLock */
int unlock(int fd, off_t seek_pt)
{
	/* set Lock Table F_UNLCK */
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = IO_SIZE;
	lock.l_pid = getpid();

	/* UnLock */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
		return ERR_CODE;
	}
	return 0;
}

int write_read(off_t seek_pt, int fd)
{
/*	if (lock(fd, seek_pt) < 0) {
 *		return ERR_CODE;
 *	}
 */
	if (write_record(fd, seek_pt) < 0) {
		unlock(fd, seek_pt);
		return ERR_CODE;
	}

	if (read_record(fd, seek_pt) < 0) {
		unlock(fd, seek_pt);
		return ERR_CODE;
	}

/*	if (unlock(fd, seek_pt) < 0) {
 *		return ERR_CODE;
 *	}
 */
	return 0;
}

/* main */
int main(int argc, char **argv)
{
	off_t seek_pt;
	int fd;
	int c;
	uint64_t i;
	uint64_t node = 0;
	long int random_tmp;

	/* initialize */
	seek_max = 0x7FFFFFFFFFFULL - IO_SIZE;
	seek_min = 0;

	/* option proc */
	if (argc < 3) {
		usage();
	}
	while ((c = getopt(argc, argv, "r:")) != EOF) {
		switch (c) {
		case 'r':
			node = (uint64_t)strtoull(optarg, &optarg, 16);
			seek_max = (node + 1) * IO_SIZE * IO_POINT - IO_SIZE;
			seek_min = node * IO_SIZE * IO_POINT;
			break;
		default:
			usage();
		}
	}

	srand((unsigned int)node + time(NULL));

	/* malloc buffer */
	buf = (uint64_t *)malloc(IO_SIZE);
	if (buf == NULL) {
		perror("malloc()");
		return TO_RETURN;
	}

	/* creat data set */
	for (i = 0; i < (IO_SIZE / sizeof(uint64_t)); i++) {
		buf[i] = i;
	}

	printf("node %"PRIu64" , start : %"PRIu64"  , end : %"PRIu64"\n", node, seek_min, seek_max);

	/* open file */
	fd = open(FILE_NAME, O_RDWR|O_CREAT, 0666);
	if (fd < 0) {
		perror("OPEN");
		free(buf);
		return TO_RETURN;
	}

	for (;;) {
		random_tmp = random() % IO_POINT;
		seek_pt = random_tmp * IO_SIZE + seek_min;
		if (seek_pt > seek_max) {
			continue;
		}
		if (write_read(seek_pt, fd) < 0) {
			if (close(fd) < 0) {
				perror("close");
			}
			free(buf);
			return TO_RETURN;
		}
	}
	free(buf);
	if (close(fd) < 0) {
		perror("close");
		return TO_RETURN;
	}
	return 0;
}
