all:
	clang life.c -o life -Wall -pedantic -std=c11

clean:
	rm -rf *.o life

run: all
	./life
