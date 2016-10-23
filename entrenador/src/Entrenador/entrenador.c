/*
 * entrenador.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "entrenador.h"

t_coach *coach_create(char *name, int life){
	t_coach *new = malloc( sizeof(t_coach) );
	new->name = strdup(name);
	new->life = life;
	//new->coor;
	new->pokemons = list_create();
	new->travel_sheet = list_create();
	new->index_current_map = -1;
	return new;
}

void coach_destroy(t_coach *self){
	free(self->name);
	if(!list_is_empty(self->pokemons)) list_clean(self->pokemons);
	free(self->pokemons);
	destroy_maps_list(self->travel_sheet);
	/*if(!list_is_empty(self->travel_sheet)) list_clean(self->travel_sheet);
	free(self->travel_sheet);*/

	free(self);
}

t_map* coach_next_map(t_coach* self){
	if(list_size(self->travel_sheet) > self->index_current_map){
		self->index_current_map++;
		t_map* current_map = list_get(self->travel_sheet, self->index_current_map);
		return current_map;
	}else{
		return NULL;
	}
}
