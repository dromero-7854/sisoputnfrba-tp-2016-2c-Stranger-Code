mapa:	nivel-test.o deteccionDeadlock.o dibujarNivel.o planificacion.o solicitudes.o
	gcc -L/home/utnso/tp-2016-2c-Stranger-Code/biblioteca-charmander -Wall -o mapa deteccionDeadlock.o dibujarNivel.o nivel-test.o planificacion.o solicitudes.o -lBibliotecaCharMander -lcommons -lnivel-gui -lcurses -lpkmn-battle -lpthread

nivel-test.o:	nivel-test.c nivel-test.h
	gcc -c nivel-test.c
		
deteccionDeadlock.o:	deteccionDeadlock.c deteccionDeadlock.h
	gcc -c deteccionDeadlock.c
			
dibujarNivel.o:	dibujarNivel.c dibujarNivel.h
	gcc -c dibujarNivel.c
				
planificacion.o: planificacion.c planificacion.h 
	gcc -c planificacion.c
	
solicitudes.o:	solicitudes.c solicitudes.h
	gcc -c solicitudes.c
	
clean:
	rm mapa deteccionDeadlock.o dibujarNivel.o nivel-test.o planificacion.o solicitudes.o
