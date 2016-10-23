/*
 * coordenadas.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_COORDENADAS_COORDENADAS_H_
#define SRC_COORDENADAS_COORDENADAS_H_

#define MOVE_UP 72
#define MOVE_RIGHT 77
#define MOVE_DOWN 80
#define MOVE_LEFT 75

typedef struct {
	int x;
	int y;
} t_coor;

int coor_equals(t_coor coorOne, t_coor coorTwo);

#endif /* SRC_COORDENADAS_COORDENADAS_H_ */
