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
#include <commons/string.h>
#include "../Coordenadas/coordenadas.h"
#include "../Mapa/mapa.h"
#include "../Conexion/conexion.h"

#define SOLICITAR_UBICACION 0;
#define SOLICITAR_AVANZAR 1;
#define SOLICITAR_ATRAPAR 2;

typedef struct {
	char* name ;
	char *simbol;
	int life;
	t_coor* coor;
	t_list* pokemons;
	t_list* travel_sheet;
	int index_current_map;
	t_connection* conn;
} t_coach;

t_coach *coach_create(char* nombreEntrenador, char* simbol, int life);
void coach_destroy(t_coach* self);
t_map* coach_next_map(t_coach* self);
int coach_move_to_pokemon(t_coach* self, t_pokemon* pokemon);
int coach_capture_pokemon(t_coach* self, t_pokemon* pokemon);
void coach_connect_to_map(t_coach* self, t_map* mapa);
void coach_medal_copy(t_coach* self, t_map* mapa);

#endif /* SRC_ENTRENADOR_ENTRENADOR_H_ */
