/*
 * nivel-test.c


 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#include <curses.h>
#include <tad_items.h>
#include <nivel.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "nivel-test.h"
#include "planificacion.h"
#include "deteccionDeadlock.h"
#include "dibujarNivel.h"
#include <bibliotecaCharMander.h>
#include <signal.h>

#define QUANTUM 5


void releerConfiguracion(int n){
	leerConfiguracion(conf_metadata, rutaMetadata);
}




int main(int argc, char* argv[]) {

	int len_nombre_mapa, len_pto_mnt;
	signal(SIGUSR1, releerConfiguracion);
	log_mapa = crear_log(argv[1]);
	//getchar();
	nombre_mapa = argv[1];
	pto_montaje = argv[2];

	ruta_mapa = getRutaMapa(pto_montaje, nombre_mapa);

	items  = list_create();
	entrenadores = list_create();
	listaPokenests = list_create();

	colaListos = queue_create();
	colaBloqueados = queue_create();


	listaPokenests = list_create();
	rutaMetadata = getRutaMetadata();
	log_trace(log_mapa, "Se obtuvo la ruta a metadata %s", rutaMetadata);
	conf_metadata = malloc(sizeof(metadata));
	leerConfiguracion(conf_metadata, rutaMetadata);
	log_trace(log_mapa, "se cargo la metadata");
	pthread_t planificador;
	pthread_attr_t attr;

	pthread_mutex_init(&mutex_cola_listos, NULL);
	pthread_attr_init(&attr);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&planificador, &attr, (void *) planificar, NULL) != 0){
		log_error(log_mapa, "problemas al crear hilo planificador");
	}


	pthread_t pth, guipth;
	t_combo comboListas;
	comboListas.entrenadores = entrenadores;
	comboListas.pokenests = listaPokenests;

	//getchar();
	log_trace(log_mapa, "Se iniciaron las colas y listas");
	if(pthread_create(&pth, NULL, (void *)detectarDeadlock, &comboListas)) {

		log_error(log_mapa, "Error creando hilo deadlock\n");
		return 1;

	}
	log_trace(log_mapa, "se creo hilo deadlock");
	//getchar();


	//colaListos = queue_create();
	//colaBloqueados = queue_create();

	char *rutaPokenests;
	rutaPokenests = getRutaPokenests();
	t_pkmn_factory* fabrica = create_pkmn_factory();

	cargarPokenests(rutaPokenests, fabrica);
	pthread_attr_t attr2;

	pthread_attr_init(&attr2);
	pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&guipth, &attr2, (void *) dibujarNivel, &comboListas)) {
				log_error(log_mapa, "Error creando el hilo de la GUI");
				return 1;
	}
	log_trace(log_mapa, "se creo hilo de dibujo");
	int listener;
	listener = socket_servidor(conf_metadata->ip, conf_metadata->puerto, log_mapa);
	manejar_select(listener, log_mapa);

	//liberar conf_metadata
	return EXIT_SUCCESS;
}

void manejar_select(int socket, t_log* log){
	fd_set lectura, master;
	int nuevaConexion, a, recibido, fdMax;
	char buf[512];
	char* objetivos;
	char simbolo;
	fdMax = socket;
	FD_ZERO(&lectura);
	FD_ZERO(&master);
	FD_SET(socket, &master);
	while(1){
		lectura = master;
		select(fdMax +1, &lectura, NULL, NULL, NULL);
		for(a = 0 ; a <= fdMax ; a++){
			if(FD_ISSET(a, &lectura)){
					if(a == socket){
						nuevaConexion = aceptar_conexion(socket, log);
						//simbolo = handshake(nuevaConexion, objetivos);
						simbolo = handshake(nuevaConexion, objetivos);
						FD_SET(nuevaConexion, &master);
						if(nuevaConexion > fdMax) fdMax = nuevaConexion;
						//t_entrenador* nuevoEntrenador = crearEntrenador(nuevaConexion, simbolo, objetivos);
						t_entrenador* nuevoEntrenador = crearEntrenador(nuevaConexion, simbolo);
						nuevoEntrenador->objetivos = objetivos;

						CrearPersonaje(items, nuevoEntrenador->simbolo, nuevoEntrenador -> posx, nuevoEntrenador -> posy);
						list_add(entrenadores, nuevoEntrenador);
						pthread_mutex_lock(&mutex_cola_listos);
						queue_push(colaListos, nuevoEntrenador);
						pthread_mutex_unlock(&mutex_cola_listos);
					}/*else {
						recibido = recv(a,  (void*) buf, 512, 0);
						if(recibido <= 0){
							if(recibido < 0){
								log_error(log, "Error al recibir de %d", a);
								printf("error");
								} else {
								eliminarEntrenador(a);
								log_error(log, "Se desconecto %d", a);
								printf("Se desconecto alguien\n");
								}
							close(a);
							FD_CLR(a, &master);
							}
						}*/
				}
		}
	}
}

void leerConfiguracion(metadata* conf_metadata, char* ruta){
	t_config* configuracion = config_create(ruta);
	conf_metadata->tiempoChequeoDeadlock = config_get_int_value(configuracion, "TiempoChequeoDeadlock");
	conf_metadata->batalla = config_get_int_value(configuracion, "Batalla");
	meterStringEnEstructura(&(conf_metadata->algoritmo), config_get_string_value(configuracion, "algoritmo"));
	meterStringEnEstructura(&(conf_metadata->ip), config_get_string_value(configuracion, "IP"));
	meterStringEnEstructura(&(conf_metadata->puerto), config_get_string_value(configuracion, "Puerto"));
	conf_metadata->retardo = config_get_long_value(configuracion, "retardo");
	conf_metadata->quantum = config_get_int_value(configuracion, "quantum");
	tim.tv_sec = 0;
	tim.tv_nsec = (conf_metadata->retardo) * 1000000;
	config_destroy(configuracion);
}

char* getRutaMapa(char* ptoMnt, char* nombreMapa){
	int letrasPto, letrasNombre, len;
	char *ruta, *directorio;
	letrasPto = strlen(ptoMnt);
	letrasNombre = strlen(nombreMapa);
	directorio = strdup("Mapas");
	len = strlen(directorio);
	ruta = malloc(letrasPto + letrasNombre + len + 2);
	snprintf(ruta, letrasPto + letrasNombre + len + 2, "%s%s/%s", ptoMnt, directorio, nombreMapa);
	free(directorio);
	return ruta;
}
char* getRutaMetadata(){
	char* directorio = strdup("/metadata");
	int x = strlen(directorio);
	int len_ruta_mapa = strlen(ruta_mapa);
	char* ruta = malloc(len_ruta_mapa + x + 1);
	snprintf(ruta, len_ruta_mapa + x + 1, "%s%s", ruta_mapa, directorio);
	free(directorio);
	return ruta;
}

char* getRutaPokenests(){
	char* directorio = strdup("/PokeNests");
	int x = strlen(directorio);
	char* ruta = malloc(strlen(ruta_mapa) + x + 1);
	snprintf(ruta, strlen(ruta_mapa) + x + 1, "%s%s", ruta_mapa, directorio);
	free(directorio);
	return ruta;
}

char* getRutaPokemon(char* rutaPokenests, char* pokemon){
	int cantLetras = strlen(rutaPokenests) + strlen(pokemon) + 2;
	char* ruta = malloc(cantLetras);
	snprintf(ruta, cantLetras, "%s/%s", rutaPokenests, pokemon);
	return ruta;
}

PokeNest* crearPokenest(char* rutaPokenest){
	char* metadata;
	PokeNest* pokeNest = malloc(sizeof(PokeNest));
	char* aux = strdup("/metadata");
	int len = strlen(aux);
	metadata = malloc(strlen(rutaPokenest) + len + 1);
	snprintf(metadata, strlen(rutaPokenest) + len + 1, "%s%s", rutaPokenest, aux);
	t_config* config = config_create(metadata);
	pokeNest->id = *(config_get_string_value(config, "Identificador"));
	char *posiciones, *posicion;

	meterStringEnEstructura(&posiciones, config_get_string_value(config, "Posicion"));
	posicion = strtok(posiciones, ";");
	pokeNest->posx = atoi(posicion);
	posicion = strtok(NULL, ";");
	pokeNest->posy = atoi(posicion);
	//free(posiciones);
	//free(posicion);
	free(aux);
	free(metadata);
	config_destroy(config);
	return pokeNest;
}

t_list* crearPokemons(char* rutaPokemon, t_pkmn_factory* fabrica, char* nombrePokemon){
	DIR* dir;
	struct dirent* directorio;
	dir = opendir(rutaPokemon);
	char* metadataPokemon;
	int len, lvl;
	t_pokemon* pokemon;
	t_list* listaPokemons = list_create();
	while((directorio = readdir(dir)) != NULL){
		if(!strcmp(directorio->d_name, ".") || !strcmp(directorio->d_name, "..")) continue;
		if(!strcmp(directorio->d_name, "metadata")) continue;

		len = strlen(directorio->d_name);
		metadataPokemon = malloc( strlen(rutaPokemon) + len + 2);
		snprintf(metadataPokemon, strlen(rutaPokemon) + len + 2, "%s/%s", rutaPokemon, directorio->d_name);
		t_config* config = config_create(metadataPokemon);
		lvl = config_get_int_value(config, "Nivel");

		t_infoPokemon* infoPokemon = malloc(sizeof(t_infoPokemon));
		infoPokemon->pokemon = create_pokemon(fabrica, nombrePokemon, lvl);
		meterStringEnEstructura(&(infoPokemon->nombre), directorio->d_name);

		list_add(listaPokemons, infoPokemon);

		//free(infoPokemon);
		free(metadataPokemon);
		config_destroy(config);
	}
	closedir(dir);
	return listaPokemons;
}

t_entrenador* crearEntrenador(int file_descriptor, char simbolo){
	t_entrenador* entrenador = malloc(sizeof(t_entrenador));
	entrenador->id = file_descriptor;
	entrenador->posx = 1;
	entrenador->posy = 1;
	//entrenador->objetivos = objetivos;
	entrenador->pokemons = list_create();
	entrenador->simbolo = simbolo;
	return entrenador;
}

void eliminarEntrenador(int fd_entrenador){
	buscar_entrenador_y_borrar(colaListos, fd_entrenador);
	buscar_entrenador_y_borrar(colaBloqueados, fd_entrenador);
}

void buscar_entrenador_y_borrar(t_queue* cola, int file_descriptor){
	//t_list* listaAux;
	t_entrenador* entrenadorAux;

	pthread_mutex_lock(&mutex_cola_listos);
	entrenadorAux = buscarEntrenador(file_descriptor, cola->elements);
	pthread_mutex_unlock(&mutex_cola_listos);
	liberarEntrenador(entrenadorAux);
}

void liberarEntrenador(t_entrenador* entrenador){
	//free(entrenador->objetivos);
	//free(entrenador->proximoMapa);
	BorrarItem(items, entrenador->simbolo);
	list_destroy(entrenador->pokemons);
	free(entrenador);
}

void cargarPokenests(char* rutaPokenests, t_pkmn_factory* fabrica){
	DIR* d;
	struct dirent *directorio;
	d = opendir(rutaPokenests);
	char* rutaPokemon;
	while((directorio = readdir(d)) != NULL){
		if((!strcmp(directorio->d_name, ".")) || (!strcmp(directorio->d_name, ".."))) continue;
		rutaPokemon = getRutaPokemon(rutaPokenests, directorio->d_name);

		PokeNest* pokenest = crearPokenest(rutaPokemon);

		pokenest->listaPokemons = crearPokemons(rutaPokemon, fabrica, directorio->d_name);
		pokenest->cantidad = list_size(pokenest->listaPokemons);
		list_add(listaPokenests, pokenest);

		CrearCaja(items, pokenest->id, pokenest->posx, pokenest->posy, pokenest->cantidad);
		free(rutaPokemon);
	}
	closedir(d);
}

t_log* crear_log(char* nombre){
	char nombre_archivo[256];
	snprintf((char *)&nombre_archivo, 256, "%s.log", nombre);
	t_log* logger = log_create(nombre_archivo, "MAPA", false, LOG_LEVEL_TRACE);
	return logger;
}
void destruir_config(metadata* config){
	free(config->algoritmo);
	free(config->ip);
	free(config->puerto);
	free(config);
}
