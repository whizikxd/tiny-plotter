#!/bin/sh

compile_flags="-O0 -g -Wall $(pkg-config --cflags sdl3 sdl3-ttf)"
linker_flags="$(pkg-config --libs sdl3 sdl3-ttf)"

cc $compile_flags -o plot src/*.c $linker_flags
