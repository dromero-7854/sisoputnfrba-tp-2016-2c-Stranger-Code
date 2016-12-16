/*
 * entrenador.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_ENTRENADOR_ENTRENADOR_H_
#define SRC_ENTRENADOR_ENTRENADOR_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "../Coordenadas/coordenadas.h"
#include "../Mapa/mapa.h"
#include "../Conexion/conexion.h"
#include "../helper.h"

#define SOLICITAR_UBICACION 0;
#define SOLICITAR_AVANZAR 1;
#define SOLICITAR_ATRAPAR 2;

typedef struct {
	char* name ;
	char *simbol;
	int life;
	double pokenest_time;
	t_coor* coor;
	t_list* pokemons;
	t_list* travel_sheet;
	int index_current_map;
	int count_deadlock;
	t_connection* conn;
} t_coach;

t_coach *coach_create(char* nombreEntrenador, char* simbol, int life);
void coach_destroy(t_coach* self);
t_map* coach_next_map(t_coach* self);
int coach_move_to_pokemon(t_coach* self, t_pokemon* pokemon);
int coach_capture_pokemon(t_coach* self, t_pokemon* pokemon, char* pathPokedex, uint8_t oc);
int coach_capture_last_pokemon(t_coach* self, t_pokemon* pokemon, char* pathPokedex);
void coach_connect_to_map(t_coach* self, t_map* mapa);
void coach_medal_copy(t_coach* self, t_map* mapa, char* pathPokedex);
int conectar_entrenador_mapa(t_coach* entrenador, t_map* mapa);
int handshake(t_coach* entrenador, t_map* mapa);
int desconectar_entrenador_mapa(t_coach* entrenador, t_map* mapa);
int completar_mapa(t_log* logger, t_map* mapa, t_coach* entrenador, char* pathPokedex);
uint8_t move_to(uint8_t movement, t_coach* entrenador);
uint8_t calcular_movimiento(uint8_t lastMovement, t_coor* coor_entrenador, t_coor* coor_pokemon);
t_coach* cargar_metadata(t_log* logger, char* path, char* nombre_entrenador);

#endif /* SRC_ENTRENADOR_ENTRENADOR_H_ */
