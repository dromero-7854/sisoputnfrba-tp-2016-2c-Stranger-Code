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
#include <bibliotecaCharMander.c>
#include "nivel-test.h"
#include "planificacion.h"
#include <deteccionDeadlock.h>
#include <pthread.h>


#define QUANTUM 5

int rows, cols;

PokeNest* crearPokenest(char* rutaPokenest);
void crearJugadores(t_list *entrenadores, t_list *items);
void moverJugadores(t_list *entrenadores, t_list *items);
void moverJugador(t_entrenador *personaje, t_list *items,int x,int y);
void leerConfiguracion(metadata* conf_metadata, char* ruta);



int main(int argc, char* argv[]) {

	t_list* items = list_create();
	t_list *entrenadores = list_create();
	t_list * pokenests = lista_create();

	nivel_gui_inicializar();

    nivel_gui_get_area_nivel(&rows, &cols);

    crearJugadores(entrenadores, items);


	nivel_gui_dibujar(items, "Stranger Code");

	pthread_t pth;
	t_combo comboListas;
	comboListas.entrenadores = entrenadores;
	comboListas.pokenests = pokenests;

	if(pthread_create(&pth, NULL, detectarDeadlock, &comboListas)) {

		fprintf(stderr, "Error creating thread\n");
		return 1;

	}

	moverJugadores(entrenadores, items);

	nivel_gui_terminar();

	char* rutaMetadata;
	t_list* listaPokenests = list_create();
	rutaMetadata = getRutaMetadata(argv[2], argv[1]);
	metadata* conf_metadata = malloc(sizeof(metadata));
	leerConfiguracion(conf_metadata, rutaMetadata);

	char *rutaPokenests;
	rutaPokenests = getRutaPokenests(argv[2], argv[1]);
	t_pkmn_factory* fabrica = create_pkmn_factory();
	DIR* d;
	struct dirent *directorio;
	d = opendir(rutaPokenests);
	while((directorio = readdir(d)) != NULL){
		if((!strcmp(directorio->d_name, ".")) || (!strcmp(directorio->d_name, ".."))) continue;
		char* rutaPokemon = getRutaPokemon(rutaPokenests, directorio->d_name);
		PokeNest* pokenest = crearPokenest(rutaPokemon);

		pokenest->listaPokemons = crearPokemons(rutaPokemon, fabrica, directorio->d_name);
		list_add(listaPokenests, pokenest);
	}
	closedir(d);
	//liberar conf_metadata
	return EXIT_SUCCESS;
}

void crearJugadores(t_list * entrenadores, t_list *items) {

	int q, p;
	p = cols;
	q = rows;

	t_entrenador ash;
	ash.id = 35;
	ash.posx = 1;
	ash.posy = 1;
	ash.movVert = 0;
	ash.quantum = 0;
	char obj1[] = {'P', 'C'};
	ash.objetivos = &obj1;
	ash.objetivoActual = 0;
	ash.bloq = false;

	t_entrenador misty;
	misty.id = '@';
	misty.posx = p;
	misty.posy = q;
	misty.movVert = 0;
	misty.quantum = 0;

	list_add(entrenadores, &ash);
	list_add(entrenadores, &misty);

	int i;

	for(i = 0; i < list_size(entrenadores); i++) {

		t_entrenador *personaje = list_get(entrenadores, i);
		CrearPersonaje(items, personaje -> id, personaje -> posx, personaje -> posy);
	}
}
void moverJugadores(t_list * entrenadores, t_list *items)
{
	int i = 0;

	while(1)
	{
		t_entrenador *personaje = list_get(entrenadores, i);

		while(personaje -> quantum < QUANTUM)
		{
			sleep(1);
			moverJugador(personaje, items, 25, 5);
			personaje -> quantum++;

			char o[2];
			o[0] = personaje -> id;

			nivel_gui_dibujar(items, 0);
		}

		personaje -> quantum = 0;

		i++;
		if(i == list_size(entrenadores)) i = 0;

	}
}

void moverJugador(t_entrenador *personaje, t_list *items, int x, int y) {

	if(personaje-> posx < x && !(personaje->movVert)) {
		personaje->posx < x ? personaje->posx++ : personaje->posx--;

		if(personaje->posy != y) personaje->movVert = 1;
	}
	else if(personaje->posy != y && personaje->movVert) {

		personaje->posy < y ? personaje->posy++ : personaje->posy--;

		if(personaje ->posx !=x) personaje->movVert=0;
	}
	else personaje->movVert = !personaje->movVert;

	MoverPersonaje(items, personaje -> id, ((*personaje).posx), ((*personaje).posy));

}

void darRecurso(t_entrenador * entrenador, PokeNest * poke, t_list * items) {


	if(poke -> cantidad > 0) {
		poke -> cantidad--;
		entrenador ->objetivoActual++;
		restarRecurso(items, poke->id);
	}
	else if(poke ->cantidad == 0) {
		queue_push(poke -> colaBloqueados, (t_entrenador *)entrenador);
		entrenador->bloq= true;
	}
}

void manejar_select(int socket, t_log* log){
	fd_set lectura, master;
	int nuevaConexion, a, recibido, fdMax;
	char buf[512];
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
						FD_SET(nuevaConexion, &master);
						if(nuevaConexion > fdMax) fdMax = nuevaConexion;
					}else {
						recibido = recv(a,  (void*) buf, 512, 0);
						if(recibido <= 0){
							if(recibido < 0){
								log_error(log, "Error al recibir de %d", a);
								printf("error");
								} else {
								log_error(log, "Se desconecto %d", a);
								printf("Se desconecto alguien\n");
								}
							close(a);
							FD_CLR(a, &master);
							} else {
								log_trace(log, "Me estan hablando sin permiso");
								printf("Que mandas?? Estas flashiando\n");
							}
						}
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
	conf_metadata->retardo = config_get_int_value(configuracion, "retardo");
	conf_metadata->quantum = config_get_int_value(configuracion, "quantum");
	config_destroy(configuracion);
}

char* getRutaMetadata(char* ptoMnt, char* nombreMapa){
	int letrasPto = strlen(ptoMnt);
	int letrasNombre = strlen(nombreMapa);
	char* directorio = strdup("/metadata");
	int x = strlen(directorio);
	char* ruta = malloc(letrasPto + letrasNombre + x + 3);
	snprintf(ruta, letrasPto + letrasNombre + x + 3, "%s/%s%s", ptoMnt, nombreMapa, directorio);
	return ruta;
}

char* getRutaPokenests(char* ptoMnt, char* nombreMapa){
	int letrasPto = strlen(ptoMnt);
	int letrasNombre = strlen(nombreMapa);
	char* directorio = strdup("/PokeNests");
	int x = strlen(directorio);
	char* ruta = malloc(letrasPto + letrasNombre + x + 3);
	snprintf(ruta, letrasPto + letrasNombre + x + 3, "%s/%s%s", ptoMnt, nombreMapa, directorio);
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
