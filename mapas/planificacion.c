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




void planificar(){

	FD_ZERO(&set_bloq_master);
	FD_ZERO(&set_bloqueados);

	while(1){
		if(!strcmp(conf_metadata->algoritmo, "RR")){
			//log_trace(log_mapa, "Planificacion RR");
			ejecutarRafagaRR();
		} else {
			//log_trace(log_mapa, "Planificacion SRDF");
			ejecutarRafagaSRDF();
		}
		/*set_bloqueados = set_bloq_master;
		select(fd_bloq_max, &set_bloqueados, NULL, NULL, 0);*/
	}
}

int atender(t_entrenador* entrenador){
	int q, capturo_pokemon;
	for(q = 0; q < conf_metadata->quantum; q++){
		log_trace(log_mapa, "quantum del entrenador: %d", q);
		nanosleep(&tim, NULL);
		capturo_pokemon = atenderSolicitud(entrenador);


		//if(capturo_pokemon == TERMINO_MAPA) break;

	}
	return capturo_pokemon;
}

void ejecutarRafagaSRDF(){
	t_entrenador* entrenador;
	int respuesta;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> pokenest_buscada == NULL);
	}
	//if((entrenador = list_find(colaListos->elements, (void*) _no_tiene_objetivo_asignado)) != NULL){
	if ((entrenador = buscarEntrenadorSinDistanciaDefinida()) != NULL){
		nanosleep(&tim, NULL);
		respuesta = atenderSolicitud(entrenador);
		if(respuesta == DESCONEXION){
			liberarRecursos2(entrenador);
			sem_post(&sem_dibujo);
			return;
		}
		if(respuesta == NO_ENCONTRO_POKEMON){
			bloquearEntrenador(entrenador);
			return;
		}
		pthread_mutex_lock(&mutex_cola_listos);
		queue_push(colaListos, entrenador);
		pthread_mutex_unlock(&mutex_cola_listos);
		return;
	}else if(!queue_is_empty(colaListos)){
		//atenderEntrenadoresSinDistanciaDefinida();

		entrenador = buscarEntrenadorConMenorDistancia();
		do {
			//pthread_mutex_lock(&dibujo);
			nanosleep(&tim, NULL);
			respuesta = atenderSolicitud(entrenador);
			sem_post(&sem_dibujo);

			//pthread_mutex_unlock(&dibujo);
		} while(respuesta == TURNO_NORMAL);

		switch(respuesta){
		//case TURNO_NORMAL:
		//	continue;
		case NO_ENCONTRO_POKEMON:
			bloquearEntrenador(entrenador);
			break;
		//	continue;
		case DESCONEXION:
			liberarRecursos2(entrenador);
			sem_post(&sem_dibujo);
			break;
		case CAPTURO_POKEMON:
			pthread_mutex_lock(&mutex_cola_listos);
			queue_push(colaListos, entrenador);
			pthread_mutex_unlock(&mutex_cola_listos);
			break;
		default:
			log_error(log_mapa, "Hubo un problema luego de atender solicitud. Respuesta no correcta");
			break;
		}
		/*
		pthread_mutex_lock(&mutex_cola_listos);
		queue_push(colaListos, entrenador);
		pthread_mutex_unlock(&mutex_cola_listos);*/
	}
	//sem_post(&sem_dibujo);
}

void ejecutarRafagaRR(){
	int respuesta, q, capturo_pokemon;
	t_entrenador* entrenador;
	//sem_wait(&sem_turno);
	if(!queue_is_empty(colaListos)){
		log_trace(log_mapa, "atendiendo cola Listos");
		pthread_mutex_lock(&mutex_cola_listos);
		entrenador = queue_pop(colaListos);
		pthread_mutex_unlock(&mutex_cola_listos);
		for(q = 0; q < conf_metadata->quantum; q++){
			log_trace(log_mapa, "quantum del entrenador: %d", q);
			//pthread_mutex_lock(&dibujo);
			nanosleep(&tim, NULL);
			respuesta = atenderSolicitud(entrenador);
			//pthread_mutex_unlock(&dibujo);
			sem_post(&sem_dibujo);
			if(respuesta != TURNO_NORMAL) break;

		}
		//respuesta = atender(entrenador);
		switch(respuesta){
		case DESCONEXION:
			liberarRecursos2(entrenador);
			sem_post(&sem_dibujo);
			break;
		case NO_ENCONTRO_POKEMON:
			//queue_push(colaBloqueados, entrenador);
			bloquearEntrenador(entrenador);
			break;
		default:
			pthread_mutex_lock(&mutex_cola_listos);
			queue_push(colaListos, entrenador);
			pthread_mutex_unlock(&mutex_cola_listos);
			break;
		}

		//queue_push(colaListos, entrenador);
	}
}

t_entrenador* buscarEntrenadorSinDistanciaDefinida(){
	t_entrenador* entrenador;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> pokenest_buscada == NULL);
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
		pokenest1 = buscarPokenest(listaPokenests, entrenador1->pokenest_buscada);
		pokenest2 = buscarPokenest(listaPokenests, entrenador2->pokenest_buscada);
		dist1 = abs(pokenest1->posx - entrenador1->posx) + abs(pokenest1->posy - entrenador1->posy);
		dist2 = abs(pokenest2->posx - entrenador2->posx) + abs(pokenest2->posy - entrenador2->posy);
		return dist1 < dist2;
	}

	list_sort(listaAux, (void*) _menor_distancia_a_pokenest);
	entrenador = list_get(listaAux, 0);
	return buscarEntrenador(entrenador->id, colaListos->elements);
}

void liberarRecursos(t_entrenador* entrenador){
	int index_entrenador, index_pokenest, nadieLoQuiso;
	t_infoPokemon* infoPokemon;
	t_entrenador* entrenadorBloqueado = malloc(sizeof(t_entrenador));

	while(!list_is_empty(entrenador->pokemons)) {
		infoPokemon = list_remove(entrenador->pokemons, 0);
		nadieLoQuiso = 1;

		pthread_mutex_lock(&mutex_cola_listos);
		index_entrenador =0;
		//while(index_entrenador < queue_size(colaBloqueados))

		int _buscaEsePokemon(t_entrenador * entr) {
			return infoPokemon->id_pokenest == entr->pokenest_buscada;
		}
		entrenadorBloqueado = list_find(colaBloqueados->elements, (void *) _buscaEsePokemon);

		list_add(entrenador->pokemons, infoPokemon);
		notificar_captura_pokemon(infoPokemon, entrenadorBloqueado);
		queue_push(colaListos, buscarEntrenador(entrenadorBloqueado->id, colaBloqueados->elements));

		/*for(index_entrenador = 0; index_entrenador < queue_size(colaBloqueados); index_entrenador++){
			entrenadorBloqueado = list_get(colaBloqueados->elements, index_entrenador);
			if(entrenadorBloqueado->pokenest_buscada == infoPokemon->id_pokenest){
				//list_add(entrenadorBloqueado->pokemons, infoPokemon);
				nadieLoQuiso = 0;
				//entrenadorBloqueado->objetivoActual = NULL;
				queue_push(colaListos, buscarEntrenador(entrenadorBloqueado->id, colaBloqueados->elements));

				break;
			}
		}*/

		pthread_mutex_unlock(&mutex_cola_listos);
		int _mismo_id(PokeNest * pokenest) {
			return (infoPokemon->id_pokenest == pokenest->id);
		}
		PokeNest * pokenest = list_find(listaPokenests, (void *) _mismo_id);

		list_add(pokenest->listaPokemons, infoPokemon);
		pokenest->cantidad++;


		/*if(nadieLoQuiso) {
			for(index_pokenest = 0; index_pokenest < list_size(listaPokenests); index_pokenest++) {

				PokeNest *pokenest = list_get(listaPokenests, index_pokenest);
				if(infoPokemon->id_pokenest == pokenest->id) {
					list_add(pokenest->listaPokemons, infoPokemon);
					pokenest->cantidad++;
					break;
				}
			}
		}*/
	}
	int i;
	pthread_mutex_lock(&mutex_cola_listos);
	for(i = 0; i < queue_size(colaBloqueados); i++) {
		t_entrenador * entrEnCola = list_get(colaBloqueados->elements, i);
		if(entrEnCola->simbolo == entrenador->simbolo) {
			list_remove(colaBloqueados->elements, i);
			break;
		}
	}
	if(i == queue_size(colaBloqueados)) {
		for(i = 0; i < queue_size(colaListos); i++) {
			t_entrenador * entrEnCola = list_get(colaListos->elements, i);
			if(entrEnCola->simbolo == entrenador->simbolo){
				pthread_mutex_lock(&mutex_cola_listos);
				list_remove(colaListos->elements, i);
				pthread_mutex_unlock(&mutex_cola_listos);
				break;
			}
		}
	}
	pthread_mutex_unlock(&mutex_cola_listos);

	list_destroy(entrenador->pokemons);
	liberarEntrenador(entrenador);
}

void liberarRecursos2(t_entrenador* entrenadorLiberado){
	int index_entrenador, pokemon_asignado = 0;
	t_infoPokemon* infoPokemon;
	t_entrenador* entrenadorBloqueado;
	while((infoPokemon = list_remove(entrenadorLiberado->pokemons, 0)) != NULL){
		//infoPokemon = list_get(entrenadorLiberado->pokemons, index_pokemon);


		pthread_mutex_lock(&mutex_turno_desbloqueo);
		pthread_mutex_lock(&mutex_cola_bloqueados);
		for(index_entrenador = 0; index_entrenador < list_size(colaBloqueados->elements); index_entrenador++){
			entrenadorBloqueado = list_get(colaBloqueados->elements, index_entrenador);
			if(entrenadorBloqueado->pokenest_buscada == infoPokemon->id_pokenest){
				list_add(entrenadorBloqueado->pokemons, infoPokemon);
				notificar_captura_pokemon(infoPokemon, entrenadorBloqueado);
				//entrenadorBloqueado->objetivoActual = NULL;
				desbloquearEntrenador(entrenadorBloqueado);
				pokemon_asignado = 1;
				break;
			}
		}
		pthread_mutex_unlock(&mutex_cola_bloqueados);
		pthread_mutex_unlock(&mutex_turno_desbloqueo);
		if(!pokemon_asignado){
			int _mismo_id_pokenest(PokeNest * pokenest) {
				return (infoPokemon->id_pokenest == pokenest->id);
			}
			PokeNest * pokenest = list_find(listaPokenests, (void *) _mismo_id_pokenest);

			list_add(pokenest->listaPokemons, infoPokemon);
			pokenest->cantidad++;
			sumarRecurso(items, infoPokemon->id_pokenest);
		}

	}
	list_destroy(entrenadorLiberado->pokemons);
	liberarEntrenador(entrenadorLiberado);
}

t_entrenador* buscarEntrenador(int socket, t_list* lista){
	int _mismo_id(t_entrenador* e){
		return (e->id == socket);
	}

	t_entrenador * encontrado = list_find(lista, (void*) _mismo_id);
	list_remove_by_condition(lista, (void*)_mismo_id);
	return encontrado;
}

void bloquearEntrenador(t_entrenador* entrenador){
	pthread_mutex_lock(&mutex_cola_bloqueados);
	queue_push(colaBloqueados, entrenador);
	FD_SET(entrenador->id, &master);
	if(entrenador->id > set_fd_max) set_fd_max = entrenador->id;

	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

void desbloquearEntrenador(t_entrenador* entrenador){
	//pthread_mutex_lock(&mutex_cola_bloqueados);
	t_entrenador* trainer = buscarEntrenador(entrenador->id, colaBloqueados->elements);
	queue_push(colaListos, trainer);
	FD_CLR(entrenador->id, &master);
	//pthread_mutex_unlock(&mutex_cola_bloqueados);
}
