/*
 * planificacion.c
 *
 *  Created on: 24/9/2016
 *      Author: utnso
 */
#include <stdint.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include "planificacion.h"
#include "solicitudes.h"

void handshake(int socketCliente, t_log* logger);
void planificar(void);

#define MENSAJE_BIENVENIDA "Bienvenido al mapa"
#define QUANTUM 3


void handshake(int socketCliente, t_log* logger){

	uint32_t cantLetras;



	cantLetras = strlen(MENSAJE_BIENVENIDA);
	char* msg_bienvenida = malloc(cantLetras + 1);
	strcpy(msg_bienvenida, MENSAJE_BIENVENIDA);


	void *buffer= malloc(cantLetras + 1 + sizeof(uint32_t));

	memcpy(buffer, &cantLetras, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), msg_bienvenida, cantLetras + 1);
	free(msg_bienvenida);

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

void planificar(){


	colaListos = queue_create();
	colaBloqueados = queue_create();

	int q, quantum;
	t_entrenador* entrenador;

	while(1){
		quantum = 0;
		verificarBloqueados();
		for(q = 0; q < QUANTUM; q++){ // QUANTUM lo va a leer de config

			entrenador = queue_pop(colaListos);
			atenderSolicitud(entrenador);
			quantum ++;
		}
		if (quantum == QUANTUM){
			queue_push(colaListos, entrenador);
		} else {
			queue_push(colaBloqueados, entrenador);
		}
	}
}
void verificarBloqueados(){
	//hacer
}
