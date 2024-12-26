CC=gcc
CFLAGS=-g3 -Wall -Wextra -pedantic -std=c17
all=term.o fileio.o input.o output.o jim.o
srcall = main.c term.c fileio.c input.c output.c

jim: $(all)
	$(CC) $(CFLAGS) -o jim $(all)

jim.o: main.c
	$(CC) $(CFLAGS) -o jim.o -c main.c

output.o: output.c
	$(CC) $(CFLAGS) -o output.o -c output.c

input.o: input.c
	$(CC) $(CFLAGS) -o input.o -c input.c

fileio.o: fileio.c
	$(CC) $(CFLAGS) -o fileio.o -c fileio.c

term.o: term.c
	$(CC) $(CFLAGS) -o term.o -c term.c

touch:
	touch $(srcall)

clean:
	rm -rf $(all)
