CC=gcc
CFLAGS=-g3 -Wall -Wextra -pedantic -std=c17
all=term.o buffer.o fileio.o input.o output.o jim.o
srcall = main.c term.c buffer.c fileio.c input.c output.c

jim: $(all)
	$(CC) $(CFLAGS) -o jim $(all)

jim.o: main.c
	$(CC) $(CFLAGS) -o jim.o -c main.c

output.o: output.c output.h
	$(CC) $(CFLAGS) -o output.o -c output.c

input.o: input.c input.h
	$(CC) $(CFLAGS) -o input.o -c input.c

fileio.o: fileio.c fileio.h
	$(CC) $(CFLAGS) -o fileio.o -c fileio.c

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) -o buffer.o -c buffer.c

term.o: term.c term.h
	$(CC) $(CFLAGS) -o term.o -c term.c

clean:
	rm -f *.o

touch:
	touch $(srcall)

