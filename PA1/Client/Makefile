EXE=client
CC=gcc
CFLAGS=-O3 -Wall
CLEAN=rm -f $(EXE) *.o *.a

all: $(EXE)

client:udp_client.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(CLEAN)
