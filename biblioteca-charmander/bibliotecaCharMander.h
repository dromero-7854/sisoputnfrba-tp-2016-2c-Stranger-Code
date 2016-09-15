/*
 * bibliotecaCharmander.h
 *
 *  Created on: 29/8/2016
 *      Author: utnso
 */

#ifndef BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_
#define BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_

int socket_servidor();
void conectar(char* socket_servidor, char* puerto_servidor);
int aceptar_conexion(int socket);
void manejar_select(int socket);
#endif /* BIBLIOTECA_CHARMANDER_BIBLIOTECACHARMANDER_H_ */
