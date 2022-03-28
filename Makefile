include config.mk

DEPS := 3dmr 3dasset 3dnk jansson

ifneq ($(RELEASE),)
	CFLAGS	+= $(shell pkg-config --cflags-only-I $(DEPS)) -I. -DTDMR_SHADERS_PATH=\"./shaders/\" -DTDMR_GLTF=1
	CFLAGS	+= -march=x86-64
	LDFLAGS	+= $(shell pkg-config --libs-only-L $(DEPS)) -l:lib3dasset.a -l:lib3dnk.a -l:lib3dmr.a -l:libjpeg.a -l:libjansson.a -l:libpng16.a -l:libGLEW.a -l:libGLU.a -l:libz.a -lm -lglfw -lGL -lX11 -lpthread
else
	CFLAGS	+= $(shell pkg-config --cflags $(DEPS)) -I.
	LDFLAGS	+= $(shell pkg-config --libs $(DEPS)) -lm -lpthread
endif

DIR			 = $(if $(DESTDIR),$(DESTDIR)/)$(PREFIX)
BIN_DIR		 = $(DIR)/$(or $(BINDIR),bin)
MAN_DIR 	 = $(DIR)/share/man/man1
DATA_DIR	?= $(DIR)/share/weiqi
TEXTURE_DIR	 = $(DATA_DIR)/textures

DATA_SRC	 = data
TEXTURE_SRC	 = $(DATA_SRC)/textures

CFLAGS += $(if $(CONFIG_DIR),-DW_CONFIG_DIR=\"$(CONFIG_DIR)\") -DW_DATA_DIR=\"$(DATA_DIR)\"
CFLAGS += $(if $(COORDINATES),-DW_COORDINATES)

OBJECTS		:= $(patsubst %.c,%.o,$(wildcard src/*.c))
TEXTURES	:= wood.png wood2.png wood3.png sky.hdr splash.png
TEXFILES	:= $(addprefix $(TEXTURE_SRC)/, $(TEXTURES))
FONTFILES	:= $(DATA_SRC)/font.ttf

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
	if [ ! -d ~/.config/weiqi ] ; then \
		mkdir -p ~/.config/weiqi ; \
	fi
	if [ ! -f ~/.config/weiqi/config.json ] ; then \
		cp ./config.json ~/.config/weiqi/config.json ; \
	fi
