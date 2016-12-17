/*
 * conexion.h
 *
 *  Created on: 24/10/2016
 *      Author: utnso
 */

#ifndef SRC_CONEXION_CONEXION_H_
#define SRC_CONEXION_CONEXION_H_

#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>
#include <stddef.h>
#include <commons/string.h>
#include "../Coordenadas/coordenadas.h"

#define OC_MENSAJE 0
#define OC_UBICAR_POKENEST 1
#define OC_UBICAR_ENTRENADOR 2
#define OC_AVANZAR_POSICION 3
#define OC_ATRAPAR_POKEMON 4
#define OC_UBICACION_POKENEST 5
#define OC_UBICACION_ENTRENADOR 6
#define OC_OBTENER_MEDALLA 7
#define OC_MEDALLA 8
#define OC_OBTENER_OBJETIVOS 9
#define OC_VICTIMA_DEADLOCK 10
#define OC_ATRAPAR_ULTIMO_POKEMON 11
#define OC_OBTENER_CANTIDAD_DEADLOCK 12
#define OC_CANTIDAD_DEADLOCK 13
#define OC_POKEMON 14
#define OC_PERDIO_BATALLA 15
#define OC_GANO_BATALLA 16
#define OC_POKEMON_BATALLA 17

#define MOVE_UP 72
#define MOVE_RIGHT 77
#define MOVE_DOWN 80
#define MOVE_LEFT 75

/** provisorio **/
/*typedef struct {
	int x;
	int y;
} t_coor;*/
/****************/

typedef struct {
	char* ip;
	char* port;
	int* socket;
} t_connection;

t_connection* connection_create(char* ip, char* port);
void connection_destroy(t_connection* self);

int connection_open(t_connection* connection);
void connection_close(t_connection* connection);
int connection_send(t_connection* connection, uint8_t operation_code, void* message);
int connection_recv(t_connection* connection, uint8_t* operation_code, void** message);

#endif /* SRC_CONEXION_CONEXION_H_ */
