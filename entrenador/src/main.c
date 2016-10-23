/*
 * main.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "helper.h"
#include "Entrenador/entrenador.h"

t_log* logger;

int main(int argc, char** argv){
	if(argc!=3) {
		printf("Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./entrenador <nombre_entrenador> <ruta_archivo_metadata>\n");
		exit(1);
	}

	int life = 3;
	char* nombreEntrenador = argv[1];
	char* pathConfig = argv[2];

	t_coach* entrenador = coach_create(nombreEntrenador, life);
	logger = crear_log(nombreEntrenador, pathConfig);

	log_info(logger, "Datos Entrenador: \n%s \n%d \n", entrenador->name, entrenador->life);

	log_info(logger, "Cargando archivo de metadata: %s", pathConfig);
	cargar_metadata(pathConfig, entrenador->travel_sheet);
	log_info(logger, "Archivo de metadata cargado correctamente.");

	t_map* mapa = coach_next_map(entrenador);
	while(mapa != NULL){
		log_info(logger, "Conectando con el mapa: %s", mapa->name);
		conectar_entrenador_a_mapa(entrenador, mapa);
		log_info(logger, "Conexión establecida con el mapa: %s", mapa->name);
		completar_mapa(logger, mapa, entrenador);
		//desconectar_mapa(current_mapa);
		mapa = coach_next_map(entrenador);
	}

	//list_iterate(entrenador->travel_sheet,(void*) for_each_map);

	coach_destroy(entrenador);
	log_destroy(logger);
	return 0;
}
/*
int hola(int argc, char** argv){
	if(argc!=3) {
		printf("Hola! Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./Entrenador <nombre_entrenador> <ruta_archivo_metadata>\n");
		exit(1);
	}

	int life = 3;
	char* nombreEntrenador = argv[1];
	char* pathConfig = argv[2];

	t_coach entrenador = new_coach(nombreEntrenador, life);

	t_mapa *current_map;
	char temp_file[255];

	strcpy(temp_file, "entrenador_");
	strcat(temp_file, nombreEntrenador);
	strcat(temp_file, ".log");

	logger = log_create(temp_file, "TEST", true, LOG_LEVEL_INFO);
	log_info(logger, "Nombre del Entrenador: %s", nombreEntrenador);
	log_info(logger, "Metadata del Entrenador: %s", pathConfig);
	log_info(logger, "Log del Entrenador: %s\n", temp_file);

	leer_metadata(pathConfig);

	int pos = 0;
	while(list_size(maps_list) > pos){
		current_map = list_get(maps_list, pos);
		conectar_mapa(current_map);
		completar_mapa(current_map);
		//desconectar_mapa(current_mapa);
		pos++;
	}

	//liberar memoria
	destroy_maps_list();
    log_destroy(logger);

	return 0;
}*/
