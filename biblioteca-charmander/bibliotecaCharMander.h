/*
 * bibliotecaCharmander.h
 *
 *  Created on: 29/8/2016
 *      Author: utnso
 */

#ifndef BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_
#define BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_

int socket_servidor(char* puerto, t_log* log);
int conectar(char* socket_servidor, char* puerto_servidor, t_log* log);
int aceptar_conexion(int socket, t_log* log);
void meterStringEnEstructura(char** stringTo, char* stringFrom);

#endif /* BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_ */
