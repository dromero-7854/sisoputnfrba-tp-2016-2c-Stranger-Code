/*
 * pokemon.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#include "pokemon.h"

t_pokemon *pokemon_create(char *name, char *simbol){
	t_pokemon *new = malloc( sizeof(t_pokemon) );
	new->name = strdup(name);
	new->simbol = strdup(simbol);
	new->coor = coor_create(0, 0);
	return new;
}

void pokemon_destroy(t_pokemon *self){
	free(self->name);
	free(self->simbol);
	free(self->coor);
	free(self);
}

void destroy_pokemon_list(t_list *poke_list){
	/*int lenght = list_size(poke_list);
	while( lenght > 0){
		//log_info(logger, "\t\ttamaño de poke_list: %d", lenght);
		t_pokemon* poke = list_remove(poke_list, lenght-1);
		pokemon_destroy(poke);
		lenght = list_size(poke_list);
	}*/

	list_destroy_and_destroy_elements(poke_list, (void*) pokemon_destroy);

}
