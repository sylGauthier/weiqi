# weiqi

`weiqi` is a simple and elegant openGL client to play the ancient game of GO
(Chinese: weiqi). It aims at being small, CLI friendly and have few
dependencies.

For now, it only support playing against GTP engines and through Unix domain
sockets, but in the future it will be able to connect to mainstream go servers.

![screenshot](https://pedantic.software/syg/files/weiqi/4.png)

## Install

### Dependencies

- `openGL` >= 3
- `glew`
- `glfw3`
- `libpng`
- [3dmr](https://pedantic.software/projects/3dmr.html), our homemade 3D
  renderer.

### Build

To install `3dmr`:

```
git clone https://pedantic.software/git/3dmr
cd 3dmr
git checkout syg/gltf
sudo make install PREFIX=/usr GLTF=1
```

Then, to install `weiqi`:

```
git clone https://pedantic.software/git/weiqi
cd weiqi
sudo make install
```

## How to play

`weiqi` can play against local GTP engines or through a UNIX domain socket.

`gnugo` is a decent engine for beginner to intermediary players and is available
on most linux distributions. Katago is a state of the art, CNN-based open source
engine that also supports GTP, although it wasn't tested with `weiqi`.

### Configuring engines

When installing `weiqi`, if you already have `gnugo` installed, the build system
will detect it and configure it automatically. Otherwise, or if you wish to add
another engine, edit `weiqi`'s config file located in `$HOME/.weiqi`, and add
the following line (for `gnugo`):

```
engine gnugo "/usr/bin/gnugo --mode gtp" # or whatever the path to the executable is
```

You can add other engines the same way, look up their documentation on how to
get them to use the GTP protocol.

### Play

#### Against an engine

Once the engine added, to play as black against the engine as white:

```
weiqi -b human -w gnugo
```

This will start a game on the standard 19x19 board but you can set the size with
the `-s` option, for example `weiqi -s 9` to play on a 9x9 board.

#### Through a UNIX domain socket

You can tell `weiqi` to create and listen on a UNIX domain socket for a player
by specifying `socket:<path-to-socket>` instead of an engine:

```
weiqi -w human -b socket:/tmp/weiqiconn
```

`weiqi` will expect another instance of `weiqi` to connect to it in client mode.

To connect to an already existing UNIX domain socket in client mode:

```
weiqi --client human <path-to-socket>
```

Note that you can put an engine instead of `human`. You can use a socket relay
tool such as `socat` to play on the network through TCP sockets. Look up the
wrapper `wrappers/qitcp` to do that easily.

### Interface

Left click to place a stone when it is your turn, or press `P` to pass. Press
backspace to cancel your last move (if the GTP engine allows it).

You can rotate the board by pressing right click and dragging the mouse. You can
zoom in/out with the wheel. You can reset the orientation and zoom by pressing
the `HOME` key.

## Configure the interface

`weiqi` looks for a configuration file in `$HOME/.weiqi` or assume defaults if
none is found.

The configuration file allows you to customize the look of the board, by setting
the shader used, the board texture, the image used for image-based lighting and
so on, as well as the default game mode, player assignation and board size.

Look up the examples in the `configs` folder. Here are screenshots of what they
correspond to:

`configs/fancy`

![fancy](https://pedantic.software/syg/files/weiqi/1.png)

`configs/fancy-direct-lighting`

![fancy](https://pedantic.software/syg/files/weiqi/3.png)

`configs/minimal`

![fancy](https://pedantic.software/syg/files/weiqi/2.png)

## More info

The man page details all configuration and command line options extensively:
`man weiqi`.
