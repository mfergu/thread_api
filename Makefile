
CC=clang-3.6
CFLAGS=-Wall -g -c

BINS=mythreads

all: $(BINS)

%: %.c                                                                                                        
	$(CC) $(CFLAGS) -o $@ $? -pthread 

mythreads.a: mythreads.o list.o
	$(CC) $(CFLAGS) -o $@ mythreads.c list.c

clean: 
	rm -rf $(BINS) *.o *.dSYM
