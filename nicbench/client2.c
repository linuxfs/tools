#include "local.h"

int pid, f_wait;

void sig_finish()
{
  //DbgPrint("\nsending shutdown...\n");
  f_wait = 0;
}


void
usage(void)
{
  fprintf(stderr, "Usage: CMD -[srtSTH]\n");
  fprintf(stderr, "       -s Send data\n");
  fprintf(stderr, "       -r Receive data\n");
  fprintf(stderr, "       -t Terminate Pipe\n");
  fprintf(stderr, "       -S <Buffer Size> [1024-131072](byte: default=32768)\n");
  fprintf(stderr, "       -T <Transfer Time> [10-180](sec: default=60)\n");
  fprintf(stderr, "       -H <IP address>\n");
  fprintf(stderr, "       Count: times for transfering 32kb data\n");
  exit(1);
}


void 
read_socket(int new_sockfd, int unitsize, char *ipaddr)
{
  int cnt;
  double bytesent=0;
  long sec, usec;
  float	lib_elapsed;

  gettimeofday(&time1,&tz);

  while ((cnt = read(new_sockfd, buf, unitsize)) > 0){
    bytesent += cnt;
  }

  gettimeofday(&time2,&tz);
  if (time2.tv_usec < time1.tv_usec) {
    time2.tv_usec += 1000000;
    time2.tv_sec -= 1;
  }
  sec = time2.tv_sec - time1.tv_sec;
  usec = time2.tv_usec - time1.tv_usec;
  lib_elapsed = (float)sec + ((float)usec/(float)1000000.0);

  printf("Target<--%s(size=%d): ", ipaddr, unitsize);
  printf("%.0f(byte) in %.0f(sec).\t%5.2f (Mbps)\n",
	 bytesent, lib_elapsed, bytesent * 8.0 / 1024 /1024 / lib_elapsed);
}

void 
write_socket(int new_sockfd, int unitsize, int time_wait, char *ipaddr)
{
  int cnt;
  double bytesent=0;
  long sec, usec;
  float	lib_elapsed;

  gettimeofday(&time1,&tz);

  switch(pid=fork())
    {
    case -1:
      _exit(1);

    case 0:
      sleep(time_wait);
      _exit(2);

    default:
      signal(SIGCHLD, sig_finish);
      f_wait = 1;

      while (f_wait){
  	cnt = write(new_sockfd, buf, unitsize);
	bytesent += cnt;
      }
    }

  gettimeofday(&time2,&tz);
  //shutdown(new_sockfd);
  close(new_sockfd);

  if (time2.tv_usec < time1.tv_usec) {
    time2.tv_usec += 1000000;
    time2.tv_sec -= 1;
  }
  sec = time2.tv_sec - time1.tv_sec;
  usec = time2.tv_usec - time1.tv_usec;
  lib_elapsed = (float)sec + ((float)usec/(float)1000000.0);

  printf("Target-->%s(size=%d): ", ipaddr, unitsize);
  printf("%.0f(byte) in %.0f(sec).\t%5.2f (Mbps)\n",
	 bytesent, lib_elapsed, bytesent * 8.0 / 1024 /1024 / lib_elapsed);
}

main(int argc, char **argv)
{
  int fd;
  int sockfd;
  struct sockaddr_in client_addr;
  int c;
  char sendflg = 0;
  char recvflg = 0;
  char terminateflg = 0;
  int unitsize = UNITSIZE;
  int time_wait = TIMEWAIT;
  char *ipaddr = "127.0.0.1";
  struct param paramater, *p_param;
  int count;
  char **p = argv;

  while ((c = getopt(argc, argv, "srtSTH:")) > 0) {

    p++;

    switch (c) {
    case 's':
      sendflg = 1;
      //DbgPrint(" %s\n", *p);
      p++;
      break;
    case 'r':
      recvflg = 1;
      //DbgPrint(" %s\n", *p);
      p++;
      break;
    case 't':
      terminateflg = 1;
      //DbgPrint(" %s\n", *p);
      p++;
      break;

    case 'S':
      unitsize = atoi(*p);
      if ((unitsize < MIN_UNITSIZE)||(unitsize > MAX_UNITSIZE)) {
	usage();
      }
#if 0
      if (unitsize < MIN_UNITSIZE) {
	unitsize = MIN_UNITSIZE;
      } else if (unitsize > MAX_UNITSIZE) {
	unitsize = MAX_UNITSIZE;
      }
#endif
      //DbgPrint(" unitsize=%d\n", unitsize);
      p++;
      break;

    case 'T':
      time_wait = atoi(*p);
      if ((time_wait < MIN_TIME)||(time_wait > MAX_TIME)) {
	usage();
      }
#if 0
      if (time_wait < MIN_TIME) {
	time_wait = MIN_TIME;
      } else if (time_wait > MAX_TIME) {
	time_wait = MAX_TIME;
      }
#endif
      //DbgPrint(" time_wait=%d\n", time_wait);
      p++;
      break;

    case 'H':
      ipaddr = *p;
      //DbgPrint(" ipaddr=%s\n", ipaddr);
      p++;
      break;

    default:
      usage();
    }
  }

#if 0
  if (*argv[1] != '-')
    usage();

  //count = atoi(argv[3]);
  ipaddr = argv[2];

  if (count == 0 || ipaddr == NULL)
    usage();
#endif
  
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  bzero((char *)&client_addr, sizeof(client_addr));
  client_addr.sin_family = PF_INET;
  client_addr.sin_addr.s_addr = inet_addr(ipaddr);
  client_addr.sin_port = SERVERPORT;
  
  if (connect(sockfd, (struct sockaddr *)&client_addr,
              sizeof(client_addr)) < 0) {
    perror("connect");
    close(sockfd);
    exit(1);
  }

  p_param = &paramater;
#if 0
  p_param->count = count;
#endif
  p_param->unitsize = unitsize;
  p_param->time = time_wait;

  if (sendflg) {
    p_param->direction = 1;
  } else if (recvflg) {
    p_param->direction = 2;
  } else {
    p_param->direction = 3;
  }
  write(sockfd, p_param, sizeof(struct param));

  if (sendflg) {
    read(sockfd, buf, sizeof(buf));   // Wait ACK.

    write_socket(sockfd, unitsize, time_wait, ipaddr);
  } else if (recvflg) {
    read_socket(sockfd, unitsize, ipaddr);
  }
  
  close(sockfd);
}
