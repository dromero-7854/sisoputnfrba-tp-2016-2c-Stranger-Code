/*
 * coordenadas.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "coordenadas.h"

t_coor *coor_create(uint8_t* coorX, uint8_t* coorY){
	t_coor *new = malloc( sizeof(t_coor) );
	new->x = coorX;
	new->y = coorY;

	return new;
}

void coor_destroy(t_coor *self){
	free(self->x);
	free(self->y);
	free(self);
}

int coor_equals(t_coor* coorOne, t_coor* coorTwo){
	return coorOne->x == coorTwo->x && coorOne->y == coorTwo->y;
}
