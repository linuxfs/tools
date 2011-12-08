#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define OFF_T_MAX 0x7FFFFFFFFFFFFFFFLL
#define FILE_NAME "file"
#define IO_SIZE   1000000ULL
#define IO_LEN    1


double e_time(struct timeval *first, struct timeval *second)
{
	if (first->tv_usec > second->tv_usec) {
		second->tv_usec += 1000000;
		second->tv_sec--;
	}
	return((double)(second->tv_sec - first->tv_sec) * 1000000
		+ (double)(second->tv_usec - first->tv_usec));
}

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
int lock(int fd, loff_t seek_pt, struct flock *lock)
{
	lock->l_start = seek_pt;

	/* Lock Start */
	if (fcntl(fd, F_SETLKW, lock) < 0) {
		perror("fcntl(): Lock ");
		return - 1;
	}
	return 0;
}

/* UnLock */
int unlock(int fd, off_t seek_pt, struct flock *lock)
{
	/* set Lock Table F_UNLCK */
	lock->l_start = seek_pt;

	/* UnLock */
	if (fcntl(fd, F_SETLKW, lock) < 0) {
		perror("fcntl(): UnLock ");
		return - 1;
	}
	return 0;
}


/* main */
int main(int argc, char **argv)
{
	loff_t seek_pt;
	int fd;
	struct flock lock_w, lock_r, lock_u;
	struct timeval start, stop;
	double default_etime, print_etime;

	/* set Lock Table F_SETLKW(read) */
	lock_r.l_type = F_RDLCK;
	lock_r.l_whence = SEEK_SET;
//      lock_r.l_start = seek_pt;
	lock_r.l_len = IO_LEN;
	lock_r.l_pid = getpid();

	/* set Lock Table F_SETLKW(write) */
	lock_w.l_type = F_WRLCK;
	lock_w.l_whence = SEEK_SET;
//      lock_w.l_start = seek_pt;
	lock_w.l_len = IO_LEN;
	lock_w.l_pid = getpid();

	/* set Lock Table F_UNLCK */
	lock_u.l_type = F_UNLCK;
	lock_u.l_whence = SEEK_SET;
//      lock_u.l_start = seek_pt;
	lock_u.l_len = IO_LEN;
	lock_u.l_pid = getpid();

	gettimeofday (&start, NULL);
	gettimeofday (&stop, NULL);
	default_etime = e_time(&start, &stop);

	fd = open_file();
	if (fd < 0) {
		return - 1;
	}

	gettimeofday (&start, NULL);
	for (seek_pt = 0; seek_pt < IO_SIZE; seek_pt += IO_LEN) {
		if (lock(fd, seek_pt, &lock_r) < 0) {
			return - 1;
		}

		if (unlock(fd, seek_pt, &lock_u) < 0) {
			return - 1;
		}
	}
	gettimeofday (&stop, NULL);
	print_etime = (e_time(&start, &stop) - default_etime) / 1000000;
	printf("lock_test(r) : %10.2lf sec \n", print_etime);


	gettimeofday (&start, NULL);
	for (seek_pt = 0; seek_pt < IO_SIZE; seek_pt += IO_LEN) {
		if (lock(fd, seek_pt, &lock_w) < 0) {
			return - 1;
		}

		if (unlock(fd, seek_pt, &lock_u) < 0) {
			return - 1;
		}
	}
	gettimeofday (&stop, NULL);
	print_etime = (e_time(&start, &stop) - default_etime) / 1000000;
	printf("lock_test(w) : %10.2lf sec \n", print_etime);

	if (close(fd) < 0) {
		perror("close");
		return - 1;
	}

	return 0;
}


