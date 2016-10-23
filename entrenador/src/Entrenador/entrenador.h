/*
 * entrenador.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_ENTRENADOR_ENTRENADOR_H_
#define SRC_ENTRENADOR_ENTRENADOR_H_

#include <stddef.h>
#include <commons/collections/list.h>
#include "../Coordenadas/coordenadas.h"
#include "../Mapa/mapa.h"

#define SOLICITAR_UBICACION 0;
#define SOLICITAR_AVANZAR 1;
#define SOLICITAR_ATRAPAR 2;

typedef struct {
	char* name ;
	int life;
	t_coor coor;
	t_list* pokemons;
	t_list* travel_sheet;
	int index_current_map;
} t_coach;

t_coach *coach_create(char* nombreEntrenador, int life);
void coach_destroy(t_coach* self);
t_map* coach_next_map(t_coach* self);

#endif /* SRC_ENTRENADOR_ENTRENADOR_H_ */
