CC=gcc
CFLAGS=-I.

DEPS = sbcp.h common.h

SERV = server.o

CLI = client.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

chat: server client
	

server: $(SERV)
	$(CC) -o $@ $^ $(CFLAGS)

client: $(CLI)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f ./*.o *~
