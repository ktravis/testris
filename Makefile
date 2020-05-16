BUILDDIR=build
SRC=main.cpp game.cpp
HEADERS=game.h
OBJS=$(addprefix $(BUILDDIR)/,$(patsubst %.cpp,%.o,$(SRC)))
BIN=$(BUILDDIR)/main

CC ?= g++
CXXFLAGS=-x c++ -std=c++11 -g -Iinclude/ -Wfatal-errors #-DDEBUG
LD_FLAGS=-lSDL2 -lGL -lGLEW -lm -lpthread

build: $(BIN)

$(BIN): compile_flags.txt $(OBJS)
	g++ $(OBJS) $(LD_FLAGS) -o $@

$(BUILDDIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

run: $(BIN)
	$(BIN)

compile_flags.txt:
	@echo "$(CXXFLAGS) $(LD_FLAGS)" | sed -E "s/\s+/\n/g" > $@

export BINARYEN := /usr

EMSCRIPTEN_FLAGS := -lidbfs.js -s WASM=1 -s USE_SDL_MIXER=2 -s USE_LIBPNG=1 \
	-s USE_FREETYPE=1 -s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 \
	-s ERROR_ON_UNDEFINED_SYMBOLS=0 --preload-file assets/

wasm: $(BUILDDIR)/js/testris.html

$(BUILDDIR)/js/testris.html: assets/ $(SRC) $(HEADERS)
	@mkdir -p $(shell dirname $@)
	em++ $(SRC) $(EMSCRIPTEN_FLAGS) -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJS) $(BIN) compile_flags.txt
