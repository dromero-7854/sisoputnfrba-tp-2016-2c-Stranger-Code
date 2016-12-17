/*
 * deteccionDeadlock.c
 *
 *  Created on: 18/10/2016
 *      Author: utnso
 */

#include "deteccionDeadlock.h"
#include "nivel-test.h"
#include <time.h>

bool estaBloqueado(t_entrenador * entrenador);
int hayAlguienParaAtender(int vectorAtendido[], int cantidad);
void mostrarMatriz(int entrenadores, int pokenests, int matriz[entrenadores][pokenests]);
t_list* duplicar_lista(t_list*);
void detectarDeadlock(t_combo * comboLista) {

	char * archivoDeadlock = string_from_format("deadlock_%s", nombre_mapa);
	log_deadlock = crear_log(archivoDeadlock);

	struct timespec retardo, opa;
	retardo.tv_sec = conf_metadata->tiempoChequeoDeadlock / 1000;
	retardo.tv_nsec = (conf_metadata->tiempoChequeoDeadlock % 1000) * 1000000;

	log_trace(log_deadlock, "Se inicia el hilo de deteccion de deadlock");
	log_trace(log_deadlock, "El retardo es de %d segundos, %d milisegundos",
			retardo.tv_sec, retardo.tv_nsec);

	while (1) {

		nanosleep(&retardo, &opa);

		t_list * entrenadores = duplicar_lista(comboLista->entrenadores);
		t_list * pokenests = comboLista->pokenests;
		t_list * deadlockeados = list_create();

		int cantidadPokenest = list_size(pokenests);
		int cantidadEntrenadores = list_size(entrenadores);
		log_trace(log_deadlock, "cant. de lista Entrenadores: %d", list_size(entrenadores));

		int pokemonsDisponibles[cantidadPokenest];
		int atendido[cantidadEntrenadores];
		int matrizUtilizados[cantidadEntrenadores][cantidadPokenest];
		int matrizPedidos[cantidadEntrenadores][cantidadPokenest];

		int i_entrenadores, i_pokenest, i_pokemon;

		for (i_pokenest = 0; i_pokenest < cantidadPokenest; i_pokenest++) {
			PokeNest *pokenest = list_get(pokenests, i_pokenest);

			pokemonsDisponibles[i_pokenest] = pokenest->cantidad;
		}

		for (i_entrenadores = 0; i_entrenadores < cantidadEntrenadores;
				i_entrenadores++) {
			t_entrenador *entrenador = list_get(entrenadores, i_entrenadores);
			atendido[i_entrenadores] = 0;

			int i_pokenest = 0;
			while (i_pokenest < cantidadPokenest) {
				PokeNest *pokenest = list_get(pokenests, i_pokenest);
				int contador_pokemon = 0;

				for (i_pokemon = 0; i_pokemon < list_size(entrenador->pokemons);
						i_pokemon++) {
					t_infoPokemon * pokemon = list_get(entrenador->pokemons,
							i_pokemon);
					if (pokemon->id_pokenest == pokenest->id)
						contador_pokemon++;

				}
				matrizUtilizados[i_entrenadores][i_pokenest] = contador_pokemon;

				if (pokenest->id == entrenador->pokenest_buscada)
					matrizPedidos[i_entrenadores][i_pokenest] = 1;
				else
					matrizPedidos[i_entrenadores][i_pokenest] = 0;
				i_pokenest++;
			}
		}

		log_info(log_deadlock, "MATRIZ DE POKEMONS RETENIDOS");
		mostrarMatriz(cantidadEntrenadores, cantidadPokenest, matrizUtilizados);

		log_info(log_deadlock, "MATRIZ DE POKEMONS BUSCADOS");
		mostrarMatriz(cantidadEntrenadores, cantidadPokenest, matrizPedidos);

		int hayDeadlock = 0;
		int tieneUnPedido = 0;

		while (!hayDeadlock && hayAlguienParaAtender(atendido, cantidadEntrenadores)) {

			if (cantidadEntrenadores >= 2)
				hayDeadlock = 1;

			for (i_entrenadores = 0; i_entrenadores < cantidadEntrenadores; i_entrenadores++) {
				for (i_pokenest = 0; i_pokenest < cantidadPokenest;
						i_pokenest++) {
					if (matrizPedidos[i_entrenadores][i_pokenest]) {
						tieneUnPedido = 1;
						break;
					}
				}
				if (tieneUnPedido && pokemonsDisponibles[i_pokenest] > 0) {
					matrizUtilizados[i_entrenadores][i_pokenest]++;
					pokemonsDisponibles[i_pokenest]--;
					matrizPedidos[i_entrenadores][i_pokenest] = 0;
					atendido[i_entrenadores] = 1;
					hayDeadlock = 0;
				} else if (!tieneUnPedido) {

					for (i_pokenest = 0; i_pokenest < cantidadPokenest; i_pokenest++) {
						pokemonsDisponibles[i_pokenest] +=
								matrizUtilizados[i_entrenadores][i_pokenest];
						matrizUtilizados[i_entrenadores][i_pokenest] = 0;
						atendido[i_entrenadores] = 1;
					}
				}
			}
		}
		for (i_entrenadores = 0; i_entrenadores < cantidadEntrenadores; i_entrenadores++) {
			t_entrenador * entrenador = list_get(entrenadores, i_entrenadores);
			if (!atendido[i_entrenadores]) {
				list_add(deadlockeados, entrenador);
			}
		}


		char _map_objetivosDelEntrenador(t_entrenador * entrenadorMapeado) {
			return entrenadorMapeado->pokenest_buscada;
		}
		t_list * listaDeObjetivos = list_map(deadlockeados, (void *) _map_objetivosDelEntrenador);

		int retieneAlgunObjetivo(t_entrenador * entrenador) {

			char _map_IDPOKENEST(t_infoPokemon * infoPokemon) {
				return infoPokemon->id_pokenest;
			}

			t_list * listaIdPokemonsRetenidos = list_map(entrenador->pokemons, (void *) _map_IDPOKENEST);

			int _esUnPokemonBuscadoPorAlguien(char retenido) {
				int _esIgual(char objetivo) {
					return objetivo == retenido;
				}
				return list_any_satisfy(listaDeObjetivos, (void *) _esIgual);
			}
			int retieneAlguno = list_any_satisfy(listaIdPokemonsRetenidos, (void *) _esUnPokemonBuscadoPorAlguien);
			list_clean(listaIdPokemonsRetenidos);
			list_destroy(listaIdPokemonsRetenidos);
			return retieneAlguno;
		}
		deadlockeados = list_filter(deadlockeados, (void *) retieneAlgunObjetivo);

		if (list_size(deadlockeados) >= 2) {

			int coinciden;

			coinciden = list_all_satisfy(deadlockeados, (void *) estaBloqueado);

			void _contarDeadlock(t_entrenador* entr) {
				entr->cantDeadlocks++;
			}

			list_iterate(deadlockeados, (void *) _contarDeadlock);

			if(coinciden)
				log_info(log_deadlock, "HAY DEADLOCK");

			if (conf_metadata->batalla && coinciden) {

				log_info(log_deadlock, "HAY BATALLA");

				pthread_mutex_lock(&deadlock_ejecutando);
				t_entrenador * entrenador1 = list_remove(deadlockeados, 0);


				while (list_size(deadlockeados)) {
					t_entrenador * entrenador2 = list_remove(deadlockeados, 0);

					log_trace(log_deadlock, "Se realizara una battalla entre %c y %c", entrenador1->simbolo, entrenador2->simbolo);
					t_entrenador * loser = mandarAPelear(entrenador1, entrenador2);

					entrenador1 = loser;

					if( loser == -1)
						break;
				}
				if(entrenador1 != -1)
					matarEntrenador(entrenador1);

				pthread_mutex_unlock(&deadlock_ejecutando);
			}
		}
		list_clean(listaDeObjetivos);
		list_destroy(listaDeObjetivos);
		list_clean(deadlockeados);
		list_destroy(deadlockeados);
		list_clean_and_destroy_elements(entrenadores, (void*) _liberar_entrenador_de_lista);
		list_destroy(entrenadores);
	}
}
bool estaBloqueado(t_entrenador * entrenador) {

	int i = 0;
	for (; i < list_size(colaBloqueados->elements); i++) {
		t_entrenador *entrBloq = list_get(colaBloqueados->elements, i);

		if (entrBloq->simbolo == entrenador->simbolo)
			return 1;
	}
	return 0;
}
bool esDeMayorNivel(t_infoPokemon * pokemon1, t_infoPokemon * pokemon2) {
	return pokemon1->pokemon->level > pokemon2->pokemon->level;
}
t_entrenador * mandarAPelear(t_entrenador* entrenador1, t_entrenador* entrenador2) {

	if(entrenador1 == NULL|| entrenador2 == NULL)
		return -1;

	uint8_t operation_code;
	char *nombrePokemon;
	uint8_t operation_code_loser = OC_PERDIO_BATALLA;
	uint8_t operation_code_winner = OC_GANO_BATALLA;
	uint8_t size = 0;
	t_pokemon * pok1;
	t_pokemon * pok2;


	enviar_oc(entrenador1->id, OC_POKEMON_BATALLA);
	connection_recv(entrenador1->id, &operation_code, &nombrePokemon);

	pok1 = buscar_pokemon_de_entrenador(entrenador1, nombrePokemon);
	free(nombrePokemon);

	enviar_oc(entrenador2->id, OC_POKEMON_BATALLA);
	connection_recv(entrenador2->id, &operation_code, &nombrePokemon);

	pok2 = buscar_pokemon_de_entrenador(entrenador2, nombrePokemon);
	free(nombrePokemon);

	log_info(log_deadlock, "%s (%c) eligio a  %s (nivel %d)", entrenador1->nombre, entrenador1->simbolo, pok1->species, pok1->level);
	log_info(log_deadlock, "%s (%c) eligio a  %s (nivel %d)", entrenador2->nombre, entrenador2->simbolo, pok2->species, pok2->level);

	t_pokemon * loser = pkmn_battle(pok1, pok2);

	if (loser == pok1) {

		enviar_oc(entrenador2->id, operation_code_winner);

		enviar_oc(entrenador1->id, operation_code_loser);

		log_info(log_deadlock, "%s (%c) ha ganado la batalla frente a %s (%c)", entrenador2->nombre, entrenador2->simbolo, entrenador1->nombre, entrenador1->simbolo);

		return entrenador1;
	}
	else {

		enviar_oc(entrenador1->id, operation_code_winner);

		enviar_oc(entrenador2->id, operation_code_loser);

		log_info(log_deadlock, "%s (%c) ha ganado la batalla frente a %s (%c)", entrenador1->nombre, entrenador1->simbolo, entrenador2->nombre, entrenador2->simbolo);

		return entrenador2;
	}

}
void matarEntrenador(t_entrenador * entrenador) {

	uint8_t operation_code = OC_VICTIMA_DEADLOCK;
	uint8_t tamanio = 0;

	send(entrenador->id, &operation_code, sizeof(uint8_t), 0);
	send(entrenador->id, &tamanio, sizeof(uint8_t), 0);

	FD_CLR(entrenador->id, &master);

	pthread_mutex_lock(&mutex_turno_desbloqueo);
	pthread_mutex_lock(&mutex_cola_bloqueados);
	buscarEntrenador(entrenador->id, colaBloqueados->elements);
	liberarRecursos2(entrenador);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	pthread_mutex_unlock(&mutex_turno_desbloqueo);
}
int hayAlguienParaAtender(int atendido[], int cantEntrenadores) {
	int i;
	for (i = 0; i < cantEntrenadores; i++) {
		if (atendido[i] == 0)
			return 1;
	}
	return 0;
}
void mostrarMatriz(int cantEntrenadores, int pokenests, int matriz[cantEntrenadores][pokenests]) {

	int i, j;
	char id[2*pokenests+1];

	char borde[2*pokenests+1];
	borde[2*pokenests]=0;
	for(j = 0; j < 2*pokenests; j++) {
		borde[j]='_';

		i = j/2;
		if((j % 2)==0) {

			PokeNest * pok = list_get(listaPokenests, i);
			id[j] = pok->id;
			id[j+1] = '|';
		}
	}
	id[2*pokenests] = 0;
	log_info(log_deadlock, "			| %s", id);
	for(i = 0; i < cantEntrenadores; i++) {

		t_entrenador * entr = list_get(entrenadores, i);
		char fila[2*pokenests+1];

		for(j = 0; j < 2*pokenests; j += 2) {
			fila[j] = matriz[i][j/2] + '0';
			fila[j+1] = '|';
		}
		fila[2*pokenests] = 0;
		log_info(log_deadlock, "%s (%c)	| %s", entr->nombre, entr->simbolo , fila);
	}
	log_info(log_deadlock, "		%s", borde);
}

t_list* duplicar_lista(t_list* lista_original){
	t_list* lista_nueva = list_create();
	void _duplicar_entrenador(t_entrenador* e){
		t_entrenador* e_nuevo = malloc(sizeof(t_entrenador));
		e_nuevo->nombre = string_duplicate(e->nombre);
		e_nuevo->cantDeadlocks = e->cantDeadlocks;
		e_nuevo->id = e->id;
		e_nuevo->pokemons = e->pokemons;
		e_nuevo->pokenest_buscada = e->pokenest_buscada;
		e_nuevo->posx = e->posx;
		e_nuevo->posy = e->posy;
		e_nuevo->simbolo = e->simbolo;
		//e_nuevo->tiempos = e->tiempos;
		e_nuevo->ultimo_pokemon = e->ultimo_pokemon;
		list_add(lista_nueva, e_nuevo);
	}
	list_iterate(lista_original, (void*) _duplicar_entrenador);
	return lista_nueva;
}

t_pokemon* buscar_pokemon_de_entrenador(t_entrenador* entrenador, char* nombre_pokemon){
	t_infoPokemon* infoPoke;
	int _mismo_nombre(t_infoPokemon* infopokemon){
		return string_equals_ignore_case(infopokemon->nombre, nombre_pokemon);
	}
	infoPoke = list_find(entrenador->pokemons, (void*)_mismo_nombre);
	return infoPoke->pokemon;
}

void enviar_oc(int socket, uint8_t oc_send){
	uint8_t tamanio = 0;
	void* buffer = malloc(sizeof(uint8_t) * 2);
	memcpy(buffer, &oc_send, sizeof(uint8_t));
	memcpy(buffer + sizeof(uint8_t), &tamanio, sizeof(uint8_t));
	send(socket, buffer, sizeof(uint8_t) * 2, 0);
	free(buffer);
}

void _liberar_entrenador_de_lista(t_entrenador* e){
	free(e->nombre);
	free(e);
}
