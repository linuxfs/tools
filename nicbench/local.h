#include <stdio.h>
#include <fcntl.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <signal.h>

#define DbgPrint printf

struct param {
  int direction;        // 1:read, 2:write, 3:transfer end, other:terminate
#if 0
  int count;            // data size: 32K data will be transfered COUNT time
#endif
  int unitsize;         // transfer packet size 
  int time;             // transfer time
};

int f_wait;

struct timeval time1,time2;
struct timezone tz;


#define SERVERPORT 8000
#define UNITSIZE (32*1024)
#define MIN_UNITSIZE (1024)
#define MAX_UNITSIZE (128*1024)

char buf[ MAX_UNITSIZE ];

#define TIMEWAIT 60
#define MIN_TIME 10
#define MAX_TIME 600

