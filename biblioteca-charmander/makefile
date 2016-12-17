libBibliotecaCharMander.so:	bibliotecaCharMander.o
	gcc -shared -o libBibliotecaCharMander.so bibliotecaCharMander.o

bibliotecaCharMander.o:	bibliotecaCharMander.h
	gcc -c -fpic bibliotecaCharMander.c

clean:
	rm libBibliotecaCharMander.so bibliotecaCharMander.o
