/*
 * solicitudes.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "nivel-test.h"
#include "solicitudes.h"
#include <stdint.h>

void atenderSolicitud(t_entrenador* entrenador){
	int recibidos;
	uint32_t header, eje;

	recibidos = recv(entrenador->fd, &header, sizeof(uint32_t), 0);

	switch(header){
	case SOLICITA_UBICACION_POKENEST:
		recv(entrenador->fd, &header, sizeof(uint32_t), 0);
		void *buffer = malloc(header);
		recibidos = recv(entrenador->fd, buffer, header, 0);
		// buscar ubicacion de <<buffer>>
		break;
	case NOTIFICA_MOVIMIENTO:
		recv(entrenador->fd, &eje, sizeof(uint32_t), 0);
		if (eje == 0){
			entrenador->posx++;
		} else {
			entrenador->posy++;
		}
		break;
		// volver a dibujar ??????
	case CAPTURA_POKEMON:
		// bla bla bla
		break;
	}
}
