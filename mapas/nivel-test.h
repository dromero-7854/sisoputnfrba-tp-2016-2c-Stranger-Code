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
t_list *listaPokenests;
t_list *items;
t_list *entrenadores;
int quantum;

typedef struct {
	//int fd;
	int posx;
	int posy;

	bool movVert;
	int id;
	char estado;

	char *proximoMapa;
	char *objetivos;
	int objetivoActual;
	char quantum;

	t_list * pokemons;

	bool bloq;
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

typedef struct {
	t_list * entrenadores;
	t_list * pokenests;
} t_combo;

typedef struct PokeNest {
	char id;
	char *nombrePokemon;
	char posx;
	char posy;
	char cantidad;
	t_queue * colaBloqueados;
	t_pokemon_type tipo;
	t_list* listaPokemons;
} PokeNest;

metadata* conf_metadata;

t_entrenador* crearEntrenador(int file_descriptor);
void liberarEntrenador();
#endif /* NIVEL_TEST_H_ */
