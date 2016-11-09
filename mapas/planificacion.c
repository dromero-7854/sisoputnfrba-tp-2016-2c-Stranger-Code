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

#define MENSAJE_BIENVENIDA "Bienvenido al mapa"
#define QUANTUM 3


void handshake(int socketCliente, t_log* logger){

	uint32_t cantLetras;
	uint8_t operation_code;
	void *buffer;

	recv(socketCliente, &operation_code, sizeof(operation_code), 0);
	if(operation_code != OC_UBICAR_ENTRENADOR){
		log_error(logger, "codigo de operacion incorrecto en handshake");
		exit(1);
	}


	t_coor* coordenadas = malloc(sizeof(t_coor));
	coordenadas->x = 1;
	coordenadas->y = 1;
	operation_code = OC_UBICACION_ENTRENADOR;
	memcpy(buffer, &operation_code, sizeof(uint8_t));
	memcpy(buffer + sizeof(uint8_t), coordenadas, sizeof(t_coor));
	send(socketCliente, buffer, sizeof(t_coor), 0);


	printf("FUNCIONOO\n");
	//printf("%s", buffer);
	free(buffer);
	free(coordenadas);
}

void planificar(){




	//int q, quantum;

	while(1){
		if(strcmp(conf_metadata->algoritmo, "RR")){
			ejecutarRafagaRR();
		} else {
			ejecutarRafagaSRDF();
		}
	}
}

t_entrenador* atender(t_queue* cola){
	int q, capturo_pokemon;
	quantum = 0;
	t_entrenador* entrenador;
	for(q = 0; q < QUANTUM; q++){ // QUANTUM lo va a leer de config

		entrenador = queue_pop(colaListos);
		capturo_pokemon = atenderSolicitud(entrenador);
		quantum ++;
		if(capturo_pokemon) break;
	}
	return entrenador;
}

void ejecutarRafagaSRDF(){
	t_entrenador* entrenador;
	atenderEntrenadoresSinDistanciaDefinida();

	entrenador = buscarEntrenadorConMenorDistancia();
	atenderSolicitud(entrenador);
	queue_push(colaListos, entrenador);

}

void ejecutarRafagaRR(){
	quantum = 0;
	t_entrenador* entrenador;
	if(!queue_is_empty(colaListos)){
		entrenador = atender(colaListos);
	} else {
		entrenador = atender(colaBloqueados);
	}
	if (quantum == QUANTUM){
		queue_push(colaListos, entrenador);
	} else {
		queue_push(colaBloqueados, entrenador);
	}
}

void atenderEntrenadoresSinDistanciaDefinida(){
	t_entrenador* entrenador;
	t_queue* colaAux;
	while(entrenador = queue_pop(colaListos)){
		if(entrenador->objetivoActual == NULL){
			atenderSolicitud(entrenador);
			queue_push(colaAux, entrenador);
		} else {
			queue_push(colaAux, entrenador);
		}
	}
	colaListos = colaAux;
}

t_entrenador* buscarEntrenadorConMenorDistancia(){
	t_entrenador* entrenador;
	t_list* listaAux;
	while(entrenador = queue_pop(colaListos)){
		list_add(listaAux, entrenador);
	}
	int _menor_distancia_a_pokenest(t_entrenador* entrenador1, t_entrenador* entrenador2){
		int dist1, dist2;
		PokeNest *pokenest1, *pokenest2;
		pokenest1 = buscarPokenest(listaPokenests, entrenador1->objetivoActual);
		pokenest2 = buscarPokenest(listaPokenests, entrenador2->objetivoActual);
		dist1 = abs(pokenest1->posx - entrenador1->posx) + abs(pokenest1->posy - entrenador1->posy);
		dist2 = abs(pokenest2->posx - entrenador2->posx) + abs(pokenest2->posy - entrenador2->posy);
		return dist1 < dist2;
	}
	list_sort(listaAux, (void*) _menor_distancia_a_pokenest);
	entrenador = list_get(listaAux, 0);
	list_remove(listaAux, 0);
	return entrenador;
}
