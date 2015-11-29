CC=gcc
CFLAGS=-I.
DEPS = shared.h
OBJ = main.o appbundler.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

AppBundleGenerator: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -framework CoreFoundation
