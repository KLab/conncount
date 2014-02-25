CC=gcc

all: conncount.c
	$(CC) -o conncount conncount.c

clean:
	rm -f conncount
