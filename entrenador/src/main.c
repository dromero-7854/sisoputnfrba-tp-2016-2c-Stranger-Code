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
#include <signal.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "helper.h"
#include "Entrenador/entrenador.h"

t_log* logger;
t_coach* entrenador;
int reintentos;

int main(int argc, char** argv){
	if(argc!=3) {
		printf("Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./entrenador <nombre_entrenador> <ruta_punto_montaje>\n");
		exit(1);
	}

	//redirigo cada señal al manejador de señales
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGINT, signal_handler);

	char* nombreEntrenador = argv[1];
	char* pathConfig = argv[2];
	reintentos = 0;

	string_capitalized(nombreEntrenador);
	logger = crear_log(nombreEntrenador, pathConfig);

	log_info(logger, "Cargando archivo de metadata: %s", pathConfig);
	entrenador = cargar_metadata(pathConfig, nombreEntrenador);
	//log_info(logger, "Vidas del Entrenador: %d\n", entrenador->life);
	log_info(logger, "Archivo de metadata cargado correctamente\n");

	t_map* mapa = coach_next_map(entrenador);
	while(mapa != NULL){
		log_info(logger, "Conectando con el mapa: %s", mapa->name);
		conectar_entrenador_mapa(entrenador, mapa);
		log_info(logger, "Conexión establecida con el mapa: %s", mapa->name);
		completar_mapa(logger, mapa, entrenador, pathConfig);
		log_info(logger, "Desconectando al entrenador del mapa: %s", mapa->name);
		desconectar_entrenador_mapa(entrenador, mapa);
		log_info(logger, "Desconexión éxitosa del mapa: %s\n", mapa->name);

		mapa = coach_next_map(entrenador);
	}
	//coach_medal_copy(entrenador, mapa, pathConfig);
	log_info(logger, "El Entrenador %s ha completado correctamente su Hoja de Viaje.", entrenador->name);

	free_memory();

	return 0;
}

void signal_handler(int signal) {
	switch (signal) {
	case SIGTERM:
		entrenador->life--;
		if (entrenador->life > 0) {
			log_info(logger, "El entrenador ha perdido una vida. Cantidad actual de vidas: %d \n", entrenador->life);
		} else
			noLeQuedanMasVidas();
		break;
	case SIGUSR1:
		entrenador->life++;
		log_info(logger, "El entrenador ha ganado una vida extra. Cantidad actual de vidas: %d \n", entrenador->life);
		break;
	case SIGINT:
		log_info(logger, "Finalizando el juego...\n");
		free_memory();
		break;
	default:
		// no hay que hacer nada
		break;
	}
}

void free_memory(){
	coach_destroy(entrenador);
	log_destroy(logger);
}
