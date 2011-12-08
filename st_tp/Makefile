CFILES	= nload01.c nload02.c lock_bench.c lock_iotest.c memload.c \
	  Bonnie.c metatest.c
CMD	= $(CFILES:.c=)
CFLAGS	= -Wall -O2

all:  $(CMD) cpuload

$(CMD):
	$(CC) $(CFLAGS) -o $@ $@.c 

cpuload:
	$(CC) -Wall -o cpuload cpuload.c

clean:
	/bin/rm -f $(CMD) cpuload *.bak core.*

