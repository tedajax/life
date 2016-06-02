all:
	clang life.c -o life -g -O0 -Wall -pedantic -std=c11 -lsdl2

clean:
	rm -rf *.o life

run: all
	./life
