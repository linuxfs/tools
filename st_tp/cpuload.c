#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	int i, s;
	double d;

	daemon(0, 0);
	nice(10);

	for (;;) {
		for (i = 0; i < 99999; i++) {
			s += i;
		}
		s = s / 3;
		(void)getpid();
		for (i = 0; i < 99999; i++) {
			d += 0.1;
		}
		d = d / 3.0;
		(void)getuid();
	}
}
