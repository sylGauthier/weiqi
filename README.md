# weiqi

`weiqi` is a simple and elegant openGL client to play the ancient game of GO
(Chinese: weiqi). It aims at being small, CLI friendly and have few
dependencies.

For now, it only support playing against GTP engines and through Unix domain
sockets, but in the future it will be able to connect to mainstream go servers.

You can also play over IRC with [qirc](https://pedantic.software/git/qirc/about)
which allows you to use any IRC channel as a game lobby to publish/join/play
games using `weiqi`.

![screenshot](https://pedantic.software/syg/files/weiqi/screenshots/ui.png)
![screenshot](https://pedantic.software/syg/files/weiqi/screenshots/5.png)
![screenshot](https://pedantic.software/syg/files/weiqi/screenshots/6.png)

## Install

`weiqi` is still a work in progress, with only one contributor occasionally
working on it during his free time. There is no proper build yet but there will
be in the near future when I am satisfied with the program.

So for now, the only way to use it is to build it from source.

### Dependencies

- `openGL` >= 3
- `glew`
- `glfw3`
- `libpng`
- `jansson`
- [3dmr](https://github.com/sylGauthier/3dmr)
- [3dasset](https://github.com/sylGauthier/3dasset)
- [3dnk](https://github.com/sylGauthier/3dnk)

### Build

Once you have installed the dependencies, to install `weiqi`:

```
git clone https://github.com/sylGauthier/weiqi
cd weiqi
sudo make install
```

## How to play

`weiqi` can play against local GTP engines or through a UNIX domain socket.

`gnugo` is a decent engine for beginner to intermediary players and is available
on most linux distributions. Katago is a state of the art, CNN-based open source
engine that also supports GTP, although it wasn't tested with `weiqi`.

### Configuring engines

The default JSON configuration is already set up to play against `gnugo` in 3
different modes of difficulty. If you wish to add more engines, you can edit the
config file in `~/.config/weiqi/config.json`, look at how `gnugo` is declared
and add more engines the same way. They will show up in the start-up menu.

### Play

Launching `weiqi` without any arguments will open the start-up menu which is
pretty self-explanatory. However, you can jump straight into a game by using
command line arguments as follow.

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

`weiqi` looks for a configuration file in `$HOME/.config/weiqi/config.json` or
assume defaults if none is found.

The configuration file allows you to customize the look of the board, by setting
the shader used, the board texture, the image used for image-based lighting and
so on, as well as the default game mode, player assignation and board size.
