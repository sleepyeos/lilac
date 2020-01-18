CC = gcc
CFLAGS = -fPIC -std=gnu99
LDFLAGS = -shared
SOURCE = src
TARGET = bin/liblilac.so

lilac: lilac.o mutex.o
	rm -f bin/*
	rm -f include/*.h
	rm -f launcher
	$(CC) -o $(TARGET) $(CFLAGS) -shared mutex.o lilac.o
	cp -f src/*.h include
	make clean

lilac.o: $(SOURCE)/lilac.c mutex.o
	$(CC) -c $(CFLAGS) $<

mutex.o: $(SOURCE)/mutex.c
	$(CC) -c $(CFLAGS) $<

main: $(TARGET) $(SOURCE)/main.c
	$(CC) -o launcher $(SOURCE)/main.c -L./bin/ -Wl,-rpath=bin/ -llilac

.PHONY: clean
clean:
	rm -f *.o
