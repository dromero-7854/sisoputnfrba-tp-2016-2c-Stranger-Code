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

void rnd(int *x, int max){
	*x += (rand() % 3) - 1;
	*x = (*x<max) ? *x : max-1;
	*x = (*x>0) ? *x : 1;
}

int moverJugador(char a, t_list* items, int x, int y, int p, int q, bool movVert);

int main(void) {

	t_list* items = list_create();

	int rows, cols;
	int q, p;

	int x = 1;
	int y = 1;

	bool vaVertical[2];
	vaVertical[0] = 0;
	vaVertical[1] = 0;

	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

	p = cols;
	q = rows;

	CrearPersonaje(items, '@', p, q);
	CrearPersonaje(items, '#', x, y);

	CrearCaja(items, 'P', 25, 5, 5);
	CrearCaja(items, 'B', 10, 19, 5);

	nivel_gui_dibujar(items, "Aguante Stranger Code vieja no me importa nada");

	while(1) {

		sleep(1);

		moverJugador('#', items, x, y, 25, 5, vaVertical[0]);

		moverJugador('@', items, p, q, 10, 19, vaVertical[1]);

		if(x != 25 && !vaVertical[0]) {

			x < p ? x++ : x--;

			if(y != 5) vaVertical[0] = 1;

		}
		else if( y != 5 && vaVertical[0]) {
			y < 5 ? y++ : y--;

			if(x != 25) vaVertical[0] = 0;

		}
		else vaVertical[0] = !vaVertical[0];

		MoverPersonaje(items, '#', x, y);

		if((p == 25 && q == 5) || (x == 25 && y == 5)) restarRecurso(items, 'P');

		nivel_gui_dibujar(items, "");

	}

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, 'P');
	BorrarItem(items, 'B');

	nivel_gui_terminar();

}
int moverJugador(char a, t_list* items, int x, int y, int p, int q, bool movVert) {

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

	return 1;

}
