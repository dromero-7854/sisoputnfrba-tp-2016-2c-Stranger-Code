/*
 * planificacion.c
 *
 *  Created on: 24/9/2016
 *      Author: utnso
 */
#include <stdint.h>
#include <commons/log.h>
#include "planificacion.h"

void handshake(int socketCliente, t_log* logger);

#define MENSAJE_BIENVENIDA "Bienvenido al mapa"

void handshake(int socketCliente, t_log* logger){

	uint32_t cantLetras;



	cantLetras = strlen(MENSAJE_BIENVENIDA);
	char* msg_bienvenida = malloc(cantLetras + 1);
	strcpy(msg_bienvenida, MENSAJE_BIENVENIDA);


	void *buffer= malloc(cantLetras + 1 + sizeof(uint32_t));

	memcpy(buffer, &cantLetras, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), msg_bienvenida, cantLetras + 1);

	send(socketCliente, buffer, cantLetras + 1 + sizeof(uint32_t), 0);

	recv(socketCliente, buffer, 3, 0);
	if(strcmp(buffer, "OK")){
		log_error(logger, "Error en el handshake");
		exit(1);
	}
	printf("FUNCIONOO\n");
	printf("%s", buffer);
	free(buffer);
}
