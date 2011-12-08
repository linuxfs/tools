#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>


int write_test(int, int, char *);
int read_test(int, int, char *);

/* ����¬���� */
int e_time(struct timeval *first, struct timeval *second)
{
	if (first->tv_usec > second->tv_usec) {
		second->tv_usec += 1000000;
		second->tv_sec--;
	}
	return((int)(second->tv_sec - first->tv_sec) * 1000000
		+ (int)(second->tv_usec - first->tv_usec));
}

/* �ᥤ�����   ���ץ������ɤ߹���Ǥ��줾��Υ⥸�塼���ž��
		�����ϥ��ץ����getopt����Ѥ��ƽ���
		�⥸�塼��ؤ�ž�����Ϥϲ�������������ե�����̾
		rcount | wcount,size,filename		*/
extern char *optarg;
int main(int argc, char **argv)
{
/* ��������ꡡ c getopt���ѿ� size ������(1048576byte)
		rcount �ɤ߹��߲��(0��) wcount �񤭹��߲��(0��)
		sizesw size option �����ꤵ�줿���ɤ����Υե饰
		filename �ե������̾����"/tmp/io-bench.tmp")
		st statbuf stat���ΰ�  */
	int c;
	int size = 1048576;
	int rcount = 0;
	int wcount = 0;
	int sizesw = 0;
	char filename[PATH_MAX];
	int st;
	struct stat statbuf;

/* �ե����륵����������Ϣ���ѿ������ */
	int mem[4];       /* �����ᥬ���������Ѥδ����¸�� */
	char *ans;        /* ʸ�������ѥݥ�����¸�� */
	char mainbuff[PATH_MAX]="kKmMgG", findbuff[PATH_MAX]="", sizebuff[PATH_MAX]="";
	int x;            /* �ݥ��󥿤ξ�국���� */
	strcpy(filename, "/tmp/io-bench.tmp");
	mem[1] = 1024;
	mem[2] = 1048576;
	mem[3] = 1073741824;

/* ���ץ��������ꡡ	r: �ɤ߹��ߥ��ץ����rcount
			w: �񤭹��ߥ��ץ����wcount
			s: ���������ץ����size
			���ץ����̵���ĥե�����̾��filename	*/
	while   ((c = getopt(argc, argv, "r:w:s:")) > 0)  {
		switch  (c) {
		case  'r':
			rcount = atoi(optarg);
			break;

		case  'w':
			wcount = atoi(optarg);
			break;

		case  's':
			memmove(sizebuff, optarg, strlen(optarg) - 1);
			memmove(findbuff, optarg + strlen(optarg) - 1, 1);
			ans = strstr(mainbuff, findbuff);
			if (ans != NULL) {
				x = (int)(ans - mainbuff) + 2;
				x = x / 2;
				size = atoi(sizebuff) * mem[x];
			}
			else {
				size = atoi(optarg);
			}
			sizesw = 1;
			break;

		default:
			printf("io-bench [-r count][-w coount][-s size]");
			printf("[dirname | dirname/filename]");
			printf("\n\n	-r count   [count] times read.	default 0.");
			printf("\n\n	-w count   [count] times write.	default 0.");
			printf("\n\n	-s size   [size] file open.	");
			printf("default 1M or filesize\n\n");
			printf("	default filename '/tmp/io-bench.tmp'");
			printf("\n\n	for example './io-bench -r 10 -w 10 -s");
			printf(" 1024K /tmp/io-bench.tmp'\n\n");
			exit(1);
		}
	}

/* �ե�����̾�λ��꤬���ä����filename�˥ե�����̾������ */
	if (optind < argc) {
		strcpy(filename, argv[optind++]);
	}

/* ���ꤵ�줿�ե����뤬�ե����뤫���ǥ��쥯�ȥ꤫��Ƚ�� */
	errno = 0;
	st = stat(filename, &statbuf);
	if (st == -1) {
		if (errno != ENOENT) {
			perror(filename);
			exit (1);
		}
	}
	else {	/* filemode���ǥ��쥯�ȥ�ξ�硡�ǥ��쥯�ȥ��/io-bench.tmp */
		if (S_ISDIR(statbuf.st_mode)) {
			strcat(filename, "/io-bench.tmp");
		}
	}

/* filename �Υե����륵�����򥵥�����������size���̵꤬������ */
	st = stat(filename, &statbuf);
	if (st == -1) {
	}
	else {
		if (sizesw == 0) {
			size = statbuf.st_size;
		}
	}


/* �񤭹��߽������ɤ߹��߽���  ��������ξ��¹Ԥ��ʤ�	*/
	if (wcount > 0) {
		write_test(wcount, size, filename);
	}
	if (rcount > 0) {
		read_test(rcount, size, filename);
	}
	return 0;
}

/* �ɤ߹��߽������������ɤ߹��߲�������������ե�����̾
			rcount,size,filename
		���ϡ��ɤ߹��߻��֡��Ǿ��͡������͡�ʿ���͡�Mbyte/s
		����͡�max=0.00 �����͡�min=9999999.99 �Ǿ��͡�kei=0.00 ���
			mbps Mb/s�׻��ѡ�idx���׻��Ѱ���ݴɡ�read_kei ���ɹ���� */
int read_test(int rcount, int size, char *filename)
{
	int fd;
	double max = 0.00;
	double min = 9999999.99;
	double kei = 0.00;
	double mbps;
	double idx;
	double read_kei = 0.00;
	unsigned int *buf;
	int i, j, default_etime;
	struct timeval start, stop;
	buf = (unsigned int *)malloc(size);
		if (buf == NULL) {
		perror("malloc");
		return -1;
	}

/* gettime�ˤ�����������֤λ��С�	*/
	gettimeofday (&start, NULL);
	gettimeofday (&stop, NULL);
	default_etime = e_time(&start, &stop);

/* rcountʬ�ɹ�loop */
	for (i = 1; i <= rcount;) {
		fd = open(filename, O_RDONLY, 0);
		if (fd < 0) {		/* open check */
			perror(filename);
			free(buf);
			return -1;
		}
		/* ������Ȥ��줿����ɤ߹��फ��EOF�ˤʤ�ޤǡ�EOF�ξ��bleak;��ȴ���� */
		while (i <= rcount) {
			int read_size;
			gettimeofday (&start, NULL);
			read_size = read(fd, buf, size);
			gettimeofday (&stop, NULL);

			if (read_size > 0 || size == 0) {
				read_kei += read_size;
				idx = (e_time(&start, &stop) - default_etime);
				printf("read-test %d   %.2fms. readsize=%d\n", i, idx / 1000, read_size);
				/* �ǹ��͡������͡���פη׻� */
				if (max < idx) {
					max = idx;
				}
				if (min > idx) {
					min = idx;
				}
				kei += idx;
				i++;
			}
			else {
				break;
			}
			for (j = 0; j < (size / sizeof(unsigned int)); j++) {
				if (buf[j] != j) {
					fprintf(stderr, "WARNING:DATA is wrong !!\n");
					fprintf(stderr, "\tThis data is %d !!  But true data is %d !!\n", buf[j], j);
					exit(1);
				}
			}
		}
		close(fd);
	}
	if (kei == 0.00) {
		mbps = 0.00;
	}
	else {
		mbps = ((1000000 / kei) * (read_kei / (1024 * 1024)));
	}
/* �ǽ���ɽ�� */
	printf("read-test Min %.2fms Max %.2fms Average %.2fms %.2fMbyte/s\n",
		min / 1000, max / 1000, (kei / rcount) / 1000, mbps);
	printf("io-bench: %d count read-test at %d byte Done.\n\n", rcount, size);
	free(buf);
	return 0;
}

/* �񤭹��߽��� �������Ľ񤭹��߲�������������ե�����̾
			wcount,size,filename
		���ϡĽ񤭹��߻��֡��Ǿ��͡������͡�ʿ���͡�Mbyte/s
		����͡�max=0.00 �����͡�min=9999999.99 �Ǿ��͡�kei=0.00 ��ס�
		average ʿ�ѷ׻��� mbps Mb/s�׻��� idx���׻��Ѱ���ݴ�
		i �롼�����ѿ���flg �񤭹���check�ѡ�*/
int write_test(int wcount, int size, char *filename)
{
	int fd;
	double max = 0.00;
	double min = 9999999.99;
	double kei = 0.00;
	double average, mbps;
	double idx;
	unsigned int *buf;
	int i;
	int flg;
	int default_etime;
	struct timeval start, stop;
	buf = (unsigned int *)malloc(size);
	if (buf == NULL) {
		perror("malloc");
		return -1;
	}
	for (i = 0; i < (size / sizeof(unsigned int)); i++) {
		buf[i] = i;
	}
	fd = open(filename, O_RDWR|O_CREAT|O_APPEND, 0666);
	/* fd = creat(filename,0666); */
	if (fd < 0) {		/* �ե�����open check */
		perror(filename);
		free(buf);
		return -1;
	}

/* gettime�ˤ�����������֤λ��С�	*/
	gettimeofday (&start, NULL);
	gettimeofday (&stop,  NULL);
	default_etime = e_time(&start, &stop);

/* wcountʬ�����񤭹��� */
	for (i = 1; i <= wcount; i++) {
		gettimeofday (&start, NULL);
		flg = write(fd, buf, size);
		gettimeofday (&stop, NULL);

		if (flg == -1) {
			perror(filename);
			wcount = i - 1;
		}
		else {
			idx = (e_time(&start, &stop) - default_etime);
			printf("write-test %d   %.2fms. size=%d\n", i, idx / 1000, flg);
			/* �ǹ��͡������͡���פη׻� */
			if (max < idx) {
				max = idx;
			}
			if (min > idx) {
				min = idx;
			}
			kei += idx;
		}
	}
	close(fd);
	if (kei == 0.00 || wcount == 0) {
		mbps = 0.00;
	}
	else {
		mbps = ((1000000 / (kei / wcount)) * size / (1024 * 1024));
	}
	if (wcount == 0.00) {
		average = 0.00;
	}
	else {
		average = (kei / wcount) / 1000;
	}
/* �ǽ���ɽ�� */
	printf("write-test Min %.2fms Max %.2fms Average %.2fms %.2fMbyte/s\n",
		min / 1000, max / 1000, average, mbps);
	printf("io-bench: %d count write-test at %d byte Done.\n\n", wcount, size);
	free(buf);
	return 0;
}

