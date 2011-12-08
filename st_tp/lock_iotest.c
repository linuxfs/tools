#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define OFF_T_MAX 0x7fffffffffffffffULL
#define FILE_NAME "file"
#define IO_SIZE   65536
#define IO_LEN    1

/* open file */
int open_file()
{
	int fd;
	/* write read open */
	fd = open(FILE_NAME, O_RDWR|O_CREAT, 0666);
	if (fd < 0) {          /* file open check */
		perror("OPEN");
		return - 1;
	}
	return fd;
}

/* Lock */
int lock(int fd, off_t seek_pt)
{
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = seek_pt;
	lock.l_len = IO_LEN;
	lock.l_pid = getpid();

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): Lock ");
		return - 1;
	}
	return 0;
}

int write_record(int fd, off_t seek_pt)
{
	unsigned int i;
	int size;
	char	 *buf;
	buf = (char *)malloc(sizeof(char) * IO_LEN);
	if (buf == NULL) {
		perror("malloc()");
		return - 1;
	}
	for (i = 0; i < IO_LEN; i++) {
		buf[i] = '#';
	}
	if (lseek(fd, seek_pt, SEEK_SET) < 0) {
		perror("lseek");
		free(buf);
		return - 1;
	}
	size = write(fd, buf, (sizeof(char) * IO_LEN));
	if (size != IO_LEN) {
		perror("write");
		free(buf);
		return - 1;
	}
	free(buf);
	return 0;
}

int read_record(int fd, off_t seek_pt)
{
	unsigned int i;
	int size;
	char	 *buf;
	buf = (char *)malloc(sizeof(char) * IO_LEN);
	if (buf == NULL) {
		perror("malloc()");
		return - 1;
	}
	if (lseek(fd, seek_pt, SEEK_SET) < 0) {
		perror("lseek");
		free(buf);
		return - 1;
	}
	size = read(fd, buf, (sizeof(char) * IO_LEN));
	if (size != IO_LEN) {
		perror("write");
		free(buf);
		return - 1;
	}
	for (i = 0; i < IO_LEN; i++) {
		if (buf[i] != '#') {
			fprintf(stderr, "WARNING:DATA is wrong !!\n");
			fprintf(stderr, "\tThis data is %c !!  But true data is %c !!\n", buf[i], '#');
			free(buf);
			return - 1;
		}
	}
	free(buf);
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
	lock.l_len = IO_LEN;
	lock.l_pid = getpid();

	/* UnLock */
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		perror("fcntl(): UnLock ");
		return - 1;
	}
	return 0;
}

int write_read(off_t seek_pt, int fd)
{
	if (lock(fd, seek_pt) < 0) {
		return - 1;
	}

	if (write_record(fd, seek_pt) < 0) {
		return - 1;
	}

	if (read_record(fd, seek_pt) < 0) {
		return - 1;
	}

	if (unlock(fd, seek_pt) < 0) {
		return - 1;
	}

	return 0;
}

/* main */
int main(int argc, char **argv)
{
	off_t seek_pt;
	int fd;

	fd = open_file();
	if (fd < 0) {
		return - 1;
	}
	for (seek_pt = 0; seek_pt < IO_SIZE; seek_pt += IO_LEN) {
		if (write_read(seek_pt, fd) < 0) {
			if (close(fd) < 0) {
				perror("close");
			}
			return - 1;
		}
	}
	if (close(fd) < 0) {
		perror("close");
		return - 1;
	}
	return 0;
}


