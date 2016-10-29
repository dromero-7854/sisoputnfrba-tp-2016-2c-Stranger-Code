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

typedef struct {
	char* ip;
	char* port;
	int* socket;
} t_connection;

t_connection* connection_create(char* ip, char* port);
void connection_destroy(t_connection* self);

void connection_open(t_connection* connection);
void connection_close(t_connection* connection);
int connection_send(t_connection* connection, int operation_code, char* message);
void connection_recv(t_connection* connection, int* operation_code, char** message);

#endif /* SRC_CONEXION_CONEXION_H_ */
