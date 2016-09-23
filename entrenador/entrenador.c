/*
 * entrenador.c
 *
 *  Created on: 20/9/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <bibliotecaCharMander.c>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>
int main(int argc, char* argv[]){
	char* nombre, ptoMontaje;
	char** ciudades;
	int id, cantVidas;
	t_list* hojaDeViaje, objetivosPorMapa;

	t_log* logger= log_create("entrenador.log", "Entrenador",false, LOG_LEVEL_TRACE);

	conectar("127.0.0.1", "33000", logger);


	log_destroy(logger);


	return 0;
}
