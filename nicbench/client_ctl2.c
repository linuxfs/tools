#include <sys/types.h>
#include <sys/wait.h>
//#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static char *str_opt[3] =
{
  "-s", //Send
  "-r", //Receive
  "-t"  //Terminate
};

void
usage(void)
{
  fprintf(stderr, "Usage: CMD command_name packet_size time_interval IP_addr <IP_addr2> ...\n");
  exit(1);
}


int main(argc, argv)
     int argc;
     char *argv[];
{
  int i, j, k, time, maxproc;
  int childcnt;
  int status;
  char cmdline[256];
  pid_t pid;
  char *str_cmd;
  char *str_unitsize;
  char *str_timewait;
  char *str_ipaddr[10];
  int num_targets;

  if(argc<4) {
    usage();
  }

  time = 5;
  str_cmd = argv[1];
  str_unitsize = argv[2];
  str_timewait = argv[3];
  for(i=4; i<argc; i++) {
    str_ipaddr[i-4] = argv[i];
  }

  num_targets = argc - 4;
  childcnt = 0;

  //for(k=0; k<3; k++) {
  for(k=0; k<2; k++) { // don't send terminate request

    for(j=0; j<time; j++) {
  
      for(i=0; i<num_targets; i++) {
  
  	bzero(cmdline, 256);
  	strcpy(cmdline, str_cmd);
  	strcat(cmdline, " ");
  	strcat(cmdline, str_opt[k]);
  	strcat(cmdline, " -H ");
  	strcat(cmdline, str_ipaddr[i]);
  	strcat(cmdline, " -S ");
  	strcat(cmdline, str_unitsize);
  	strcat(cmdline, " -T ");
  	strcat(cmdline, str_timewait);

  	//printf("%s\n", cmdline);
  
  	switch(pid = fork())
 	  {
 	  case -1:
 	    printf("Error! \n"); fflush(stdout);
 	    return 1;
 	    
 	  case 0:
 	    execlp("/bin/sh", "sh", "-c", cmdline, (char *)0 );
 	    _exit(2);
  
 	  default:
 	    childcnt++;
 	    //printf("PID %d is running. Total %d processes.\n", pid, childcnt);
  
 	  }
      }
  
      while(childcnt) {
  	//printf("Zzz...\n"); fflush(stdout);
  	wait(&status);
  	childcnt--;
      }
      //printf("waking up\n"); fflush(stdout);

//      if(k > 1)
//      	goto end;
    }
  }

// end:
  //printf("FINISH\n"); fflush(stdout);
}


