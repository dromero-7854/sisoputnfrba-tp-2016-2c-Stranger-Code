/*
 * nivel-test.c


 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#include <curses.h>
#include <tad_items.h>
#include <nivel.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <bibliotecaCharMander.c>
#include "nivel-test.h"
#include "planificacion.h"

#define QUANTUM 5

typedef struct PCB {
	char id;
	char estado;
	char *proximoMapa;
	char *objetivos;
	char vidas;
	char reintentos;
	char posx;
	char posy;
	char quantum;
	bool movVert;

} PCB;

typedef struct PokeNest {
	char id;
	char posx;
	char posy;
	char cantidad;
} PokeNest;

int rows, cols;


void crearJugadores(t_list *listaPCB, t_list *items);
void moverJugadores(t_list *listaPCB, t_list *items);
void moverJugador(PCB *personaje, t_list *items,int x,int y);


int main(void) {/*

	t_list* items = list_create();
	t_list *listaPCB = list_create();

	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

    crearJugadores(listaPCB, items);

	CrearCaja(items, 'P', 25, 5, 5);
	CrearCaja(items, 'B', 10, 19, 5);

	nivel_gui_dibujar(items, "Aguante Stranger Code vieja no me importa nada");

	moverJugadores(listaPCB, items);

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, 'P');
	BorrarItem(items, 'B');

	nivel_gui_terminar();*/


	return EXIT_SUCCESS;
}

void crearJugadores(t_list * listaPCB, t_list *items) {

	int q, p;
	p = cols;
	q = rows;

	PCB ash;
	ash.id = 35;
	ash.posx = 1;
	ash.posy = 1;
	ash.movVert = 0;
	ash.quantum = 0;

	PCB misty;
	misty.id = '@';
	misty.posx = p;
	misty.posy = q;
	misty.movVert = 0;
	misty.quantum = 0;

	list_add(listaPCB, &ash);
	list_add(listaPCB, &misty);

	int i;

	for(i = 0; i < list_size(listaPCB); i++) {

		PCB *personaje = list_get(listaPCB, i);
		CrearPersonaje(items, personaje -> id, personaje -> posx, personaje -> posy);
	}
}
void moverJugadores(t_list * listaPCB, t_list *items)
{
	int i = 0;

	while(1)
	{
		PCB *personaje = list_get(listaPCB, i);

		while(personaje -> quantum < QUANTUM)
		{
			sleep(1);
			moverJugador(personaje, items, 25, 5);
			personaje -> quantum++;

			char o[2];
			o[0] = personaje -> id;

			nivel_gui_dibujar(items, 0);
		}

		personaje -> quantum = 0;

		i++;
		if(i == list_size(listaPCB)) i = 0;

	}
}

void moverJugador(PCB *personaje, t_list *items, int x, int y) {

	if(personaje-> posx < x && !(personaje->movVert)) {
		personaje->posx < x ? personaje->posx++ : personaje->posx--;

		if(personaje->posy != y) personaje->movVert = 1;
	}
	else if(personaje->posy != y && personaje->movVert) {

		personaje->posy < y ? personaje->posy++ : personaje->posy--;

		if(personaje ->posx !=x) personaje->movVert=0;
	}
	else personaje->movVert = !personaje->movVert;

	MoverPersonaje(items, personaje -> id, ((*personaje).posx), ((*personaje).posy));

}

void manejar_select(int socket, t_log* log){
	fd_set lectura, master;
	int nuevaConexion, a, recibido, fdMax;
	char buf[512];
	fdMax = socket;
	FD_ZERO(&lectura);
	FD_ZERO(&master);
	FD_SET(socket, &master);
	while(1){
		lectura = master;
		select(fdMax +1, &lectura, NULL, NULL, NULL);
		for(a = 0 ; a <= fdMax ; a++){
			if(FD_ISSET(a, &lectura)){
					if(a == socket){
						nuevaConexion = aceptar_conexion(socket, log);
						FD_SET(nuevaConexion, &master);
						if(nuevaConexion > fdMax) fdMax = nuevaConexion;
					}else {
						recibido = recv(a,  (void*) buf, 512, 0);
						if(recibido <= 0){
							if(recibido < 0){
								log_error(log, "Error al recibir de %d", a);
								printf("error");
								} else {
								log_error(log, "Se desconecto %d", a);
								printf("Se desconecto alguien\n");
								}
							close(a);
							FD_CLR(a, &master);
							} else {
								log_trace(log, "Me estan hablando sin permiso");
								printf("Que mandas?? Estas flashiando\n");
							}
						}
				}
		}
	}
}
