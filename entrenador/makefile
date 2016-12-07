entrenador:	pokemon.o mapa.o entrenador.o coordenadas.o conexion.o helper.o main.o
	gcc -L../biblioteca-charmander/Debug -Wall -o entrenador pokemon.o mapa.o entrenador.o coordenadas.o conexion.o helper.o main.o -lcommons -lBibliotecaCharMander

pokemon.o:	src/Pokemon/pokemon.c src/Pokemon/pokemon.h
	gcc -c src/Pokemon/pokemon.c
			
mapa.o:	src/Mapa/mapa.c src/Mapa/mapa.h
	gcc -c src/Mapa/mapa.c
		
entrenador.o:	src/Entrenador/entrenador.c src/Entrenador/entrenador.h
	gcc -c src/Entrenador/entrenador.c
				
coordenadas.o: src/Coordenadas/coordenadas.c src/Coordenadas/coordenadas.h 
	gcc -c src/Coordenadas/coordenadas.c
	
conexion.o:	src/Conexion/conexion.c src/Conexion/conexion.h
	gcc -c src/Conexion/conexion.c

helper.o:	src/helper.c src/helper.h
	gcc -c src/helper.c
		
main.o:	src/main.c
	gcc -c src/main.c
	
clean:
	rm entrenador pokemon.o mapa.o entrenador.o coordenadas.o conexion.o helper.o main.o