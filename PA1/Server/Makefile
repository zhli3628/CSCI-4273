EXE=server
CC=gcc
CFLAGS=-O3 -Wall
CLEAN=rm -f $(EXE) *.o *.a

all: $(EXE)

server:udp_server.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(CLEAN)
