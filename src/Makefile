CC ?= gcc
COMMON_CFLAGS := -Wall -Wextra -pedantic -std=c17
DEBUG_CFLAGS := -g3 $(COMMON_CFLAGS)
RELEASE_CFLAGS := -O2 $(COMMON_CFLAGS)
CFLAGS ?= $(DEBUG_CFLAGS)

all=term.o buffer.o fileio.o input.o output.o jim.o
srcall = main.c term.c buffer.c fileio.c input.c output.c

jim: $(all)
	$(CC) $(CFLAGS) -o jim $(all)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

jim.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean touch release
clean:
	rm -f *.o jim

touch:
	touch $(srcall)

release:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(RELEASE_CFLAGS)" jim

