EXE = bm2
CC = gcc
OBJS = HD6303.o main.o m68alt.o bm2mem.o bm2sub.o sound.o util.o menu.o init.o depend.o sdlxpm.o srecord.o conf.o

# SDL2.0
CFLAGS = -O3 -Wall $(shell sdl2-config --cflags)
CXXFLAGS = $(CFLAGS) --std=c++11
LDFLAGS = -s $(shell sdl2-config --libs)

# SDL1.2
#CFLAGS = -O3 -finline-limit-20000 -DM68_TRACE -DM68_SUB -Wall $(shell sdl-config --cflags)
#LDFLAGS = -s $(shell sdl-config --libs)

$(EXE): $(OBJS)
	$(CXX) -o $(EXE) $(OBJS) $(LDFLAGS)
win32: $(OBJS) resource.o
	$(CC) -o $(EXE) $(OBJS) resource.o $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<
bm2mem.o: bm2.h depend.h
bm2sub.o: bm2.h depend.h
depend.o: bm2.h depend.h bm2icon.xpm
main.o: bm2.h depend.h
sound.o: bm2.h depend.h
menu.o: menu.c bm2.h depend.h
	$(CC) -c $(CFLAGS) -Os $<
init.o: init.c bm2.h depend.h
	$(CC) -c $(CFLAGS) -Os $<
srecord.o: srecord.c srecord.h
	$(CC) -c $(CFLAGS) -Os $<
conf.o: conf.c conf.h
	$(CC) -c $(CFLAGS) -Os $<
sdlxpm.o: sdlxpm.c sdlxpm.h
	$(CC) -c $(CFLAGS) -Os $<
resource.o: resource.rc bm2.ico
	windres $< -o $@

install:
	cp $(EXE) /usr/local/bin

arc:
	rm -r -f bm2
	mkdir bm2
	cp -p *.c *.h *.xpm Makefile *.ico *.dll readme.txt bm2config.* bm2/
	tar -c bm2/ | gzip -9 > bm2all.tgz
	rm -r -f bm2
src:
	rm -r -f bm2
	mkdir bm2
	nkf -w8 readme.txt | sed 's/\r//' > bm2/readme.txt
	nkf -w8 bm2config.linux | sed 's/\r//' > bm2/bm2config
	cp -p main.c bm2.h bm2mem.c bm2sub.c sound.c util.c menu.c init.c depend.c depend.h srecord.c srecord.h conf.c conf.h sdlxpm.c sdlxpm.h bm2icon.xpm Makefile chr.bmp bm2/
	tar -c bm2/ | gzip -9 > bm2src.tgz
	rm -r -f bm2
win32arc:
	rm -r -f bm2win32
	mkdir bm2win32
	nkf -s readme.txt > bm2win32/readme.txt
	nkf -s bm2config.win32 > bm2win32/bm2config
	cp -p bm2.exe chr.bmp bm2win32/
	#cp -p SDL.dll bm2win32/
	#cp -p README-SDL.txt bm2win32/README-SDL.txt
	cp -p SDL2.dll bm2win32/
	cp -p README-SDL2.txt bm2win32/README-SDL.txt
	zip -u -9 bm2win32.zip bm2win32/*
	rm -r -f bm2win32
macarc:
	rm -r -f bm2mac
	mkdir bm2mac
	nkf -w8 readme.txt | sed 's/\r//' > bm2mac/readme.txt
	nkf -w8 bm2config.linux | sed 's/\r//' > bm2mac/bm2config
	cp -p bm2 chr.bmp bm2mac/
	zip -u -9 bm2mac.zip bm2mac/*
	rm -r -f bm2mac
zaurusarc:
	rm -r -f bm2zaurus
	mkdir bm2zaurus
	nkf -e readme.txt | sed 's/\r//' > bm2zaurus/readme.txt
	nkf -e bm2config.linux | sed 's/\r//' > bm2zaurus/bm2config
	cp -p bm2 chr.bmp bm2icon.xpm bm2zaurus/
	zip -u -9 bm2zaurus.zip bm2zaurus/*
	rm -r -f bm2zaurus
clean:
	rm -f *.o
	rm -f $(EXE)
	rm -f $(EXE).exe
