/*
 * spclient.c
 *   偽二重化管理コマンドが偽スピンドルコピーデーモンへ要求を
 *   行うテストプログラムで
 *   STREAM型のソケットを使いま
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>  /* #include < sys/un.h >の代わり */
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h> /* wait */
#include <unistd.h> /* exit */
#include <ctype.h> /* isalpha */
#include <stdlib.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include "spcopy_net.h"

void usage(void)
{
	printf("pfs_dp fromhost frompath tohost topath\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int	acc, i, ret = 0;
	char	hostname[256 + 1];
	struct pfs_spcopy_req req;
	struct pfs_spcopy_ack ack;

	if (argc != 5)
		usage();

	memset(hostname, 0, 257);
	strcpy(hostname, argv[1]);
	req.magic = SPCOPYD_MAGIC;
	req.status = SPCOPY_FULL;
	req.to = (uint64_t)htonl(get_addr(argv[3], &ret));
	if (ret != 0) {
		printf("get addr error %s[%d]\n", strerror(-ret), -ret);
		exit(1);
	}

	printf("copy from %s [%llu] to %s [%llu]\n",
		argv[1], (uint64_t)htonl(get_addr(argv[1], &ret)),
		argv[3], (uint64_t)htonl(get_addr(argv[3], &ret)));
	strcpy(req.frompath, argv[2]);
	strcpy(req.topath, argv[4]);

	if ((acc = spcopy_connect(hostname)) < 0) {
		printf("connect error %s[%d]\n", strerror(-acc), -acc);
		exit(1); /* 初期化 */
	}
	if ((ret = spcopy_send_recv(acc, &req, &ack)) < 0) {
		printf("connect error %s[%d]\n", strerror(-ret), -ret);
		 exit(1); /* 送受信 */
	}

	req.status = SPCOPY_STAT;
	for (i = 0; ack.ack_status == 0; i++) {
		if ((ret = spcopy_send_recv(acc, &req, &ack)) < 0) {
			printf("connect error %s[%d]\n", strerror(-ret), -ret);
			exit(1); /* 送受信 */
		}
		printf("ack [%2d]: %d \n", i, ack.ack_status);
		printf("[stat] copyst %d copytype %"PRIu32" files %"PRIu64"/%"PRIu64" size %"PRIu64"/%"PRIu64" times %"PRIu64"\n",
			ack.spproc.copy_status, ack.spproc.copy_type, ack.spproc.files, ack.spproc.all_files,
			ack.spproc.size, ack.spproc.all_size, ack.spproc.runtimes);
		if (!ack.ack_status && (ack.spproc.copy_status < 11))
			sleep(20);
	}

	if ((ret = spcopy_close(acc)) < 0) {
		 printf("connect error %s[%d]\n", strerror(-ret), -ret);
		exit(1); /* 後処理 */
	}
	return 0;
}

