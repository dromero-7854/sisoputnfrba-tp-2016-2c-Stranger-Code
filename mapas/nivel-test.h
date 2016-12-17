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
#include <commons/string.h>
#include <dirent.h>
#include <pkmn/factory.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/types.h>
#include <curses.h>
#include <tad_items.h>
#include <nivel.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "../biblioteca-charmander/bibliotecaCharMander.h"
#include <signal.h>

#define DIRECCION_METADATA "/home/utnso/workspace/tp-2016-2c-Stranger-Code/mapas/Debug/metadata"

char* getRutaPokenests();
char* getRutaPokemon(char* rutaPokenests, char* pokemon);
t_list* crearPokemons(char* rutaPokemon, t_pkmn_factory* fabrica, char* nombrePokemon, char pokenest_id);
char* getRutaMetadata();
char* getRutaMapa(char* ptoMnt, char* nombreMapa);


t_log *log_mapa;
t_log * log_deadlock;
t_queue *colaListos, *colaBloqueados;
t_list *listaPokenests;
t_list *items;
t_list *entrenadores;
int quantum;
int rows, cols, set_fd_max;
fd_set master, lectura;

typedef struct {
	time_t inicio;
	time_t finalizacion;
	time_t bloqueo_acumulado;
	time_t inicia_bloqueo;
	time_t fin_bloqueo;
}t_tiempos;
typedef struct {

	char* nombre;
	int posx;
	int posy;
	int id;
	char simbolo;
	char pokenest_buscada;
	int cantDeadlocks;
	int ultimo_pokemon;

//	t_tiempos* tiempos;

	t_list * pokemons;

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
	//t_queue * colaBloqueados;
	t_pokemon_type tipo;
	t_list* listaPokemons;
} PokeNest;

pthread_t planificador;
pthread_t pth, guipth;

metadata* conf_metadata;
char* rutaMetadata;
pthread_mutex_t mutex_cola_listos;
pthread_mutex_t mutex_cola_bloqueados;
pthread_mutex_t mutex_cola_bloqueados2;
pthread_mutex_t mutex_turno_desbloqueo;
pthread_mutex_t mutex_lista_entrenador;
pthread_mutex_t mutex_lista_pokenest;
pthread_mutex_t dibujo;
pthread_mutex_t deadlock_ejecutando;
sem_t sem_dibujo;
sem_t sem_turno;

char* pto_montaje;
char* nombre_mapa;
char* ruta_mapa;
struct timespec tim;
struct timeval tv;

void manejar_select(int socket, t_log* log);
//t_entrenador* crearEntrenador(int file_descriptor, char simbolo, char* objetivos);
t_entrenador* crearEntrenador(int file_descriptor, char simbolo, char* nombre);
void liberarEntrenador();
void cargarPokenests(char* rutaPokenests, t_pkmn_factory* fabrica);
void buscar_entrenador_y_borrar(t_queue* cola, int file_descriptor);
void eliminarEntrenador(int fd_entrenador);
t_log* crear_log(char* nombre);
PokeNest* crearPokenest(char* rutaPokenest);
void leerConfiguracion(metadata* conf_metadata, char* ruta);
void liberar_variables_globales();
char* getRutaAbsoluta(char* rutaRelativa);
void _borrar_pokenest(PokeNest* pokenest);
void _borrar_pokemon(t_infoPokemon* infopokemon);
void vaciar_array(char** array);

#endif /* NIVEL_TEST_H_ */
