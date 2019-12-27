BUILDDIR=build
SRC=main.cpp game.cpp
HEADERS=game.h
OBJS=$(addprefix $(BUILDDIR)/,$(patsubst %.cpp,%.o,$(SRC)))
BIN=$(BUILDDIR)/main

CC=g++
CXXFLAGS=-std=c++11 -g -Iinclude/ -Wfatal-errors #-DDEBUG
LD_FLAGS=-lSDL2 -lGL -lGLEW -lm -lpthread

build: $(BIN)

$(BIN): $(OBJS)
	g++ $^ $(LD_FLAGS) -o $@

$(BUILDDIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

run: $(BIN)
	$(BIN)

clean:
	rm -f $(OBJS) $(BIN)
