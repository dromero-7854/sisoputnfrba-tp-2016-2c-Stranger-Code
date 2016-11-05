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
#include <netdb.h>
#include <stddef.h>

#define OC_MENSAJE 0
#define OC_UBICAR_POKENEST 1
#define OC_UBICAR_ENTRENADOR 2
#define OC_AVANZAR_POSICION 3
#define OC_ATRAPAR_POKEMON 4
#define OC_UBICACION_POKENEST 5
#define OC_UBICACION_ENTRENADOR 6

#define MOVE_UP 72
#define MOVE_RIGHT 77
#define MOVE_DOWN 80
#define MOVE_LEFT 75

/** provisorio **/
typedef struct {
	uint8_t x;
	uint8_t y;
} t_coor;
/****************/

typedef struct {
	char* ip;
	char* port;
	int* socket;
} t_connection;

t_connection* connection_create(char* ip, char* port);
void connection_destroy(t_connection* self);

void connection_open(t_connection* connection);
void connection_close(t_connection* connection);
int connection_send(t_connection* connection, uint8_t operation_code, void* message);
int connection_recv(t_connection* connection, uint8_t* operation_code, void** message);

#endif /* SRC_CONEXION_CONEXION_H_ */
