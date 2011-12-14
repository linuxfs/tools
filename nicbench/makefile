all:	client2 server2 client_ctl2

client:		client.c
server:		server.c
client_ctl:	client_ctl.c
client2:	client2.c local.h
	cc -o client2 client2.c
server2:	server2.c local.h
	cc -o server2 server2.c
client_ctl2:	client_ctl2.c

clean:
	rm -f client client2 server server2 client_ctl client_ctl2

