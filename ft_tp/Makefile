CFILES	= mkfbmt.c io-bench.c hetero_ft.c kisoken.c rikaken.c \
	  lock_ft00.c lock_ft01.c lock_ft02.c lock_ft03.c lock_ft04.c truncate.c \
	  testseek.c opendir.c nbmrw5rev1.c opentest.c scale-rw.c
CMD	= $(CFILES:.c=)
CFLAGS	= -Wall -O2

all:  $(CMD)

$(CMD):
	$(CC) $(CFLAGS) -o $@ $@.c 

clean:
	/bin/rm -f $(CMD) *.bak *.test

