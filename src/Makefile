CC ?= gcc
COMMON_CFLAGS := -Wall -Wextra -pedantic -std=c17
DEV_CFLAGS := -g3 $(COMMON_CFLAGS)
REL_CFLAGS := -O2 $(COMMON_CFLAGS)

SRCFILES := main.c term.c buffer.c fileio.c input.c output.c
DEV_DIR := ./build/dev
REL_DIR := ./build/rel
DEV_OBJS := $(patsubst %.c,$(DEV_DIR)/%.o,$(SRCFILES))
REL_OBJS := $(patsubst %.c,$(REL_DIR)/%.o,$(SRCFILES))

all: jim_dev

jim: $(REL_OBJS)
	$(CC) $(REL_CFLAGS) -o $@ $(REL_OBJS)

jim_dev: $(DEV_OBJS)
	$(CC) $(DEV_CFLAGS) -o $@ $(DEV_OBJS)

$(DEV_DIR)/%.o: %.c | $(DEV_DIR)
	$(CC) $(DEV_CFLAGS) -c $< -o $@

$(REL_DIR)/%.o: %.c | $(REL_DIR)
	$(CC) $(REL_CFLAGS) -c $< -o $@

$(DEV_DIR) $(REL_DIR):
	mkdir -p $@

.PHONY: clean touch develop release
clean:
	rm -rf build jim jim_dev

touch:
	touch $(SRCFILES)

develop:
	$(MAKE) jim_dev

release:
	$(MAKE) jim

