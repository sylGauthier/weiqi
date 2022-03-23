#!/bin/sh

RELEASE_NAME="weiqi"
RELEASE_VER="0.0-alpha"
RELEASE_DIR="$RELEASE_NAME-$RELEASE_VER"

make clean
make -j8 DATA_DIR=./data CONFIG_DIR=./config.json RELEASE=1 PREFIX= || exit 1

SHADERS_PATH="$(pkg-config --cflags 3dmr | sed 's/.*-DTDMR_SHADERS_PATH=\\"\(.*\)\\" .*/\1/g')"

rm -rf "$RELEASE_DIR"
mkdir "$RELEASE_DIR" || exit 1

cp ./weiqi "$RELEASE_DIR"
cp ./config.json "$RELEASE_DIR"
cp ./weiqi.1 "$RELEASE_DIR"
cp ./README.md "$RELEASE_DIR"
cp -r ./data "$RELEASE_DIR"
cp -r "$SHADERS_PATH" "$RELEASE_DIR/shaders" || exit 1

tar -cvzf weiqi.tar.gz "$RELEASE_DIR"
