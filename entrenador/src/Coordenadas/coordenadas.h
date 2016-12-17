/*
 * coordenadas.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_COORDENADAS_COORDENADAS_H_
#define SRC_COORDENADAS_COORDENADAS_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	uint8_t x;
	uint8_t y;
} t_coor;

t_coor *coor_create(uint8_t* coorX, uint8_t* coorY);
void coor_destroy(t_coor *self);
int coor_equals(t_coor* coorOne, t_coor* coorTwo);

#endif /* SRC_COORDENADAS_COORDENADAS_H_ */
