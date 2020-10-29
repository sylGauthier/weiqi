DEPS := 3dmr

CFLAGS ?= -std=c89 -pedantic -march=native -Wall -g -D_POSIX_C_SOURCE=200112L
CFLAGS += $(shell pkg-config --cflags $(DEPS)) -I.
LDFLAGS += $(shell pkg-config --libs $(DEPS)) -lm -lpthread

OBJECTS := $(patsubst %.c,%.o,$(wildcard src/*.c))
TEXTURES := wood.png wood2.png wood3.png
TEXDIR := textures
TEXFILES := $(addprefix $(TEXDIR)/, $(TEXTURES))

.PHONY: all
all: weiqi $(TEXFILES)

weiqi: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJECTS) weiqi

$(TEXDIR):
	mkdir -p $@

$(TEXDIR)/%.png: $(TEXDIR)
	wget pedantic.software/syg/files/$*.png -O $@
