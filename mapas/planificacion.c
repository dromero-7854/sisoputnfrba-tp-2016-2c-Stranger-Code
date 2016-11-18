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
		if(!strcmp(conf_metadata->algoritmo, "RR")){
			//log_trace(log_mapa, "Planificacion RR");
			ejecutarRafagaRR();
		} else {
			//log_trace(log_mapa, "Planificacion SRDF");
			ejecutarRafagaSRDF();
		}
	}
}

t_entrenador* atender(t_queue* cola){
	int q, capturo_pokemon;
	quantum = 0;
	t_entrenador* entrenador;
	for(q = 0; q < QUANTUM; q++){ // QUANTUM lo va a leer de config
		pthread_mutex_lock(&mutex_cola_listos);
		entrenador = queue_pop(colaListos);
		capturo_pokemon = atenderSolicitud(entrenador);
		quantum ++;
		if(capturo_pokemon) break;
	}
	return entrenador;
}

void ejecutarRafagaSRDF(){
	t_entrenador* entrenador;
	if(!queue_is_empty(colaListos)){
		atenderEntrenadoresSinDistanciaDefinida();

		entrenador = buscarEntrenadorConMenorDistancia();
		atenderSolicitud(entrenador);
		queue_push(colaListos, entrenador);
	}
}

void ejecutarRafagaRR(){
	quantum = 0;
	t_entrenador* entrenador;
	if(!queue_is_empty(colaListos)){
		entrenador = atender(colaListos);
	} else if(!queue_is_empty(colaBloqueados)) {
		entrenador = atender(colaBloqueados);
	} else {
		return;
	}
	if (quantum == QUANTUM){
		queue_push(colaListos, entrenador);
		pthread_mutex_unlock(&mutex_cola_listos);
	} else {
		queue_push(colaBloqueados, entrenador);
	}
}

void atenderEntrenadoresSinDistanciaDefinida(){
	t_entrenador* entrenador;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> objetivoActual == NULL);
	}
	while((entrenador = list_remove_by_condition(colaListos->elements, (void *)_no_tiene_objetivo_asignado)) != NULL){
			atenderSolicitud(entrenador);
			queue_push(colaListos, entrenador);
	}
}

t_entrenador* buscarEntrenadorConMenorDistancia(){
	t_entrenador* entrenador;
	t_list* listaAux = list_create();
	list_add_all(listaAux, colaListos->elements);
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
	int _mismo_id(t_entrenador* e){
		return (e->id == entrenador->id);
	}
	return list_remove_by_condition(colaListos->elements, _mismo_id);
}
