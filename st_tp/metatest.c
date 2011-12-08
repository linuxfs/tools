#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <stdlib.h>

#define PERROR(x)                               \
        do {                                    \
                fprintf(stderr, "%s %s[%d]: ",  \
		 __FUNCTION__, strerror((x)), (x)); \
        } while (0)

char filename[PATH_MAX + 1];
int x = 1, f = 1;

void usage(void)
{
	fprintf(stderr, "Usage:[-d directory][-f file count][-l loop count]"
			"[-x file count of one dir]\n"
			"         -d : test directory (default : current)\n"
			"         -f : num of making file (default : 1)\n"
			"         -l : num of loop (default : unlimited)\n"
			"         -x : num of making directory (default : 1)\n"
	);
}

/*
 * rmdir(2)�¹�(�ƥ��ȥǥ��쥯�ȥ����)�ؿ�
 * 0��(�����-1)�Υǥ��쥯�ȥ���ȥǥ��쥯�ȥ��۲��˺�������
 */
int func_mkdir(const char *dirpath)
{
	int i;
	for (i = 0; i < x; i++) {
		snprintf(filename, PATH_MAX, "%s%d", dirpath, i);
		if (mkdir(filename, 0777) < 0) {
			PERROR(errno);
			return - 1;
		}
	}
	return 0;
}

/*
 * creat(2), close(2)�¹�(�ƥ��ȥե��������)�ؿ�
 * �ƥ��ȥǥ��쥯�ȥ��۲���0��(�����-1)�Υ쥮��顼�ե�����������close����
 */
int func_creat(const char *dirpath)
{
	int i, j, fd;
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j);
			if ((fd = creat(filename, 0666)) < 0) {
				PERROR(errno);
				return - 1;
			}
			close(fd);
		}
	}
	return 0;
}

/*
 * open(2)��close(2)�¹Դؿ�
 * �����������٤ƤΥ쥮��顼�ե�������Ф���open��close����
 */
int func_open(const char *dirpath)
{
	int i, j, fd;
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j);
			if ((fd = open(filename, O_RDONLY)) < 0) {
				PERROR(errno);
				return - 1;
			}
			close(fd);
		}
	}
	return 0;
}

/*
 * utime(2)�¹Դؿ�
 * �����������٤ƤΥ쥮��顼�ե�������Ф���utime��¹Ԥ���
 */
int func_utime(const char *dirpath)
{
	int i, j;

	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j);
			if (utime(filename, NULL) < 0) {
				PERROR(errno);
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * stat(2)�¹Դؿ�
 * �����������٤ƤΥ쥮��顼�ե�������Ф���stat��¹Ԥ���
 */
int func_stat(const char *dirpath)
{
	int i, j;
	struct stat buf;
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j);
			if (stat(filename, &buf) < 0) {
				PERROR(errno);
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * symlink(2)�¹�(����ܥ�å���󥯺���)�ؿ�
 * �����������٤ƤΥ쥮��顼�ե�������Ф��ƥ���ܥ�å���󥯤��������
 * ����������ϥ�󥯸���Ʊ���ǥ��쥯�ȥ���ǡ�̾���� 0��(�����-1)��
 * �쥮��顼�ե����뤬���ä��Ȥ���� (�����*2)��(�����*3-1)
 */
int func_symlink(const char *dirpath)
{
	int i, j;
	char sym_filename[PATH_MAX + 1];
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j);
			snprintf(sym_filename, PATH_MAX, "%s%d", subpath, j + (int)(f / x) * 2);
			if (symlink(filename, sym_filename) < 0) {
				PERROR(errno);
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * readlink(2)�¹�(����ܥ�å�����ɤ߼��)�ؿ�
 * �����������٤ƤΥ���ܥ�å���󥯤��Ф���readlink��¹Ԥ���
 */
int func_readlink(const char *dirpath)
{
	int i, j;
	char sym_filename[PATH_MAX + 1];
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j + (int)(f / x) * 2);
			if (readlink(filename, sym_filename, PATH_MAX) < 0) {
				PERROR(errno);
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * rename(2)�¹�(����ܥ�å����̾���ѹ�)�ؿ�
 * �����������٤ƤΥ���ܥ�å���󥯤��Ф���rename��¹Ԥ���
 * (�����*2)��(�����*3-1)�Υ���ܥ�å���󥯤����ä��Ȥ����
 * �ѹ����̾����(�����)��(�����*2-1)�Ȥʤ�
 */
int func_rename(const char *dirpath)
{
	int i, j;
	char sym_filename[PATH_MAX + 1];
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x); j++) {
			snprintf(filename, PATH_MAX, "%s%d", subpath, j + (int)(f / x));
			snprintf(sym_filename, PATH_MAX, "%s%d", subpath, j + (int)(f / x) * 2);
			if (rename(sym_filename, filename) < 0) {
				PERROR(errno);
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * unlink(2)�¹�(�ե�������)�ؿ�
 * ���������쥮��顼�ե����롢����ܥ�å���󥯤򤹤٤ƺ������
 */
int func_unlink(const char *dirpath, const int err_flag)
{
	int i, j;
	for (i = 0; i < x; i++) {
		char subpath[PATH_MAX + 1];
		snprintf(subpath, PATH_MAX, "%s%d/", dirpath, i);
		for (j = 0; j < (int)(f / x) * 2; j++) {
			snprintf(filename, PATH_MAX, "%s/%d", subpath, j);
			if (unlink(filename) < 0) {
				if (err_flag == 0) {
					PERROR(errno);
				}
				return - 1;
			}
		}
	}
	return 0;
}

/*
 * rmdir(2)�¹�(�ƥ��ȥǥ��쥯�ȥ���)�ؿ�
 * ���������ƥ��ȥǥ��쥯�ȥ�򤹤٤ƺ������
 */
int func_rmdir(const char *dirpath, const int err_flag)
{
	int i;
	for (i = 0; i < x; i++) {
		snprintf(filename, PATH_MAX, "%s%d", dirpath, i);
		if (rmdir(filename) < 0) {
			if (err_flag == 0) {
				PERROR(errno);
			}
			return - 1;
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0, loop = -1, err_count = 0, o_err_flag = 0;
	char dirpath[PATH_MAX + 1], dirpath_bak[PATH_MAX + 1], tmpdirpath[PATH_MAX + 1], opt;
	struct stat statbuf;

	strcpy(dirpath, "");

	while ((opt = getopt(argc, argv, "d:l:f:x:")) != EOF) {
		switch (opt) {
		case 'd' :
			strncpy(dirpath, optarg, PATH_MAX);
			break;
		case 'l' :
			if ((loop = atoi(optarg)) <= 0) {
				fprintf(stderr, "Invalid Argument : l = %d\n", loop);
				o_err_flag = 1;
			}
			break;
		case 'f' :
			if ((f = atoi(optarg)) <= 0) {
				fprintf(stderr, "Invalid Argument : f = %d\n", f);
				o_err_flag = 1;;
			}
			break;
		case 'x' :
			if ((x = atoi(optarg)) <= 0) {
				fprintf(stderr, "Invalid Argument : x = %d\n", x);
				o_err_flag = 1;;
			}
			break;
		case '?' :
		default:
			o_err_flag = 1;
			break;
		}
	}

	if (o_err_flag == 1) {
		usage();
		goto end;
	}

	if (x > f) {
		fprintf(stderr, "directory num is larger than file num : x=%d, f=%d\n", x, f);
		exit(1);
	} else if (f % x != 0) {
		int tmp_f = f;
		while (f % x != 0) {
			f--;
		}
		fprintf(stderr, "file num is changed by program : %d --> %d\n"
			"if you don't want to change file num,\n"
			"you must set dividable num(f/x)\n", tmp_f, f);
	}

	if (mkdir(dirpath, 0777) <0) {
		if (errno == EEXIST) {
			if (stat(dirpath, &statbuf) < 0 || !S_ISDIR(statbuf.st_mode)) {
				fprintf(stderr, "%s is not directory\n", dirpath);
				exit(errno);
			}
		} else {
			perror("failed to make working directory");
			exit(errno);
		}
	}

	if (strcmp(dirpath, "") != 0 && dirpath[strlen(dirpath) - 1] != '/') {
		strncat(dirpath, "/", PATH_MAX);
	}

	strncpy(dirpath_bak, dirpath, PATH_MAX);

	if (!strcmp(dirpath, "")) {
		strcpy(dirpath, "./dir0/");
	} else {
		strncat(dirpath, "/dir0/", PATH_MAX);
	}

	/* ��ȥǥ��쥯�ȥ�Υѥ����� */
	if (mkdir(dirpath, 0777) < 0) {
		if (errno == EEXIST) {
			if (stat(dirpath, &statbuf) < 0 || !S_ISDIR(statbuf.st_mode)) {
				fprintf(stderr, "%s is not directory\n", dirpath);
				exit(errno);
			}
		} else {
			perror("failed to make sub working directory");
			exit(errno);
		}
	}

 restart :

	/* -l �λ��꤬�ʤ����̵�¥롼�פ��� */
	while (i++ != loop) {
		if (func_mkdir(dirpath) < 0) {
			goto err;
		}
		if (func_creat(dirpath) < 0) {
			goto err;
		}
		if (func_open(dirpath) < 0) {
			goto err;
		}
		if (func_utime(dirpath) < 0) {
			goto err;
		}
		if (func_stat(dirpath) < 0) {
			goto err;
		}
		if (func_symlink(dirpath) < 0) {
			goto err;
		}
		if (func_readlink(dirpath) < 0) {
			goto err;
		}
		if (func_rename(dirpath) < 0) {
			goto err;
		}
		if (func_unlink(dirpath, 0) < 0) {
			goto err;
		}
		if (func_rmdir(dirpath, 0) < 0) {
			goto err;
		}
		/* ̵�¥롼�׻��Υ����С��ե��ɻ� */
		if (i == INT_MAX) {
			i = 0;
		}
	}
	return 0;

 err :

	err_count++;
	/*
	 * ���顼������ȥǥ��쥯�ȥ��۲���������������ν������Τ����顼��
	 * ����ͤ��ơ���ȥǥ��쥯�ȥ���ѹ�����
	 * ��ȥǥ��쥯�ȥ�̾�� ����/dir + err_count
	 */
	strncpy(dirpath, dirpath_bak, PATH_MAX);
	if (dirpath[strlen(dirpath) - 1] == '/') {
		dirpath[strlen(dirpath) - 1] = '\0';
		snprintf(tmpdirpath, PATH_MAX, "%s/dir%d/", dirpath, err_count);
	} else {
		snprintf(tmpdirpath, PATH_MAX, "%s/dir%d/", dirpath, err_count);
	}
	strcpy(dirpath, tmpdirpath);
	if (err_count == INT_MAX) {
		fprintf(stderr, "too many errors detected! exit program\n");
		exit(1);
	}
	func_unlink(dirpath, 1);
	func_rmdir(dirpath, 1);
	if (mkdir(dirpath, 0777) < 0) {
		if (errno == EEXIST) {
			if (stat(dirpath, &statbuf) < 0 || !S_ISDIR(statbuf.st_mode)) {
				fprintf(stderr, "%s is not directory\n", dirpath);
				goto err;
			}
		}
		else {
			fprintf(stderr, "failed to make sub working directory %d\n", errno);
			goto err;
		}
	}
	goto restart;

 end :
	return 1;
}

