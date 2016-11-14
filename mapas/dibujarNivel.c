/*
 * dibujarNivel.c
 *
 *  Created on: 13/11/2016
 *      Author: utnso
 */

#include "nivel-test.h"
#include "dibujarNivel.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void dibujarNivel(t_combo * comboLista) {

	while(1) {

		pthread_mutex_lock(&mutex);

		nivel_gui_inicializar();
		nivel_gui_get_area_nivel(&rows, &cols);


		nivel_gui_dibujar(items, "Stranger Code");

		nivel_gui_terminar();

		pthread_mutex_unlock(&mutex);
	}

}
