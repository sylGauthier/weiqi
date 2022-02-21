include config.mk

DEPS := 3dmr lib3dasset lib3dnk jansson

DIR=$(if $(DESTDIR),$(DESTDIR)/)$(PREFIX)
BIN_DIR=$(DIR)/$(or $(BINDIR),bin)
MAN_DIR=$(DIR)/share/man/man1
DATA_DIR=$(DIR)/share/weiqi
TEXTURE_DIR=$(DATA_DIR)/textures

DATA_SRC=data
TEXTURE_SRC=$(DATA_SRC)/textures

CFLAGS += $(shell pkg-config --cflags $(DEPS)) -I.
LDFLAGS += $(shell pkg-config --libs $(DEPS)) -lm -lpthread

CFLAGS += -DW_DATA_DIR=\"$(DATA_DIR)\" -DW_DATA_SRC=\"$(DATA_SRC)\"
CFLAGS += $(if $(COORDINATES),-DW_COORDINATES)

OBJECTS := $(patsubst %.c,%.o,$(wildcard src/*.c))
TEXTURES := wood.png wood2.png wood3.png sky.hdr
TEXFILES := $(addprefix $(TEXTURE_SRC)/, $(TEXTURES))
FONTFILES := $(DATA_SRC)/font.ttf

.PHONY: all install
all: weiqi $(TEXFILES) $(FONTFILES)

weiqi: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(OBJECTS) weiqi

$(TEXTURE_SRC)/%:
	test -d $(TEXTURE_SRC) || mkdir -p $(TEXTURE_SRC)
	curl "https://pedantic.software/syg/files/weiqi/data/$*" > $@

$(DATA_SRC)/%.ttf:
	test -d $(DATA_SRC) || mkdir -p $(DATA_SRC)
	curl "https://pedantic.software/syg/files/weiqi/data/$*.ttf" > $@

install: weiqi $(TEXFILES) $(FONTFILES)
	mkdir -p $(BIN_DIR) $(TEXTURE_DIR) $(MAN_DIR)
	cp weiqi $(BIN_DIR)
	cp wrappers/* $(BIN_DIR)
	cp $(TEXFILES) $(TEXTURE_DIR)
	cp $(FONTFILES) $(DATA_DIR)
	cp weiqi.1 $(MAN_DIR)
	if [ ! -f ~/.weiqi ] ; then \
		touch ~/.weiqi ; \
		command -v gnugo && echo engine gnugo \"`which gnugo` --mode gtp\" >> ~/.weiqi ; \
	fi
