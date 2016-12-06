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

	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

	while(1) {

		sem_wait(&sem_dibujo);
		pthread_mutex_lock(&dibujo);
		nivel_gui_dibujar(items, nombre_mapa);

		//sem_post(&sem_turno);
		//sleep(1);
		pthread_mutex_unlock(&dibujo);
	}
	nivel_gui_terminar();
}

