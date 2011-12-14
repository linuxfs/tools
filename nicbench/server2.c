#include "local.h"

int pid, f_wait;

void sig_finish()
{
  //DbgPrint("\nsending shutdown...\n");
  f_wait = 0;
}


void 
read_socket(int new_sockfd, int unitsize, struct in_addr ipaddr)
{
  int cnt;
  double bytesent=0;
  long sec, usec;
  float	lib_elapsed;

  printf("%s-->Reference(size=%d): ", inet_ntoa(ipaddr), unitsize);
  fflush(stdout);

  gettimeofday(&time1,&tz);
  while ((cnt = read(new_sockfd, buf, unitsize)) > 0) {
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

  printf("%.0f(byte) in %.0f(sec).\t%5.2f (Mbps)\n",
	 bytesent, lib_elapsed, bytesent * 8.0 / 1024 /1024 / lib_elapsed);
}

void 
write_socket(int new_sockfd, int unitsize, int time_wait, struct in_addr ipaddr)
{
  int cnt;
  double bytesent=0;
  long sec, usec;
  float	lib_elapsed;

  printf("%s<--Reference(size=%d): ", inet_ntoa(ipaddr), unitsize);
  fflush(stdout);

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

  printf("%.0f(byte) in %.0f(sec).\t%5.2f (Mbps)\n",
	 bytesent, lib_elapsed, bytesent * 8.0 / 1024 /1024 / lib_elapsed);
}

main(int argc, char **argv)
{
  int client_len;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int c;
  int sockfd, new_sockfd;
  struct param paramater, *p_param;

  //DbgPrint("socket:\n");
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = SERVERPORT;

  //DbgPrint("bind:\n");
  if (bind(sockfd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("bind");
    close(sockfd);
    exit(1);
  }

  while (1) {
    //DbgPrint("listen:\n");
    if (listen(sockfd, 5) < 0) {
      perror("listen");
      close(sockfd);
      exit(1);
    }

    //DbgPrint("accept:\n");
    if((new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr,
                            &client_len)) < 0) {
      perror("accept");
      close(sockfd);
      exit(1);
    }

    //DbgPrint("new_socket:\n");
    read(new_sockfd, buf, sizeof(buf));   // Get transfer paramaters
    p_param = &paramater;
    memcpy(p_param, buf, sizeof(struct param));

    //DbgPrint("direction: %d, unitsize: %d, time: %d\n", p_param->direction, p_param->unitsize, p_param->time);

    switch(p_param->direction) {
    case 1: // Read Request
      //send ACK to synclonize, before starting translation.
      write(new_sockfd, p_param, sizeof(struct param));

      read_socket(new_sockfd, p_param->unitsize, client_addr.sin_addr);
      break;

    case 2: // Write Request
      write_socket(new_sockfd, p_param->unitsize, p_param->time, client_addr.sin_addr);
      break;

    case 3: // Terminate Session
      //DbgPrint("close: teminate session\n");
      close(new_sockfd);
      close(sockfd);
      exit(1);

    default:
      printf("direction=%d\n", p_param->direction);

    }
    close(new_sockfd);
  }

  close(new_sockfd);
  close(sockfd);
}
