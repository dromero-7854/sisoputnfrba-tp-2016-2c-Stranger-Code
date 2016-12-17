/*
 * pokemon.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_POKEMON_POKEMON_H_
#define SRC_POKEMON_POKEMON_H_

#include <stdlib.h>
#include "../Coordenadas/coordenadas.h"
#include <commons/collections/list.h>

typedef struct {
	char *name;
	char *simbol;
	int level;
	t_coor *coor;
} t_pokemon;

t_pokemon *pokemon_create(char *name, char *simbol);
void pokemon_destroy(t_pokemon *self);
void destroy_pokemon_list(t_list *poke_list);

#endif /* SRC_POKEMON_POKEMON_H_ */
