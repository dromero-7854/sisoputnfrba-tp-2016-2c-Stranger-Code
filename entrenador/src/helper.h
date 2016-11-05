/*
 * helper.h
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */

#ifndef SRC_HELPER_H_
#define SRC_HELPER_H_

#include <stdbool.h>
#include <stdint.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "Pokemon/pokemon.h"
#include "Mapa/mapa.h"
#include "Coordenadas/coordenadas.h"
#include "Entrenador/entrenador.h"
#include "Conexion/conexion.h"

t_log* crear_log(char* nombreEntrenador, char* pathConfig);
int cargar_metadata(char* path, t_list* travel_sheet);
int conectar_entrenador_mapa(t_coach* entrenador, t_map* mapa);
int desconectar_entrenador_mapa(t_coach* entrenador, t_map* mapa);
int completar_mapa(t_log* logger, t_map* mapa, t_coach* entrenador);
uint8_t move_to(uint8_t movement, t_coach* entrenador);
uint8_t calcular_movimiento(uint8_t lastMovement, t_coor* coor_entrenador, t_coor* coor_pokemon);

#endif /* SRC_HELPER_H_ */
