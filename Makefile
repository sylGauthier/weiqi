DEPS := 3dmr

PREFIX ?= /usr
DIR=$(if $(DESTDIR),$(DESTDIR)/)$(PREFIX)
BIN_DIR=$(DIR)/$(or $(BINDIR),bin)
TEXTURE_DIR=$(DIR)/share/weiqi/textures
TEXTURE_SRC=./textures

CFLAGS ?= -std=c89 -pedantic -march=native -Wall -O2 -D_POSIX_C_SOURCE=200112L
CFLAGS += $(shell pkg-config --cflags $(DEPS)) -I.
LDFLAGS += $(shell pkg-config --libs $(DEPS)) -lm -lpthread

CFLAGS += -DW_TEXTURE_DIR=\"$(TEXTURE_DIR)/\" -DW_TEXTURE_SRC=\"$(TEXTURE_SRC)/\"

OBJECTS := $(patsubst %.c,%.o,$(wildcard src/*.c))
TEXTURES := wood.png wood2.png wood3.png
TEXFILES := $(addprefix $(TEXTURE_SRC)/, $(TEXTURES))

.PHONY: all install
all: weiqi $(TEXFILES)

weiqi: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJECTS) weiqi

$(TEXTURE_SRC)/%.png: $(TEXTURE_SRC)
	mkdir -p $(TEXTURE_SRC)
	wget pedantic.software/syg/files/$*.png -O $@

install: weiqi $(TEXFILES)
	mkdir -p $(BIN_DIR) $(TEXTURE_DIR)
	cp weiqi $(BIN_DIR)
	cp $(TEXFILES) $(TEXTURE_DIR)
	if [ ! -f ~/.weiqi ] ; then \
		touch ~/.weiqi ; \
		command -v gnugo && echo engine gnugo \"`which gnugo` --mode gtp\" >> ~/.weiqi ; \
	fi
