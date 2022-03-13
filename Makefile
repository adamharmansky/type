LIBS:=`pkg-config --libs fontconfig freetype2` -lGL -lpthread
CFLAGS:=-DDEBUG -g `pkg-config --cflags fontconfig freetype2`

type: type.c
	cc -c type.c $(CFLAGS)

example: type example.c
	cc example.c type.o $(LIBS) -g -lglfw -lm -o example

run: example
	./example

debug: example
	gdb -tui ./example

.PHONY: type run

# vi: noet
