# testris

**Play in your browser, [here!](https://ktravis.github.io/testris)**

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

## faq

- Why do this?
  
  Implementing an existing game is a great way to isolate the technical/mechanical details. The core creative side - game rules, etc., is already handled. Tetris has simple mechanics, but hides a lot of depth and nuance.

- Why does your C++ look like this?

  I really like C, and I'm not very experienced with C++. Mainly I end up writing C with some slight syntax adjustments, default parameters, and function overloading.
  
- Why are you including `.cpp` files directly / using one big header file?

  I'm aware that this isn't the traditional structure of a C++ program. I chose to avoid creating many individual compilation units because a) it's less complex, and b) it compiles from scratch significantly faster - the best-case performance is probably slightly worse, but any potential build caching issues are off the table.
  
- Why write engine code when you could be using ____ ?

  I like making the tools that make the product. Being realistic with expectations, this functions as a great learning exercise!

- Why can you rewind?! You can't rewind in tetris...

  Why not! I wanted a simple replay system and as a side effect, game states can be played in reverse.
  
- What is that sound that plays when you rewind?

  That's the sound of your [going-back-in-time potion](https://www.youtube.com/watch?v=xSXofLK5hFQ)
  
- ... can I turn it off?

  If you must.
