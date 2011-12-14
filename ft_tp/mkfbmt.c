/*
 *	mkfbmt.c ファイル操作時間測定
 *
 *	2005.09.14  updated by y-takahashi
 *      2007.01.05  updated by y-takahashi
 *                  creat -> open(O_CREAT | O_EXCL | O_WRONLY)
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <utime.h>

#define KEEP_FILE 20
#define CHECK_ENTRY 6

#define MK_ALL		  0
#define MK_MKDIR	  1
#define MK_CREATE	  2
#define MK_OPEN		  4
#define MK_UTIME	 16
#define MK_STAT		 32
#define MK_UNLINK	 64
#define MK_RMDIR	128

/* grobal config */
typedef struct mkfbmt_conf {
	int files;
	uint64_t max_files;
	int directry;
	int time_out;
	int total;
	int mk_files;
	int entry;
	int loops;
	int modis;
} mkfbmt_conf_t;

double table[KEEP_FILE][CHECK_ENTRY];
struct timeval g_rtime;
struct timeval g_etime;

struct mkfbmt_conf conf = {
	.files = 10000,
	.max_files = 0xffffffffffffffffULL,
	.directry = 0,
	.time_out = 0,
	.total = 0,
	.mk_files = 0,
	.entry = 0,
	.loops = 1,
	.modis = 1000,
};

void handler(int sig);

/* usage */
void Bform()
{
	fprintf(stderr, "Usage:[-d directry][-f targetfile count][-t signal][-x file count][-l count][-mcosur][-w size]\n"
			"       -d test directry\n"
			"       -f targetfile count\n"
			"       -t time_out print time\n"
			"       -x max file for directry\n"
			"       -l test loop count\n"
			"       -m mkdir  test\n"
			"       -c creat  test\n"
			"       -o open   test\n"
			"       -p utime  test\n"
			"       -s stat   test\n"
			"       -u unlink test\n"
			"       -r rmdir  test\n"
			"	-w write  test\n");
	exit(1);
}

void __linuxfs_init(void)
{
	fprintf(stderr, "## %s(%d) [%p] \n", __FUNCTION__, __LINE__, __builtin_return_address(0));
}

/* error */
void Berror(int errs)
{
	fprintf(stderr, "ERROR : %s \n", strerror(errs));
	exit(1);
}

/* e_time */
double e_time(struct timeval first, struct timeval second)
{
	if (first.tv_usec > second.tv_usec) {
		second.tv_usec += 1000000;
		second.tv_sec--;
	}
	return((double)(second.tv_sec - first.tv_sec) * 1000000.0
		+ (double)(second.tv_usec - first.tv_usec));
}

/* for timeout setting */
void set_sigalrm(void)
{
	struct sigaction act;
	sigset_t full_mask;
	conf.mk_files = 0;


	if (sigfillset(&full_mask)) {
		fprintf(stderr, "Cannot fill signal mask\n");
		exit(1);
	}

	if (sigdelset(&full_mask, SIGALRM)) {
		fprintf(stderr, "Cannot reset signal mask\n");
		exit(1);
	}

	if (sigprocmask(SIG_BLOCK, &full_mask, NULL)) {
		fprintf(stderr, "Cannot set signal proc mask\n");
		exit(1);
	}

	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	act.sa_mask = full_mask;

	if (sigaction(SIGALRM,  &act, NULL)) {
		fprintf(stderr, "Cannot set signal action\n");
		exit(1);
	}
	alarm(conf.time_out);
}

/* unset timeout */
void unset_sigalrm(void)
{
	alarm(0);
}

/* signal handler */
void handler(int sig)
{
	if (sig == SIGALRM) {
		unset_sigalrm();
		conf.total += conf.mk_files;
		printf("make %8d(+%8d) / %8d  \n", conf.total, conf.mk_files, conf.files);
		conf.mk_files = 0;
		set_sigalrm();
	}
}

/* make dir */
void ll_mkdir(void)
{
	int i;

	char file_name[PATH_MAX + 1];
	for (i = 0; i < conf.directry; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		if (mkdir(file_name, 0777)) {
			Berror(errno);
		}
		conf.entry++;
		conf.mk_files++;
	}
}

/* rmdir */
void ll_rmdir(void)
{
	int i;

	char file_name[PATH_MAX + 1];
	for (i = 0; i < conf.directry; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		if (rmdir(file_name)) {
			Berror(errno);
		}
		conf.entry++;
		conf.mk_files++;
	}
}

/* create */
void ll_creat(int files)
{
	int fd, i, x;
	char file_name[PATH_MAX + 1];

	for (i = 0; i < files; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		fd = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0666);
		if (fd < 0) {
			//Berror(errno);
			return;
		}
		close(fd);
		conf.entry++;
		conf.mk_files++;
		if (!(conf.entry % conf.modis)) {
			gettimeofday (&g_etime, NULL);
			x = ((conf.entry / conf.modis) > 0) ? ((conf.entry / conf.modis) - 1) : 0;
			table[x][1] = (double)(e_time(g_rtime, g_etime)) / 1000000.0;
		}
	}
}

/* open */
void ll_open(int files)
{
	int i, fd, x;

	char file_name[PATH_MAX + 1];

	for (i = 0; i < files; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		fd = open(file_name, O_RDONLY);
		if (fd < 0) {
			//Berror(errno);
			return;
		}
		close(fd);
		conf.entry++;
		conf.mk_files++;
		if (!(conf.entry % conf.modis)) {
			gettimeofday (&g_etime, NULL);
			x = ((conf.entry / conf.modis) > 0) ? ((conf.entry / conf.modis) - 1) : 0;
			table[x][2] = (double)(e_time(g_rtime, g_etime)) / 1000000.0;
		}
	}
}

void ll_utime(int files)
{
	int i, x;
	struct utimbuf ut;
	char file_name[PATH_MAX + 1];
	ut.actime = 0;
	ut.modtime = 0;

	for (i = 0; i < files; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		if (utime(file_name, &ut) < 0) {
			//Berror(errno);
			return;
		}

		conf.entry++;
		conf.mk_files++;
		if (!(conf.entry % conf.modis)) {
			gettimeofday (&g_etime, NULL);
			x = ((conf.entry / conf.modis) > 0) ? ((conf.entry / conf.modis) - 1) : 0;
			table[x][3] = (double)(e_time(g_rtime, g_etime)) / 1000000.0;
		}
	}
}

/* stat */
void ll_stat(int files)
{
	int i, x;

	char file_name[PATH_MAX + 1];

	struct stat sb;

	for (i = 0; i < files; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		if (stat(file_name, &sb)) {
			//Berror(errno);
			return;
		}
		conf.entry++;
		conf.mk_files++;
		if (!(conf.entry % conf.modis)) {
			gettimeofday (&g_etime, NULL);
			x = ((conf.entry / conf.modis) > 0) ? ((conf.entry / conf.modis) - 1) : 0;
			table[x][4] = (double)(e_time(g_rtime, g_etime)) / 1000000.0;
		}
	}
}

/* unlink */
void ll_unlink(int files)
{
	int i, x;

	char file_name[PATH_MAX + 1];

	for (i = 0; i < files; i++) {
		snprintf(file_name, PATH_MAX, "%d", i);
		unlink(file_name);
		conf.entry++;
		conf.mk_files++;
		if (!(conf.entry % conf.modis)) {
			gettimeofday (&g_etime, NULL);
			x = ((conf.entry / conf.modis) > 0) ? ((conf.entry / conf.modis) - 1) : 0;
			table[x][5] = (double)(e_time(g_rtime, g_etime)) / 1000000.0;
		}

	}
}

void creat_op(const char *directry)
{
	int i, files, sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	if (conf.directry == 0) {
		snprintf(path, PATH_MAX, "%s", directry);
		chdir(path);
		ll_creat(sv_files);
	} else {
		for (i = 0; i < conf.directry; i++) {
			sv_files > conf.max_files ? (files = conf.max_files) : (files = sv_files);
			sv_files -= conf.max_files;
			snprintf(path, PATH_MAX, "%s/%d", directry, i);
			chdir(path);
			ll_creat(files);
		}
	}
	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("creat : %lf sec , 1 file creat  for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void open_op(const char *directry)
{
	int i, files, sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	if (conf.directry == 0) {
		snprintf(path, PATH_MAX, "%s", directry);
		chdir(path);
		ll_open(sv_files);
	} else {
		for (i = 0; i < conf.directry; i++) {
			sv_files > conf.max_files ? (files = conf.max_files) : (files = sv_files);
			sv_files -= conf.max_files;
			snprintf(path, PATH_MAX, "%s/%d", directry, i);
			chdir(path);
			ll_open(files);
		}
	}
	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("open  : %lf sec , 1 file open   for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void stat_op(const char *directry)
{
	int i, files, sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	if (conf.directry == 0) {
		snprintf(path, PATH_MAX, "%s", directry);
		chdir(path);
		ll_stat(sv_files);
	} else {
		for (i = 0; i < conf.directry; i++) {
			sv_files > conf.max_files ? (files = conf.max_files) : (files = sv_files);
			sv_files -= conf.max_files;
			snprintf(path, PATH_MAX, "%s/%d", directry, i);
			chdir(path);
			ll_stat(files);
		}
	}
	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("stat  : %lf sec , 1 file stat   for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void utime_op(const char *directry)
{
	int i, files, sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	if (conf.directry == 0) {
		snprintf(path, PATH_MAX, "%s", directry);
		chdir(path);
		ll_utime(sv_files);
	} else {
		for (i = 0; i < conf.directry; i++) {
			sv_files > conf.max_files ? (files = conf.max_files) : (files = sv_files);
			sv_files -= conf.max_files;
			snprintf(path, PATH_MAX, "%s/%d", directry, i);
			chdir(path);
			ll_utime(files);
		}
	}
	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("utime : %lf sec , 1 file utime   for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void unlink_op(const char *directry)
{
	int i, files, sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	if (conf.directry == 0) {
		snprintf(path, PATH_MAX, "%s", directry);
		chdir(path);
		ll_unlink(sv_files);
	} else {
		for (i = 0; i < conf.directry; i++) {
			sv_files > conf.max_files ? (files = conf.max_files) : (files = sv_files);
			sv_files -= conf.max_files;
			snprintf(path, PATH_MAX, "%s/%d", directry, i);
			chdir(path);
			ll_unlink(files);
		}
	}
	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("unlink: %lf sec , 1 file unlink for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void mkdir_op(const char *directry)
{
	int sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	snprintf(path, PATH_MAX, "%s", directry);
	chdir(path);
	ll_mkdir();

	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("mkdir : %lf sec , 1 file mkdir  for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void rmdir_op(const char *directry)
{
	int sv_files;
	char path[PATH_MAX + 1];
	double sec;
	struct timeval start, stop;

	conf.entry = 0;
	conf.total = 0;
	sv_files = conf.files;
	set_sigalrm();
	gettimeofday (&start, NULL);
	gettimeofday (&g_rtime, NULL);

	snprintf(path, PATH_MAX, "%s", directry);
	chdir(path);
	ll_rmdir();

	gettimeofday (&stop, NULL);
	unset_sigalrm();

	sec = (double)(e_time(start, stop)) / 1000000.0 / conf.files;
	printf("rmdir : %lf sec , 1 file rmdir  for %lf sec\n", (double)(e_time(start, stop)) / 1000000.0, sec);
}

void print(void)
{
	int i, j;
	printf("file        creat       open        utime       stat        unlink\n");
	for (i = 0; i < KEEP_FILE; i++) {
		printf("%10.0lf ", table[i][0]);
		for (j = 1; j < CHECK_ENTRY; j++) {
			printf("%10.2lf ", table[i][j]);
		}
		printf("\n");
	}
}

/* main */
int main(int argc, char *argv[])
{
	int c, i, j;
	char directry[PATH_MAX + 1];
	char *realpath_ptr;
	unsigned int submit = MK_ALL;

	__linuxfs_init();
	strncpy(directry, "./\0", PATH_MAX);

	for (i = 0; i < KEEP_FILE; i++) {
		for (j = 0; j < CHECK_ENTRY; j++) {
			table[i][j] = 0;
		}
		table[i][0] = (double) i * conf.modis;
	}

	while ((c = getopt(argc, argv, "d:f:x:t:l:w:mcosurp")) != EOF) {
		switch (c) {
		case 'd':
			strncpy(directry, optarg, PATH_MAX);
			break;
		case 'f':
			conf.files = atoi(optarg);
			if (conf.files < 0) {
				Berror(EINVAL);
			}
			break;
		case 'x':
			conf.max_files = atoi(optarg);
			if (conf.max_files < 0) {
				Berror(EINVAL);
			}
			break;
		case 't':
			conf.time_out = atoi(optarg);
			if (conf.time_out < 0) {
				Berror(EINVAL);
			}
			break;
		case 'l':
			conf.loops = atoi(optarg);
			if (conf.loops < 0) {
				Berror(EINVAL);
			}
			break;
		case 'm':
			submit |= MK_MKDIR;
			break;
		case 'c':
			submit |= MK_CREATE;
			break;
		case 'o':
			submit |= MK_OPEN;
			break;
		case 'p':
			submit |= MK_UTIME;
			break;
		case 's':
			submit |= MK_STAT;
			break;
		case 'u':
			submit |= MK_UNLINK;
			break;
		case 'r':
			submit |= MK_RMDIR;
			break;
		default:
			Bform();
		}
	}

	if (conf.max_files > 0) {
		conf.directry = conf.files / conf.max_files;
		conf.modis = conf.files / KEEP_FILE;
	} else {
		Berror(EINVAL);
	}

	for (i = 0; i < KEEP_FILE; i++) {
		for (j = 0; j < CHECK_ENTRY; j++) {
			table[i][j] = 0;
		}
		table[i][0] = (double) (i + 1) * conf.modis;
	}

	realpath_ptr = realpath(directry, NULL);
	if (realpath_ptr == NULL) {
		Berror(errno);
	}

	printf("mkfbmt %d directry %d files , one directry %"PRIu64" files\n",
		conf.directry, conf.files, conf.max_files);
	printf("make files for %s\n\n", realpath_ptr);

	for (i = 0; i < conf.loops; i++) {
		if (submit == MK_ALL || submit & MK_MKDIR) {
			mkdir_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_CREATE) {
			creat_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_OPEN) {
			open_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_UTIME) {
			utime_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_STAT) {
			stat_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_UNLINK) {
			unlink_op(realpath_ptr);
		}
		if (submit == MK_ALL || submit & MK_RMDIR) {
			rmdir_op(realpath_ptr);
		}
		print();
	}
	return 0;
}
