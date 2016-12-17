/*
 * planificacion.c
 *
 *  Created on: 24/9/2016
 *      Author: utnso
 */

#include "planificacion.h"
#include "solicitudes.h"




void planificar(){

	FD_ZERO(&set_bloq_master);
	FD_ZERO(&set_bloqueados);

	while(1){
		if(!strcmp(conf_metadata->algoritmo, "RR")){
			ejecutarRafagaRR();
		} else {
			ejecutarRafagaSRDF();
		}
	}
}

int atender(t_entrenador* entrenador){
	int q, capturo_pokemon;
	for(q = 0; q < conf_metadata->quantum; q++){
		log_trace(log_mapa, "quantum del entrenador: %d", q);
		nanosleep(&tim, NULL);
		capturo_pokemon = atenderSolicitud(entrenador);

	}
	return capturo_pokemon;
}

void ejecutarRafagaSRDF(){
	t_entrenador* entrenador;
	int respuesta;
	int _no_tiene_objetivo_asignado(t_entrenador* entrenador){
		return (entrenador-> pokenest_buscada == NULL);
	}
	if ((entrenador = buscarEntrenadorSinDistanciaDefinida()) != NULL){
		usleep(conf_metadata->retardo);
		log_trace(log_mapa, "atendiendo a: %s", entrenador->nombre);
		respuesta = atenderSolicitud(entrenador);
		if(respuesta == DESCONEXION){
			pthread_mutex_lock(&mutex_turno_desbloqueo);
			pthread_mutex_lock(&mutex_cola_bloqueados);
			liberarRecursos2(entrenador);
			pthread_mutex_unlock(&mutex_cola_bloqueados);
			pthread_mutex_unlock(&mutex_turno_desbloqueo);
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

		entrenador = buscarEntrenadorConMenorDistancia();
		do {
			usleep(conf_metadata->retardo);
			log_trace(log_mapa, "atendiendo a: %s", entrenador->nombre);
			respuesta = atenderSolicitud(entrenador);
			sem_post(&sem_dibujo);
		} while(respuesta == TURNO_NORMAL);

		switch(respuesta){
		case NO_ENCONTRO_POKEMON:
			bloquearEntrenador(entrenador);
			break;
		case DESCONEXION:
			pthread_mutex_lock(&mutex_turno_desbloqueo);
			pthread_mutex_lock(&mutex_cola_bloqueados);
			liberarRecursos2(entrenador);
			pthread_mutex_unlock(&mutex_cola_bloqueados);
			pthread_mutex_unlock(&mutex_turno_desbloqueo);
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
	}
}

void ejecutarRafagaRR(){
	int respuesta, q, capturo_pokemon;
	t_entrenador* entrenador;
	if(!queue_is_empty(colaListos)){
		pthread_mutex_lock(&mutex_cola_listos);
		entrenador = queue_pop(colaListos);
		log_trace(log_mapa, "atendiendo a: %s", entrenador->nombre);
		informar_contenido_cola(colaListos);
		pthread_mutex_unlock(&mutex_cola_listos);
		for(q = 0; q < conf_metadata->quantum; q++){
			log_trace(log_mapa, "quantum del entrenador: %d", q);
			usleep(conf_metadata->retardo);
			respuesta = atenderSolicitud(entrenador);
			sem_post(&sem_dibujo);
			if(respuesta != TURNO_NORMAL) break;

		}
		switch(respuesta){
		case DESCONEXION:
			pthread_mutex_lock(&mutex_turno_desbloqueo);
			pthread_mutex_lock(&mutex_cola_bloqueados);
			liberarRecursos2(entrenador);
			pthread_mutex_unlock(&mutex_cola_bloqueados);
			pthread_mutex_unlock(&mutex_turno_desbloqueo);
			sem_post(&sem_dibujo);
			break;
		case NO_ENCONTRO_POKEMON:
			bloquearEntrenador(entrenador);
			break;
		default:
			pthread_mutex_lock(&mutex_cola_listos);
			queue_push(colaListos, entrenador);
			informar_contenido_cola(colaListos);
			pthread_mutex_unlock(&mutex_cola_listos);
			break;
		}
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
	list_clean(listaAux);
	list_destroy(listaAux);
	return buscarEntrenador(entrenador->id, colaListos->elements);
}

void liberarRecursos2(t_entrenador* entrenadorLiberado){
	int index_entrenador, pokemon_asignado;
	t_infoPokemon* infoPokemon;
	t_entrenador* entrenadorBloqueado;
	uint8_t operation_code;
	void* buffer;
	log_trace(log_mapa, "Arranca a matar a entrenador");
	while((infoPokemon = list_remove(entrenadorLiberado->pokemons, 0)) != NULL){
		pokemon_asignado = 0;

		for(index_entrenador = 0; index_entrenador < list_size(colaBloqueados->elements); index_entrenador++){
			entrenadorBloqueado = list_get(colaBloqueados->elements, index_entrenador);
			if(entrenadorBloqueado->pokenest_buscada == infoPokemon->id_pokenest){
				list_add(entrenadorBloqueado->pokemons, infoPokemon);
				log_trace(log_mapa, "notificando a bloqueado que obtuvo pokemon");
				notificar_captura_pokemon(infoPokemon, entrenadorBloqueado);
				if(entrenadorBloqueado->ultimo_pokemon){
					connection_recv(entrenadorBloqueado->id, &operation_code, &buffer);
					if(operation_code != OC_OBTENER_CANTIDAD_DEADLOCK){
						log_error(log_mapa, "error durante solicitud de deadlocks");
						exit(1);
					}
					liberarRecursos2(entrenadorBloqueado);
					sem_post(&sem_dibujo);
				} else {
					desbloquearEntrenador(entrenadorBloqueado);
					pokemon_asignado = 1;
					break;
				}
			}
		}

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
	list_clean(entrenadorLiberado->pokemons);
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
	informar_contenido_cola(colaBloqueados);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

void desbloquearEntrenador(t_entrenador* entrenador){
	t_entrenador* trainer = buscarEntrenador(entrenador->id, colaBloqueados->elements);
	queue_push(colaListos, trainer);
	FD_CLR(entrenador->id, &master);
	informar_contenido_cola(colaListos);
}
