# weiqi

`weiqi` is a simple and elegant openGL client to play the ancient game of GO
(Chinese: weiqi). It aims at being small, CLI friendly and have few
dependencies.

For now, it only support playing against GTP engines and through Unix domain
sockets, but in the future it will be able to connect to mainstream go servers.

![screenshot](https://sgauthier.fr/files/weiqi/screenshots/2.png)
![screenshot](https://sgauthier.fr/files/weiqi/screenshots/3.png)

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
PREFIX=/usr sudo make install
```

Then, to install `weiqi`:

```
git clone https://pedantic.software/git/weiqi
cd weiqi
sudo make install
```

## How to play

`weiqi` can only play against local GTP engines for now, so having one makes it
more useful. `gnugo` is a decent AI for beginner to intermediary players.

### Configuring engines

When installing `weiqi`, if you already have `gnugo` installed, the build system
will detect it and configure it automatically. Otherwise, or if you wish to add
another engine, edit `weiqi`'s config file located in `$HOME/.weiqi`, and add
the following line:

```
engine gnugo "/usr/bin/gnugo --mode gtp"
```

You can add other engines the same way, look up their documentation on how to
get them to use the GTP protocol.

### Play

Once the engine added, to play as black against the engine as white:

```
weiqi -b human -w gnugo
```

This will start a game on the standard 19x19 board but you can set the size with
the `-s` option, for example `weiqi -s 9` to play on a 9x9 board.

### Interface

You can rotate the board by pressing right click and dragging the mouse. You can
reset the orientation by pressing the `HOME` key.

The interface can use 2 different shaders, the default one is a modern
physic-based rendering method (PBR), the alternative one uses a solid color
shader, which is extremely fast but more minimal. You can use either by
specifying:

- `weiqi -i nice`: for the PBR shader.
- `weiqi -i pure`: for the solid shader.

You can also set the wood texture, or remove it and set a plain color instead.
Example, to play on a typical yellowish board:

```
weiqi -i pure --texture none --color 0.6 0.5 0.2
```

![screenshot](https://sgauthier.fr/files/weiqi/screenshots/5.png)

## More info

The man page details all configuration and command line options extensively:
`man weiqi`.
