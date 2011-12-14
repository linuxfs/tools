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

/* 時間測定用 */
int e_time(struct timeval *first, struct timeval *second)
{
	if (first->tv_usec > second->tv_usec) {
		second->tv_usec += 1000000;
		second->tv_sec--;
	}
	return((int)(second->tv_sec - first->tv_sec) * 1000000
		+ (int)(second->tv_usec - first->tv_usec));
}

/* メイン処理   オプションを読み込んでそれぞれのモジュールへ転送
		引数はオプション、getoptを使用して処理
		モジュールへの転送出力は回数、サイズ、ファイル名
		rcount | wcount,size,filename		*/
extern char *optarg;
int main(int argc, char **argv)
{
/* 初期値設定　 c getopt用変数 size サイズ(1048576byte)
		rcount 読み込み回数(0回) wcount 書き込み回数(0回)
		sizesw size option が設定されたかどうかのフラグ
		filename ファイルの名前（"/tmp/io-bench.tmp")
		st statbuf stat用領域  */
	int c;
	int size = 1048576;
	int rcount = 0;
	int wcount = 0;
	int sizesw = 0;
	char filename[PATH_MAX];
	int st;
	struct stat statbuf;

/* ファイルサイズ処理関連の変数初期化 */
	int mem[4];       /* キロ、メガ、ギガ　用の基数保存用 */
	char *ans;        /* 文字検索用ポインタ保存域 */
	char mainbuff[PATH_MAX]="kKmMgG", findbuff[PATH_MAX]="", sizebuff[PATH_MAX]="";
	int x;            /* ポインタの場所記憶域 */
	strcpy(filename, "/tmp/io-bench.tmp");
	mem[1] = 1024;
	mem[2] = 1048576;
	mem[3] = 1073741824;

/* オプションの設定　	r: 読み込みオプション　rcount
			w: 書き込みオプション　wcount
			s: サイズオプション　size
			オプション無し…ファイル名…filename	*/
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

/* ファイル名の指定があった場合filenameにファイル名を代入 */
	if (optind < argc) {
		strcpy(filename, argv[optind++]);
	}

/* 指定されたファイルがファイルか、ディレクトリかを判別 */
	errno = 0;
	st = stat(filename, &statbuf);
	if (st == -1) {
		if (errno != ENOENT) {
			perror(filename);
			exit (1);
		}
	}
	else {	/* filemodeがディレクトリの場合　ディレクトリ＋/io-bench.tmp */
		if (S_ISDIR(statbuf.st_mode)) {
			strcat(filename, "/io-bench.tmp");
		}
	}

/* filename のファイルサイズをサイズに代入（size指定が無い場合） */
	st = stat(filename, &statbuf);
	if (st == -1) {
	}
	else {
		if (sizesw == 0) {
			size = statbuf.st_size;
		}
	}


/* 書き込み処理、読み込み処理  回数が０の場合実行しない	*/
	if (wcount > 0) {
		write_test(wcount, size, filename);
	}
	if (rcount > 0) {
		read_test(rcount, size, filename);
	}
	return 0;
}

/* 読み込み処理　引数…読み込み回数、サイズ、ファイル名
			rcount,size,filename
		出力…読み込み時間、最小値、最大値、平均値、Mbyte/s
		初期値、max=0.00 最大値、min=9999999.99 最小値、kei=0.00 合計
			mbps Mb/s計算用　idx　計算用一時保管　read_kei 実読込合計 */
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

/* gettimeにおける処理時間の算出。	*/
	gettimeofday (&start, NULL);
	gettimeofday (&stop, NULL);
	default_etime = e_time(&start, &stop);

/* rcount分読込loop */
	for (i = 1; i <= rcount;) {
		fd = open(filename, O_RDONLY, 0);
		if (fd < 0) {		/* open check */
			perror(filename);
			free(buf);
			return -1;
		}
		/* カウントされた回数読み込むか、EOFになるまで、EOFの場合bleak;で抜ける */
		while (i <= rcount) {
			int read_size;
			gettimeofday (&start, NULL);
			read_size = read(fd, buf, size);
			gettimeofday (&stop, NULL);

			if (read_size > 0 || size == 0) {
				read_kei += read_size;
				idx = (e_time(&start, &stop) - default_etime);
				printf("read-test %d   %.2fms. readsize=%d\n", i, idx / 1000, read_size);
				/* 最高値、最低値、合計の計算 */
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
/* 最終行表示 */
	printf("read-test Min %.2fms Max %.2fms Average %.2fms %.2fMbyte/s\n",
		min / 1000, max / 1000, (kei / rcount) / 1000, mbps);
	printf("io-bench: %d count read-test at %d byte Done.\n\n", rcount, size);
	free(buf);
	return 0;
}

/* 書き込み処理 　引数…書き込み回数、サイズ、ファイル名
			wcount,size,filename
		出力…書き込み時間、最小値、最大値、平均値、Mbyte/s
		初期値、max=0.00 最大値、min=9999999.99 最小値、kei=0.00 合計、
		average 平均計算用 mbps Mb/s計算用 idx　計算用一時保管
		i ループ用変数　flg 書き込みcheck用　*/
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
	if (fd < 0) {		/* ファイルopen check */
		perror(filename);
		free(buf);
		return -1;
	}

/* gettimeにおける処理時間の算出。	*/
	gettimeofday (&start, NULL);
	gettimeofday (&stop,  NULL);
	default_etime = e_time(&start, &stop);

/* wcount分だけ書き込み */
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
			/* 最高値、最低値、合計の計算 */
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
/* 最終行表示 */
	printf("write-test Min %.2fms Max %.2fms Average %.2fms %.2fMbyte/s\n",
		min / 1000, max / 1000, average, mbps);
	printf("io-bench: %d count write-test at %d byte Done.\n\n", wcount, size);
	free(buf);
	return 0;
}

