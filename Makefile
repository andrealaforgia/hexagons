.DEFAULT_GOAL := all

CC = gcc

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    PLATFORM = LINUX
    INSTALL_CMD = sudo apt-get update && sudo apt-get install -y libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
    DEV_INSTALL_CMD = sudo apt-get update && sudo apt-get install -y cpplint clang-format
else ifeq ($(UNAME_S), Darwin)
    PLATFORM = OSX
    INSTALL_CMD = brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf
    DEV_INSTALL_CMD = brew install cpplint clang-format
else
    $(error Unsupported platform)
endif

SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LFLAGS := $(shell sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lSDL2_ttf

# Every directory under the engine core and the game sources holds one module
ENGINE_DIRS := $(patsubst %/,%,$(wildcard engine/core/*/))
GAME_DIRS := $(patsubst %/,%,$(wildcard game/src/*/))
SOURCE_DIRS := $(ENGINE_DIRS) $(GAME_DIRS)

SRC = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.c))
HEADERS = $(foreach dir,$(SOURCE_DIRS),$(wildcard $(dir)/*.h))

OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d)

-include $(DEP)

INCLUDES = -I. $(addprefix -I,$(SOURCE_DIRS))

CFLAGS := -ggdb3 -O3 --std=c99 -Wall -Wextra -pedantic-errors $(INCLUDES) $(SDL2_CFLAGS)
LFLAGS := $(SDL2_LFLAGS) -lm

TARGET = hexagons

.PHONY: all install dev_install clean lint format

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

install:
	git submodule update --init --recursive
	$(INSTALL_CMD)

dev_install:
	git submodule update --init --recursive
	$(DEV_INSTALL_CMD)

lint:
	cpplint --filter=-build/include_subdir,-legal/copyright,-runtime/threadsafe_fn --root=engine $(SRC) $(HEADERS) tests/*.c engine/tests/*.c engine/tests/*.h

clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

format:
	clang-format -i -style=Google $(SRC) $(HEADERS)

.PHONY: test sanitize

test:
	python3 engine/tests/run_tests.py
	python3 tests/run_tests.py
	python3 tests/test_build.py

sanitize:
	python3 engine/tests/run_tests.py --sanitize
	python3 tests/run_tests.py --sanitize
