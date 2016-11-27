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
#include <pthread.h>

#define DIRECCION_METADATA "/home/utnso/workspace/tp-2016-2c-Stranger-Code/mapas/Debug/metadata"

char* getRutaPokenests();
char* getRutaPokemon(char* rutaPokenests, char* pokemon);
t_list* crearPokemons(char* rutaPokemon, t_pkmn_factory* fabrica, char* nombrePokemon);
char* getRutaMetadata();
char* getRutaMapa(char* ptoMnt, char* nombreMapa);


t_log *log_mapa;
t_queue *colaListos, *colaBloqueados;
t_list *listaPokenests;
t_list *items;
t_list *entrenadores;
int quantum;
int rows, cols;

typedef struct {

	int posx;
	int posy;

	bool movVert;
	int id;
	char simbolo;
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
	char id_pokenest;
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
char* rutaMetadata;
pthread_mutex_t mutex_cola_listos;
char* pto_montaje;
char* nombre_mapa;
char* ruta_mapa;

void manejar_select(int socket, t_log* log);
t_entrenador* crearEntrenador(int file_descriptor, char simbolo);
void liberarEntrenador();
void cargarPokenests(char* rutaPokenests, t_pkmn_factory* fabrica);
void buscar_entrenador_y_borrar(t_queue* cola, int file_descriptor);
void eliminarEntrenador(int fd_entrenador);
t_log* crear_log(char* nombre);
PokeNest* crearPokenest(char* rutaPokenest);
void moverJugadores(t_list *entrenadores, t_list *items);
void moverJugador(t_entrenador *personaje, t_list *items,int x,int y);
void leerConfiguracion(metadata* conf_metadata, char* ruta);

#endif /* NIVEL_TEST_H_ */
