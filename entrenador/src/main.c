/*
 * main.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <signal.h>
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
t_coach* entrenador;
int reintentos;
char* nombreEntrenador;
char* pathPokedex;
char* pathMedallas;
char* pathDirDeBill;

void signal_handler(int signal) {
	switch (signal) {
	case SIGTERM:
		entrenador->life--;
		if (entrenador->life > 0) {
			log_info(logger, "El entrenador ha perdido una vida. Cantidad actual de vidas: %d \n", entrenador->life);
		} else
			zero_lives();
		break;
	case SIGUSR1:
		entrenador->life++;
		log_info(logger, "El entrenador ha ganado una vida extra. Cantidad actual de vidas: %d \n", entrenador->life);
		break;
	case SIGINT:
		game_over();
		break;
	default:
		// no hay que hacer nada
		break;
	}
}

void game_over(){
	log_info(logger, "Finalizando el juego...\n");
	coach_destroy(entrenador);

	deleteDir(pathDirDeBill);
	deleteDir(pathMedallas);

	free(pathDirDeBill);
	free(pathMedallas);
	free(nombreEntrenador);
	free(pathPokedex);
	log_info(logger, "Juego finalizado.");
	log_destroy(logger);
	exit(EXIT_SUCCESS);
}

int zero_lives() {
	printf("El entrenador se ha quedado sin vidas. Desea reiniciar el juego? Ya se han realizado %d reintentos. (y/n)\n", reintentos);
	char respuesta=0;
	fflush(stdin);
	respuesta = fgetc(stdin);
	while(respuesta!='y' && respuesta != 'n'){
		printf("Por favor, ingrese 'y' o 'n'\n");
		fflush(stdin);
		respuesta = fgetc(stdin);
	}
	if (respuesta == 'y') {
		reintentos++;
		deleteDir(pathDirDeBill);
		deleteDir(pathMedallas);
		coach_destroy(entrenador);
		log_info(logger, "Volviendo a cargar archivo de metadata: %s", pathPokedex);
		entrenador = cargar_metadata(logger, pathPokedex, nombreEntrenador);
		log_info(logger, "Archivo de metadata cargado correctamente\n");
		log_info(logger, "Creando Directorio de Bill...");
		createDir(pathDirDeBill);
		log_info(logger, "Creando Directorio de Medallas...");
		createDir(pathMedallas);

		return EXIT_SUCCESS;
	} else {
		game_over();
		return EXIT_FAILURE;
	}
}

int main(int argc, char** argv){
	if(argc!=3) {
		printf("Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./entrenador <nombre_entrenador> <ruta_punto_montaje>\n");
		exit(1);
	}

	//redirigo cada señal al manejador de señales
	signal(SIGUSR1, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	nombreEntrenador = argv[1];
	pathPokedex = argv[2];
	reintentos = 0;

	string_capitalized(nombreEntrenador);
	logger = crear_log(nombreEntrenador, pathPokedex);

	pathMedallas = string_from_format("%s/Entrenadores/%s/medallas", pathPokedex, nombreEntrenador);
	pathDirDeBill = string_from_format("%s/Entrenadores/%s/Dir de Bill/", pathPokedex, nombreEntrenador);

	log_info(logger, "Creando Directorio de Bill...");
	createDir(pathDirDeBill);
	log_info(logger, "Creando Directorio de Medallas...");
	createDir(pathMedallas);

	log_info(logger, "Cargando archivo de metadata: %s", pathPokedex);
	entrenador = cargar_metadata(logger, pathPokedex, nombreEntrenador);
	log_info(logger, "Archivo de metadata cargado correctamente\n");

	t_map* mapa = coach_next_map(entrenador);
	while(mapa != NULL){
		log_info(logger, "Conectando con el mapa: %s", mapa->name);
		conectar_entrenador_mapa(entrenador, mapa);
		log_info(logger, "Conexión establecida con el mapa: %s", mapa->name);
		completar_mapa(logger, mapa, entrenador, pathPokedex);
		log_info(logger, "Desconectando al entrenador del mapa: %s", mapa->name);
		desconectar_entrenador_mapa(entrenador, mapa);
		log_info(logger, "Desconexión éxitosa del mapa: %s\n", mapa->name);

		mapa = coach_next_map(entrenador);
	}
	coach_medal_copy(entrenador, mapa, pathPokedex);
	log_info(logger, "El Entrenador %s ha completado correctamente su Hoja de Viaje.", entrenador->name);

	game_over();

	return 0;
}
