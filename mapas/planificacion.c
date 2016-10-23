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
