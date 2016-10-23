/*
 * coordenadas.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "coordenadas.h";

int coor_equals(t_coor coorOne, t_coor coorTwo){
	return coorOne.x == coorTwo.x && coorOne.y == coorTwo.y;
}
