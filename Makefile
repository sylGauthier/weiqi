DEPS := 3dmr

CFLAGS ?= -std=c89 -pedantic -march=native -Wall -g -D_POSIX_C_SOURCE=200112L
CFLAGS += $(shell pkg-config --cflags $(DEPS)) -I.
LDFLAGS += $(shell pkg-config --libs $(DEPS)) -lm -lpthread

OBJECTS := $(patsubst %.c,%.o,$(wildcard src/*.c))

.PHONY: all
all: weiqi

weiqi: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJECTS) weiqi
