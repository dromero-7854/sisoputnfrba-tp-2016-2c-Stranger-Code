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

void rnd(int *x, int max){
	*x += (rand() % 3) - 1;
	*x = (*x<max) ? *x : max-1;
	*x = (*x>0) ? *x : 1;
}

int main(void) {

	t_list* items = list_create();

	bool vaALaIzq = true;
	bool vaArriba = true;

	int rows, cols;
	int q, p;

	int x = 1;
	int y = 1;

	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

	p = cols;
	q = rows;

	CrearPersonaje(items, '@', p, q);
	CrearPersonaje(items, '#', x, y);

	CrearCaja(items, '$', 25, 5, 5);

	nivel_gui_dibujar(items, "Aguante Stranger Code vieja no me importa nada");

	while ( 1 ) {
		/*int key = getch();

		switch( key ) {

			case KEY_UP:
				if (y > 1) {
					y--;
				}
			break;

			case KEY_DOWN:
				if (y < rows) {
					y++;
				}
			break;

			case KEY_LEFT:
				if (x > 1) {
					x--;
				}
			break;
			case KEY_RIGHT:
				if (x < cols) {
					x++;
				}
			break;

			case 'Q':
			case 'q':
				nivel_gui_terminar();
				exit(0);
			break;
		}*/

		if(p <= 1) vaALaIzq = false;
		if(p >= cols) vaALaIzq = true;

		if(q <= 1) vaArriba = false;
		if(q >= rows) vaArriba = true;

		if(vaALaIzq) MoverPersonaje(items, '@', --p, q);
		if(!vaALaIzq) MoverPersonaje(items, '@', ++p, q);

		if(vaArriba) MoverPersonaje(items, '@', p, --q);
		if(!vaArriba) MoverPersonaje(items, '@', p, ++q);

		MoverPersonaje(items, '#', x, y);

		if((p == x) && (q == y)) {
			BorrarItem(items, '#');
		}
		if((p == 25 && q == 5) || (x == 25 && y == 5)) restarRecurso(items, '$');

		nivel_gui_dibujar(items, "");

		sleep(1);

	}

	BorrarItem(items, '#');
	BorrarItem(items, '@');

	BorrarItem(items, '$');

	nivel_gui_terminar();
}
