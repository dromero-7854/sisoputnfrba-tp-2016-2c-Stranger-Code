/*
 * deteccionDeadlock.c
 *
 *  Created on: 18/10/2016
 *      Author: utnso
 */

#include "deteccionDeadlock.h"
#include "nivel-test.h"

bool estaBloqueado(t_entrenador * entrenador);

void detectarDeadlock(t_combo * comboLista) {

	while (1) {

		t_log * log_deadlock = crear_log("deadlock");

		t_list * entrenadores = comboLista->entrenadores;
		t_list * pokenest = comboLista->pokenests;

		t_list * bloqueados = list_filter(entrenadores, (void*) estaBloqueado);

		int i, j;
		int count1 = 0, count2 = 0;
		int tamEntr = list_size(entrenadores);
		int tamPok = list_size(pokenest);
		int matrizRetenido[tamEntr][tamPok];
		int matrizNecesita[tamEntr][tamPok];
		int recursosDisponibles[tamPok];
		int completado[tamEntr];

		for (i = 0; i < tamEntr; i++) {
			completado[i] = 0;
			for (j = 0; j < tamPok; j++) {
				matrizRetenido[i][j] = 0;
				matrizNecesita[i][j] = 0;
			}
		}
		for (i = 0; i < tamPok; i++) {
			PokeNest * pok = list_get(pokenest, i);
			recursosDisponibles[i] = pok->cantidad;
		}

		for (i = 0; i < tamEntr; i++) {
			t_entrenador *entrenador = list_get(entrenadores, i);
			int k = 0;
			for (k = 0; k < entrenador->objetivoActual; k++) {
				char obj = *(entrenador->objetivos + k);

				for (j = 0; j < tamPok; j++) {
					PokeNest * pok = list_get(pokenest, j);

					if (pok->id == obj)
						break;
				}
				matrizRetenido[i][j]++;
			}
			for (; entrenador->objetivos + k != '\0'; k++) {
				char obj = *(entrenador->objetivos + k);
				for (j = 0; j < tamPok; j++) {
					PokeNest * pok = list_get(pokenest, j);

					if (pok->id == obj)
						break;
				}
				matrizNecesita[i][j]++;
			}
		}
		while (count1 < tamEntr) {
			count2 = count1;
			int k = 0;
			for (i = 0; i < tamEntr; i++) {

				for (j = 0; j < tamPok; j++) {
					if (matrizNecesita[i][j] <= recursosDisponibles[j])
						k++;
				}
				if (k == tamPok && completado[i] == 0) {
					completado[i] = 1;
					list_remove(bloqueados, i);

					for (j = 0; j < tamPok; j++) {
						recursosDisponibles[j] += matrizRetenido[i][j];
					}
					count1++;
				}
				k = 0;
			}
			if (count1 == count2) {

				log_trace(log_deadlock, "Se detecto un deadlock!");

				if (conf_metadata->batalla) {
					t_entrenador * entrenador1 = list_remove(bloqueados, 0);

					while (list_size(bloqueados) > 0) {

						t_entrenador * entrenador2 = list_remove(bloqueados, 0);
						t_entrenador * loser = mandarAPelear(entrenador1,
								entrenador2);
						entrenador1 = loser;

						free(loser);

					}
					matarEntrenador(entrenador1);
					free(entrenador1);
					break;
				}

			}
		}
	}

}
bool estaBloqueado(t_entrenador * entrenador) {

	int i = 0;
	for(; i < list_size(colaBloqueados->elements); i++) {
		t_entrenador *entrBloq = list_get(colaBloqueados->elements, i);

		if(entrBloq->simbolo == entrenador->simbolo)
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

	t_pokemon * pokemon1 = list_get(entrenador1->pokemons, 0);
	t_pokemon * pokemon2 = list_get(entrenador2->pokemons, 0);

	t_pokemon * loser = pkmn_battle(pokemon1, pokemon2);

	if (loser == pokemon1)
		return entrenador1;
	else
		return entrenador2;

}
void matarEntrenador(t_entrenador * entrenador) {

	int i;
	void * mandar;
	uint8_t operation_code = OC_VICTIMA_DEADLOCK;

	memcpy(mandar, &operation_code, sizeof(uint8_t));
	send(entrenador->id, mandar, sizeof(uint8_t), 0);

	for (i = 0; entrenador != list_get(entrenadores, i); i++);

	list_remove(entrenadores, i);

	liberarRecursos(entrenador);
}

