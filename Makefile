
CC=clang
CFLAGS=-std=c11 -Wall -g -c

BINS=mythreads

all: $(BINS)

%: %.c                                                                                                        
	$(CC) $(CFLAGS) -o $@ $? -pthread 

mythreads: mythreads.c list.o
	$(CC) $(CFLAGS) -o $@ mythreads.c list.c

clean: 
	rm -rf $(BINS) *.o *.dSYM
