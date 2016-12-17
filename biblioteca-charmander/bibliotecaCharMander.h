/*
 * bibliotecaCharmander.h
 *
 *  Created on: 29/8/2016
 *      Author: utnso
 */

#ifndef BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_
#define BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include "coordenadas.h"

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



int socket_servidor(char* ip, char* puerto, t_log* log);
int conectar(char* socket_servidor, char* puerto_servidor, t_log* log);
int aceptar_conexion(int socket, t_log* log);
void meterStringEnEstructura(char** stringTo, char* stringFrom);
int comparar_strings(char* string1, char* string2); // retorna 1 si string1 es menor que string2; si es al revez retorna 0

#endif /* BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_ */
