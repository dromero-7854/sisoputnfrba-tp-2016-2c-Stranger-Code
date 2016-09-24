/*
 * pueblo-paleta.c
 *
 *  Created on: 22/9/2016
 *      Author: utnso
 */

#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <commons/collections/list.h>

typedef struct PCB {
	char id;
	char posx;
	char posy;
	char *objetivos; //Si esto se comenta de la nada aparece la pokenest dentro de la listaPCB
	bool movVert; //Con char objetivos por lo menos toma el primero pero no el segundo
} PCB;
typedef struct PokeNest {
	char id;
	char cantidad;
	char posx;
	char posy;
} PokeNest;

void moverJugador(PCB *jug, PokeNest *pok)
{
	if((*jug).posx != (*pok).posx && !(*jug).movVert)
	{
		(*jug).posx < (*pok).posx ? (*jug).posx++ : (*jug).posx--;

		if((*jug).posy != (*pok).posy) (*jug).movVert = 1;
	}
	else if((*jug).posy != (*pok).posy && (*jug).movVert)
	{
		(*jug).posy < (*pok).posy ? (*jug).posy++ : (*jug).posy--;

		if((*jug).posx != (*pok).posx) (*jug).movVert = 0;
	}
	else (*jug).movVert = !(*jug).movVert;

	printf("DESPUES DE MOVER\nid: %c \nposx: %i\nposy: %i\n", jug->id, jug->posx, jug->posy);
}
void moverJugadores(t_list *listaPCB) {

	int i = 0;
	bool cont = 1;

	PokeNest pok;
	pok.id = 'P';
	pok.posx = 39;
	pok.posy = 50;

	while(cont) {

		for(; i < list_size(listaPCB); i++) {

			PCB *jug = NULL;
			jug = list_get(listaPCB, i);
			printf("Moviendo jugadores\nValor de i: %i \nid: %c \nposx: %i\nposy: %i\n", i, jug->id, jug->posx, jug->posy);
			moverJugador(jug, &pok);

		}
		cont = 0;
	}
}
void crearJugadores(t_list *listaPCB) {

	PCB ash;
	ash.id = 35;
	ash.posx = 1;
	ash.posy = 1;

	PCB misty;
	misty.id = 64;
	misty.posx = 10;
	misty.posy = 20;

	list_add(listaPCB, &ash);
	list_add(listaPCB, &misty);

	printf("CREANDO JUGADORES\n");

	int i;
	for(i = 0; i < list_size(listaPCB); i++) {

		PCB *personaje = list_get(listaPCB, i);

		printf("id: %c \nposx: %i \nposy: %i \n", personaje -> id, personaje -> posx, personaje -> posy);
	}
}

int main()
{
	//t_list *listaPN = list_create();
	t_list *listaPCB = malloc(sizeof(t_list));
	listaPCB = list_create();

	crearJugadores(listaPCB);
	moverJugadores(listaPCB); //puede ser que el error este en cuando mando la lista?

	return EXIT_SUCCESS;
}
