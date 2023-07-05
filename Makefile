CC := gcc
CFLAGS := -pthread
.DEFAULT_GOAL := build


build: kvdb.c kvdbd.c hashtable.c hashtable.h const.h clientconst.h
	$(CC) kvdb.c -o kvdb
	$(CC) kvdbd.c hashtable.c -o kvdbd $(CFLAGS)
