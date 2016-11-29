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

int atender(t_entrenador* entrenador){
	int q, capturo_pokemon;
	for(q = 0; q < conf_metadata->quantum; q++){
		log_trace(log_mapa, "quantum del entrenador: %d", q);
		sleep(conf_metadata->retardo);
		capturo_pokemon = atenderSolicitud(entrenador);


		//if(capturo_pokemon == TERMINO_MAPA) break;

	}
	return capturo_pokemon;
}

void ejecutarRafagaSRDF(){
	t_entrenador* entrenador;
	int capturo_pokemon;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> objetivoActual == NULL);
	}
	//if((entrenador = list_find(colaListos->elements, (void*) _no_tiene_objetivo_asignado)) != NULL){
	if ((entrenador = buscarEntrenadorSinDistanciaDefinida()) != NULL){
		sleep(conf_metadata->retardo);
		capturo_pokemon = atenderSolicitud(entrenador);
		if(capturo_pokemon){
			liberarEntrenador(entrenador);
			return;
		}
		queue_push(colaListos, entrenador);
		return;
	}
	//}
	if(!queue_is_empty(colaListos)){
		//atenderEntrenadoresSinDistanciaDefinida();

		entrenador = buscarEntrenadorConMenorDistancia();
		while(1){
			sleep(conf_metadata->retardo);
			capturo_pokemon = atenderSolicitud(entrenador);
			if(capturo_pokemon){
				if(capturo_pokemon == 1){
					break;
				} else {
					liberarEntrenador(entrenador);
					return;
				}
			}
		}
		queue_push(colaListos, entrenador);
	}
}

void ejecutarRafagaRR(){
	int respuesta, q, capturo_pokemon;
	t_entrenador* entrenador;
	if(!queue_is_empty(colaListos)){
		log_trace(log_mapa, "atendiendo cola Listos");
		entrenador = queue_pop(colaListos);
		for(q = 0; q < conf_metadata->quantum; q++){
			log_trace(log_mapa, "quantum del entrenador: %d", q);
			sleep(conf_metadata->retardo);
			respuesta = atenderSolicitud(entrenador);


			if(respuesta != TURNO_NORMAL) break;

		}
		//respuesta = atender(entrenador);
		switch(respuesta){
		case DESCONEXION:
			liberarEntrenador(entrenador);
			break;
		case NO_ENCONTRO_POKEMON:
			queue_push(colaBloqueados, entrenador);
			break;
		default:
			queue_push(colaListos, entrenador);
			break;
		}
		//queue_push(colaListos, entrenador);
	}
}

t_entrenador* buscarEntrenadorSinDistanciaDefinida(){
	t_entrenador* entrenador;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> objetivoActual == NULL);
	}
	entrenador = list_remove_by_condition(colaListos->elements, (void*) _no_tiene_objetivo_asignado);
	return entrenador;
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
