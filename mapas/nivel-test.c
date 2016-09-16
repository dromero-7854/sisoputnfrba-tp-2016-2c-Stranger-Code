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
#include <bibliotecaCharMander.c>

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

typedef struct Caja { //todavia no se usa
	char id;
	char posx;
	char posy;
	char cantidad;
} Caja;

int rows, cols;

void rnd(int *x, int max){
	*x += (rand() % 3) - 1;
	*x = (*x<max) ? *x : max-1;
	*x = (*x>0) ? *x : 1;
}

void moverJugador(char a, t_list* items, int x, int y, int p, int q, bool movVert) {

	if(x != p) {

		x < p ? x++ : x--;

		if(y != q) movVert = 1;

	}
	else if( y != q ) {
		y < q ? y++ : y--;

		if(x != p) movVert = 0;

	}
	else movVert = !movVert;

	MoverPersonaje(items, a, x, y);

}

void crearJugadores(t_list *listaPCB, t_list *items);
void moverJugadores(t_list *listaPCB, t_list *items);

int main(void) {

	t_list* items = list_create();
	t_list *listaPCB = list_create();


	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

    crearJugadores(listaPCB, items);

	CrearCaja(items, 'P', 25, 5, 5);
	CrearCaja(items, 'B', 10, 19, 5);

	nivel_gui_dibujar(items, "Aguante Stranger Code vieja no me importa nada");

	//moverJugadores(listaPCB, items);

	PCB *personaje = list_get(listaPCB, 1);

	while(1) {

		sleep(1);

		//moverJugador(personaje -> id, items, personaje -> posx, personaje -> posy, 25, 5, personaje -> movVert);

		if((personaje -> posx) != 25 && !(*personaje).movVert) {

			(personaje -> posx) < 25 ? personaje -> posx++ : personaje -> posx--;

			if((personaje -> posy) != 5) (*personaje).movVert = 1;

		}
		else if( personaje -> posy != 5 && (*personaje).movVert) {

			(personaje -> posy) < 5 ? personaje -> posy++ : personaje -> posy--;

			if(personaje -> posx!= 25) (personaje -> movVert) = 0;

		}
		else (*personaje).movVert = !(*personaje).movVert;

		MoverPersonaje(items, '#', personaje -> posx, personaje -> posy);

		nivel_gui_dibujar(items, "");

	}

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, 'P');
	BorrarItem(items, 'B');

	nivel_gui_terminar();

}

void crearJugadores(t_list * listaPCB, t_list *items) {

	int q, p;
	p = cols;
	q = rows;

	PCB ash;
	ash.id = '#';
	ash.estado = 'R';
	char objetivosAsh[] = { 'P', 'B'};
	ash.objetivos = objetivosAsh;
	ash.vidas = 1;
	ash.posx = 1;
	ash.posy = 1;
	ash.reintentos = 0;
	ash.movVert = 0;
	ash.quantum = 0;

	PCB misty;
	misty.id = '@';
	misty.estado = 'R';
	char objetivosMisty[] = {'B', 'P'};
	misty.objetivos = objetivosMisty;
	misty.vidas = 1;
	misty.posx = p;
	misty.posy = q;
	misty.reintentos = 0;
	misty.movVert = 0;
	misty.quantum = 0;

	list_add(listaPCB, &ash);
	list_add(listaPCB, &misty);

	int i;

	for(i = 0; i < list_size(listaPCB); i++) {

		PCB *personaje = list_get(listaPCB, i);
		CrearPersonaje(items, (*personaje).id, (*personaje).posx, (*personaje).posy);
	}
}
void moverJugadores(t_list * listaPCB, t_list *items)
{
	int i = 0;

	while(1)
	{
		PCB *personaje = list_get(listaPCB, i);

		while((*personaje).quantum < QUANTUM)
		{
			sleep(1);
			moverJugador(personaje -> id, items, personaje ->posx, personaje ->posy, 25, 5, personaje -> movVert);
			personaje -> quantum++;

			nivel_gui_dibujar(items, "");
		}

		i++;
		if(i == list_size(listaPCB)) i = 0;
	}
}
