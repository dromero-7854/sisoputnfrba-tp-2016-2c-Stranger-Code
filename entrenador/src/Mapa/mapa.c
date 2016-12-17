/*
 * mapa.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "mapa.h"

t_map *map_create(char *name, char *ip, char *port){
	t_map *new = malloc( sizeof(t_map) );
	new->name = strdup(name);
	new->ip = strdup(ip);
	new->port = strdup(port);
	new->pokemon_list = list_create();
	new->index_current_pokemon = -1;
	return new;
}

void map_destroy(t_map *self){
	free(self->name);
	free(self->ip);
	free(self->port);
	//if(!list_is_empty(self->pokemon_list)) list_clean(self->pokemon_list);
	destroy_pokemon_list(self->pokemon_list);
	//free(self->pokemon_list);
	free(self);
}

t_map* find_map_by_name(t_list *maps_list, char *name){
	int _is_the_one(t_map *m) {
		return string_equals_ignore_case(m->name, name);
	}

	return list_find(maps_list, (void*) _is_the_one);
}

void destroy_maps_list(t_list* maps_list){
	//log_info(logger, "Liberando memoria...");

	/*int lenght = list_size(maps_list);
	while( lenght > 0){
		//log_info(logger, "\ttamaño de maps_list: %d", lenght);
		//conectar_mapa( list_get(maps_list, pos) );
		t_map* mapa = list_remove(maps_list, lenght-1);
		destroy_pokemon_list(mapa->pokemon_list);
		map_destroy(mapa);
		lenght = list_size(maps_list);
	}*/

	//log_info(logger, "Memoria liberada.\n");
	list_destroy_and_destroy_elements(maps_list, (void*) map_destroy);
}

t_pokemon* map_next_pokemon(t_map* self){
	if(list_size(self->pokemon_list) > self->index_current_pokemon+1){
		self->index_current_pokemon++;
		t_pokemon* current_pokemon = list_get(self->pokemon_list, self->index_current_pokemon);
		return current_pokemon;
	}else{
		return NULL;
	}
}

bool map_is_last_pokemon(t_map* self){
	return (list_size(self->pokemon_list) == self->index_current_pokemon+1);
}

int map_locate_pokemon(t_map *mapa, t_pokemon* pokemon, t_connection* conn){
	t_coor* coor;
	uint8_t operation_code;
	connection_send(conn, OC_UBICAR_POKENEST, pokemon->simbol);
	connection_recv(conn, &operation_code, (void**)&coor);
	//setear las coordenadas de dicho pokemon, recibidas del mapa
	pokemon->coor->x = coor->x;
	pokemon->coor->y = coor->y;

	return 0;
}
