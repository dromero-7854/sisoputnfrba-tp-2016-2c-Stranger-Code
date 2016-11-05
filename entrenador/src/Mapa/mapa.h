/*
 * mapa.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_MAPA_MAPA_H_
#define SRC_MAPA_MAPA_H_

#include <stddef.h>
#include <commons/collections/list.h>
#include "../Pokemon/pokemon.h"
#include "../Conexion/conexion.h"

typedef struct {
	char *name;
	char *ip;
	char *port;
	t_list *pokemon_list;
	int index_current_pokemon;
} t_map;

t_map* map_create(char *name, char *ip, char *port);
void map_destroy(t_map *self);
void destroy_maps_list(t_list* maps_list);
t_map* find_map_by_name(t_list *maps_list, char *name);
t_pokemon* map_next_pokemon(t_map* self);
int map_locate_pokemon(t_map *mapa, t_pokemon* pokemon, t_connection* conn);

#endif /* SRC_MAPA_MAPA_H_ */
