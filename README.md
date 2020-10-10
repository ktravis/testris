# testris
tetris test

I made a custom engine on SDL2 for mostly 2d rendering (though 3d is supported, it just doesn't have much work put into it yet). This is a test of a few different systems:

- scenes
- menus
- text rendering
- general mesh rendering
- shader pipeline
- very simple serde
- sound effects
- sprite sheets/simple animation (working, not currently in use)

Features:

- rewind!
- replay files
- dynamic window scaling (native)

TODO:
- set replay filename (native)
- replay file safety checks
- replay format compression

Known issues:
- some rotational issues with tight spaces
- web scale canvas with window size
- web sound effects

## uses

- [stb_image](https://github.com/nothings/stb)
- [stb_truetype](https://github.com/nothings/stb)
- shader based on https://www.shadertoy.com/view/Ms3XWH
- DejaVuSansMono and Ubuntu fonts
- SDL2 / SDL_mixer

## requirements (native)

C++ (11) compiler, and:

### arch

```shell
pacman -Sy glew mesa sdl2
```

### debian

```shell
apt install libsdl2-dev libglew-dev
```

### fedora

```shell
dnf install glew-devel SDL2-devel
```

## build

```shell
make
build/main
# or
make run
```

## build for web

depends on [emscripten/emsdk](https://github.com/emscripten/emsdk)

```shell
make wasm
# output in build/js/
```
