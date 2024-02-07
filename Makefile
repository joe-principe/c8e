# $@ - Target name
# $^ - Target dependencies
# This is a generic make file for building raylib on linux

CC = gcc
CFLAGS = -std=c99 -Wall -Wpedantic -Wextra
RAYLIB_PATH = /home/joe/raylib

c8e: c8e.c
	$(CC) $^ $(CFLAGS) -I. -I$(RAYLIB_PATH)src -I$(RAYLIB_PATH)/src/external \
		-L. -L$(RAYLIB_PATH)/src -L$(RAYLIB_PATH) -lraylib -lGL -lm -lpthread \
		-ldl -lrt -lX11 -o $@

debug: c8e.c
	$(CC) $^ $(CFLAGS) -I. -I$(RAYLIB_PATH)src -I$(RAYLIB_PATH)/src/external \
		-L. -L$(RAYLIB_PATH)/src -L$(RAYLIB_PATH) -lraylib -lGL -lm -lpthread \
		-ldl -lrt -lX11 -g -o $@.o
