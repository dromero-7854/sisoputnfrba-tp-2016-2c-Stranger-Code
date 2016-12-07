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

void detectarDeadlock(t_combo * comboLista) {

	//getchar();
	log_deadlock = crear_log("deadlock");

	struct timespec retardo, opa;
	retardo.tv_sec = conf_metadata->tiempoChequeoDeadlock / 1000;
	retardo.tv_nsec = (conf_metadata->tiempoChequeoDeadlock % 1000) * 1000000;

	log_trace(log_deadlock, "Se inicia el hilo de deteccion de deadlock");
	log_trace(log_deadlock, "El retardo es de %d segundos, %d milisegundos",
			retardo.tv_sec, retardo.tv_nsec);

	while (1) {

		nanosleep(&retardo, &opa);

		t_list * entrenadores = comboLista->entrenadores;
		t_list * pokenests = comboLista->pokenests;
		t_list * deadlockeados = list_create();

		int cantidadPokenest = list_size(pokenests);
		int cantidadEntrenadores = list_size(entrenadores);

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
		int hayDeadlock = 0;
		int tieneUnPedido = 0;

		while (!hayDeadlock
				&& hayAlguienParaAtender(atendido, cantidadEntrenadores)) {

			if (cantidadEntrenadores >= 2)
				hayDeadlock = 1;

			for (i_entrenadores = 0; i_entrenadores < cantidadEntrenadores;
					i_entrenadores++) {
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

					for (i_pokenest = 0; i_pokenest < cantidadPokenest;
							i_pokenest++) {
						pokemonsDisponibles[i_pokenest] +=
								matrizUtilizados[i_entrenadores][i_pokenest];
						matrizUtilizados[i_entrenadores][i_pokenest] = 0;
						atendido[i_entrenadores] = 1;
					}
				}
			}
		}
		for (i_entrenadores = 0; i_entrenadores < cantidadEntrenadores;
				i_entrenadores++) {
			t_entrenador * entrenador = list_get(entrenadores, i_entrenadores);
			if (!atendido[i_entrenadores])
				list_add(deadlockeados, entrenador);
		}

		if (list_size(deadlockeados) >= 2) {

			int coinciden;

			coinciden = list_all_satisfy(deadlockeados, (void *) estaBloqueado);

			log_trace(log_deadlock, "HAY DEADLOCK");

			if (conf_metadata->batalla && coinciden) {

				t_entrenador * entrenador1 = list_remove(deadlockeados, 0);

				while (list_size(deadlockeados)) {
					t_entrenador * entrenador2 = list_remove(deadlockeados, 0);

					log_trace(log_deadlock,
							"Se realizara una battalla entre %c y %c",
							entrenador1->simbolo, entrenador2->simbolo);
					t_entrenador * loser = mandarAPelear(entrenador1,
							entrenador2);

					entrenador1 = loser;
				}
				matarEntrenador(entrenador1);
			}
		}
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
t_entrenador * mandarAPelear(t_entrenador* entrenador1,
		t_entrenador* entrenador2) {
	list_sort(entrenador1->pokemons, (void*) esDeMayorNivel);
	list_sort(entrenador2->pokemons, (void*) esDeMayorNivel);

	t_infoPokemon * pokemon1 = list_get(entrenador1->pokemons, 0);
	t_infoPokemon * pokemon2 = list_get(entrenador2->pokemons, 0);
	t_pokemon * pok1 = pokemon1->pokemon;
	t_pokemon * pok2 = pokemon2->pokemon;

	log_trace(log_deadlock, "%c eligio a  %c (nivel %d)", entrenador1->simbolo,
			pok1->species, pok1->level);
	log_trace(log_deadlock, "%c eligio a  %c (nivel %d)", entrenador2->simbolo,
			pok2->species, pok2->level);
	t_pokemon * loser = pkmn_battle(pok1, pok2);

	if (loser == pok1)
		return entrenador1;
	else
		return entrenador2;

}
void matarEntrenador(t_entrenador * entrenador) {

	int i;
	uint8_t operation_code = OC_VICTIMA_DEADLOCK;

	//memcpy(mandar, &operation_code, sizeof(uint8_t));
	send(entrenador->id, &operation_code, sizeof(uint8_t), 0);

	for (i = 0; entrenador != list_get(entrenadores, i); i++);

	FD_CLR(entrenador->id, &master);
	list_remove(entrenadores, i);

	liberarRecursos2(entrenador);
}
int hayAlguienParaAtender(int atendido[], int cantEntrenadores) {
	int i;
	for (i = 0; i < cantEntrenadores; i++) {
		if (atendido[i] == 0)
			return 1;
	}
	return 0;
}
