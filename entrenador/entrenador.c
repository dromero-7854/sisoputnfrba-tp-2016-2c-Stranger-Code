/*
 * entrenador.c
 *
 *  Created on: 20/9/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <bibliotecaCharMander.c>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>


int main(){
	char* nombre, ptoMontaje;
	char** ciudades;
	int id, cantVidas, socketServidor;

	t_log* logger= log_create("entrenador.log", "Entrenador",false, LOG_LEVEL_TRACE);

	socketServidor = conectar("127.0.0.1", "33000", logger);

	uint32_t cantLetras;

	recv(socketServidor, &cantLetras, sizeof(uint32_t), 0);
	char* msg = malloc(cantLetras);

	recv(socketServidor, msg, cantLetras, 0);


	if(strcmp(msg, "Bienvenido al mapa")){
		log_error(logger, "Error en el handshake");
		exit(1);
	}
	printf("Recibio bien\n");
	printf("%s", msg);
	char* palabra = malloc(2);
	strcpy(palabra, "OK");
	send(socketServidor, palabra, 3, 0);
	log_destroy(logger);


	return 0;
}
