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
	return buscarEntrenador(entrenador->id, colaListos->elements);
}

void liberarRecursos(t_entrenador* entrenador){
	int index_pokemon, index_entrenador;
	t_infoPokemon* infoPokemon;
	t_entrenador* entrenadorBloqueado;
	for(index_pokemon = 0 ; index_pokemon < list_size(entrenador->pokemons); index_pokemon ++ ){
		infoPokemon = list_get(entrenador->pokemons, index_pokemon);

		for(index_entrenador = 0; index_entrenador < list_size(colaBloqueados->elements); index_entrenador++){
			entrenadorBloqueado = list_get(colaBloqueados, index_entrenador);
			if(entrenadorBloqueado->objetivoActual == infoPokemon->id_pokenest){
				list_add(entrenadorBloqueado->pokemons, infoPokemon);
				//entrenadorBloqueado->objetivoActual = NULL;
				queue_push(colaListos, buscarEntrenador(entrenadorBloqueado->id, colaBloqueados->elements));
				/**
				 * VER QUE PASA CON EL ENTRENADOR SIGUIENTE EN LA COLA RECORRIDA
				 */
				break;
			}
		}
	}
	list_destroy(entrenador->pokemons);
	liberarEntrenador(entrenador);
}

t_entrenador* buscarEntrenador(int socket, t_list* lista){
	int _mismo_id(t_entrenador* e){
		return (e->id == socket);
	}
	return list_remove_by_condition(lista, _mismo_id);
}
