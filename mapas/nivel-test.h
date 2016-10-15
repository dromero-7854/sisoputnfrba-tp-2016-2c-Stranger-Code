/*
 * nivel-test.h
 *
 *  Created on: 30/9/2016
 *      Author: utnso
 */

#ifndef NIVEL_TEST_H_
#define NIVEL_TEST_H_

#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/log.h>
#include <dirent.h>
#include <pkmn/factory.h>

#define DIRECCION_METADATA "/home/utnso/workspace/tp-2016-2c-Stranger-Code/mapas/Debug/metadata"

char* getRutaPokenests(char* ptoMnt, char* nombreMapa);
char* getRutaPokemon(char* rutaPokenests, char* pokemon);
t_list* crearPokemons(char* rutaPokemon, t_pkmn_factory* fabrica, char* nombrePokemon);
char* getRutaMetadata(char* ptoMnt, char* nombreMapa);


t_queue *colaListos, *colaBloqueados;

typedef struct {
	int fd;
	int posx;
	int posy;
}t_entrenador;

typedef struct {
	int tiempoChequeoDeadlock;
	int batalla;
	char* algoritmo;
	int quantum;
	int retardo;
	char* ip;
	char* puerto;
}metadata;

typedef struct {
	t_pokemon* pokemon;
	char* nombre;
}t_infoPokemon;
#endif /* NIVEL_TEST_H_ */
