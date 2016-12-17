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
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include "Pokemon/pokemon.h"
#include "Mapa/mapa.h"
#include "Coordenadas/coordenadas.h"
#include "Entrenador/entrenador.h"
#include "Conexion/conexion.h"

#define INFO_IP 0
#define INFO_PORT 1

t_log* crear_log(char* nombreEntrenador, char* pathConfig);
int copy_file(char* f_origen, char* f_destino);
void deleteDir(char* path);
void createDir(char* path);

#endif /* SRC_HELPER_H_ */
