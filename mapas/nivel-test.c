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
#include <pkmn/battle.h>
#include <pkmn/factory.h>

#define QUANTUM 5

typedef struct PCB {
	char id;
	char estado;
	char *proximoMapa;
	char *objetivos;
	int objetivoActual;
	char posx;
	char posy;
	char quantum;
	bool movVert;
	bool bloq;

} PCB;

typedef struct PokeNest {
	char id;
	char posx;
	char posy;
	char cantidad;
	t_queue * colaBloqueados;
	t_pokemon_type tipo;

} PokeNest;

int rows, cols;

void crearJugadores(t_list *listaPCB, t_list *items);
void moverJugadores(t_list *listaPCB, t_list *items);
void moverJugador(PCB *personaje, t_list *items,int x,int y);

int main(void) {

	t_list* items = list_create();
	t_list *listaPCB = list_create();

	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

    crearJugadores(listaPCB, items);

	CrearCaja(items, 'P', 25, 5, 5);
	CrearCaja(items, 'B', 10, 19, 5);

	nivel_gui_dibujar(items, "Stranger Code");

	moverJugadores(listaPCB, items);

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, 'P');
	BorrarItem(items, 'B');

	nivel_gui_terminar();

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
	char obj1[] = {'P', 'C'};
	ash.objetivos = &obj1;
	ash.objetivoActual = 0;
	ash.bloq = false;

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
void darRecurso(PCB * entrenador, PokeNest * poke, t_list * items) {


	if(poke -> cantidad > 0) {
		poke -> cantidad--;
		entrenador ->objetivoActual++;
		restarRecurso(items, poke->id);
	}
	else if(poke ->cantidad == 0) {
		queue_push(poke -> colaBloqueados, (PCB *)entrenador);
		entrenador->bloq= true;
	}
}
void detectarDeadlock(t_list * entrenadores, t_list * pokenest) {

	t_list * bloqueados = list_create();

	int i, j;
	int count1 = 0, count2 = 0;
	int tamEntr = list_size(entrenadores);
	int tamPok = list_size(pokenest);

	int matrizRetenido[tamEntr][tamPok];
	int matrizNecesita[tamEntr][tamPok];
	int recursosDisponibles[tamPok];
	int completado[tamEntr];

	for(i = 0; i < tamEntr; i++) {
		completado[i] = 0;

		for(j = 0; j < tamPok; j++) {
			matrizRetenido[i][j] = 0;
			matrizNecesita[i][j] = 0;
		}
	}
	for(i = 0; i < tamPok; i++) {
		PokeNest * pok = list_get(pokenest, i);
		recursosDisponibles[i] = pok -> cantidad;
	}

	for(i = 0; i < tamEntr; i++) {

		PCB *entrenador = list_get(entrenadores, i);
		int k = 0;
		for(k = 0; k < entrenador->objetivoActual; k++){
			char obj = *(entrenador->objetivos+k);

			for( j = 0; j < tamPok; j++) {
				PokeNest * pok = list_get(pokenest, j);

				if(pok ->id == obj)
					break;
			}
			matrizRetenido[i][j]++;
		}
		for(; entrenador ->objetivos+k != '\0'; k++) {
			char obj = *(entrenador->objetivos+k);
			for( j = 0; j < tamPok; j++) {
				PokeNest * pok = list_get(pokenest, j);

				if(pok ->id == obj)
					break;
			}
			matrizNecesita[i][j]++;
		}
	}
	while(count1 < tamEntr) {
		count2 = count1;
		int k = 0;
		for(i = 0; i < tamEntr; i++) {

			for(j = 0; j< tamPok; j++) {
				if(matrizNecesita[i][j] <= recursosDisponibles[j])
					k++;
			}
			if(k == tamPok && completado[i] == 0) {
				completado[i] = 1;

				for(j = 0; j < tamPok; j++) {
					recursosDisponibles[j] += matrizRetenido[i][j];
				}
				count1++;
			}
			k = 0;
		}
		if(count1 == count2) {
			//HAY DEADLOCK, HAY QUE LLAMAR AL ALGORITMO DE BATALLA
			break;
		}
	}

}
void matarJugador(PCB * entrenador) {

}
