/*
 * main.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
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

void iniciar_ruta_de_viaje(t_coach* entrenador);
void game_over(void);
int zero_lives(void);

t_log* logger;
t_coach* entrenador;
int reintentos;
char* nombreEntrenador;
int life;
char* pathPokedex;
char* pathMedallas;
char* pathDirDeBill;
time_t begin_time;
time_t end_time;
double adventure_time;
int death_count;

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
	log_info(logger, "Finalizando el juego...");
	coach_destroy(entrenador);

	//deleteDir(pathDirDeBill);
	//deleteDir(pathMedallas);

	//free(pathDirDeBill);
	//free(pathMedallas);
	/*free(nombreEntrenador);
	free(pathPokedex);*/
	log_info(logger, "Juego finalizado.\n");
	log_destroy(logger);
	exit(EXIT_SUCCESS);
}

int zero_lives() {
	char respuesta[2];
	int aux_countDeadlock;
	double aux_pokenestTime;

	t_map* current_map = list_get(entrenador->travel_sheet, entrenador->index_current_map);
	desconectar_entrenador_mapa(entrenador, current_map);

	printf("El entrenador se ha quedado sin vidas. Ya se han realizado %d reintentos. Desea reiniciar el juego? (y/n) ", reintentos);
	fgets(respuesta, 2, stdin);

	/*while( strcmp(respuesta,"y\n") && strcmp(respuesta,"n\n")){
		printf("Por favor, ingrese 'y' o 'n'\n");
		fgets(respuesta, 2, stdin);
	}*/
	if (true/*!strcmp(respuesta,"y")*/) {
		reintentos++;
		deleteDir(pathDirDeBill);
		deleteDir(pathMedallas);
		//guardo el valor de estas variables antes de perderlas
		aux_countDeadlock = entrenador->count_deadlock;
		aux_pokenestTime = entrenador->pokenest_time;
		coach_destroy(entrenador);
		log_info(logger, "Volviendo a cargar archivo de metadata: %s", pathPokedex);
		entrenador = cargar_metadata(logger, pathPokedex, nombreEntrenador);
		log_info(logger, "Archivo de metadata cargado correctamente\n");
		entrenador->count_deadlock = aux_countDeadlock;
		entrenador->pokenest_time = aux_pokenestTime;
		log_info(logger, "Creando Directorio de Bill...");
		createDir(pathDirDeBill);
		log_info(logger, "Creando Directorio de Medallas...");
		createDir(pathMedallas);

		coach_next_map(entrenador);
		iniciar_ruta_de_viaje(entrenador);
		return EXIT_SUCCESS;
	} else {
		game_over();
		return EXIT_FAILURE;
	}
}

void iniciar_ruta_de_viaje(t_coach* entrenador){
	int oc;
	t_map* mapa = list_get(entrenador->travel_sheet, entrenador->index_current_map);
	while(mapa != NULL){
		log_info(logger, "Conectando con el mapa: %s", mapa->name);
		conectar_entrenador_mapa(entrenador, mapa);
		log_info(logger, "Conexión establecida con el mapa: %s", mapa->name);
		oc = completar_mapa(logger, mapa, entrenador, pathPokedex);
		log_info(logger, "Desconectando al entrenador del mapa: %s", mapa->name);
		desconectar_entrenador_mapa(entrenador, mapa);
		deleteDir(pathDirDeBill);
		createDir(pathDirDeBill);
		log_info(logger, "Desconexión éxitosa del mapa: %s\n", mapa->name);

		if(oc == OC_VICTIMA_DEADLOCK){
			//se le descuenta una vida
			entrenador->life--;
			//se incrementa la cantidad de veces que murió
			death_count++;
			//se incrementa la cantidad de deadlocks
			entrenador->count_deadlock++;
			// se reinician los objetivos del mapa
			mapa->index_current_pokemon = -1;
			log_info(logger, "El entrenador %s ha muerto por Deadlock\n", entrenador->name);
			// si me quedan vidas borro los pokemones capturados y no avanzo el mapa para volver a intentar en el mismo
			if(entrenador->life > 0){
				deleteDir(pathDirDeBill);
				createDir(pathDirDeBill);
			}else{
			// si no me quedan vidas salgo del while para preguntar por reintento?
				break;
			}
		}else{
			mapa = coach_next_map(entrenador);
		}
	}
}

int main(int argc, char** argv){
	if(argc!=3) {
		printf("Faltan ingresar parámetos. Se debe ejecutar de la sig. manera:\n ./entrenador <nombre_entrenador> <ruta_punto_montaje>\n");
		exit(1);
	}

	//redirigo cada señal al manejador de señales
	signal(SIGUSR1, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	nombreEntrenador = argv[1];
	pathPokedex = argv[2];
	reintentos = 0;
	death_count = 0;

	string_capitalized(nombreEntrenador);
	logger = crear_log(nombreEntrenador, pathPokedex);

	pathMedallas = string_from_format("%sEntrenadores/%s/medallas", pathPokedex, nombreEntrenador);
	pathDirDeBill = string_from_format("%sEntrenadores/%s/Dir de Bill/", pathPokedex, nombreEntrenador);

	log_info(logger, "Creando Directorio de Bill...");
	createDir(pathDirDeBill);
	log_info(logger, "Creando Directorio de Medallas...");
	createDir(pathMedallas);

	log_info(logger, "Cargando archivo de metadata: %s", pathPokedex);
	entrenador = cargar_metadata(logger, pathPokedex, nombreEntrenador);
	life = entrenador->life;
	log_info(logger, "Archivo de metadata cargado correctamente\n");

	coach_next_map(entrenador);
	begin_time = time(NULL);
	log_info(logger, "Comenzando la aventura :)");
	iniciar_ruta_de_viaje(entrenador);
	if(entrenador->life < 1){
		/*entrenador->index_current_map = 0;
		entrenador->life = life;*/
		zero_lives();
	}

	end_time = time(NULL);
	adventure_time = difftime(end_time, begin_time);

	log_info(logger, "El Entrenador %s ha completado correctamente su Hoja de Viaje.\n", entrenador->name);

	log_info(logger, "El Entrenador %s se ha convertido en Maestro Pokémon!", entrenador->name);
	log_info(logger, "Tiempo total del viaje: %d segundos", adventure_time);
	log_info(logger, "Tiempo bloqueado en las PokeNests: %d segundos", entrenador->pokenest_time);
	log_info(logger, "Cantidad de deadlocks: %d", entrenador->count_deadlock);
	log_info(logger, "Cantidad de veces muerto: %d", death_count);

	game_over();

	return 0;
}
