/*
 * helper.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "helper.h"

t_log* crear_log(char* nombreEntrenador, char* pathConfig) {
	t_log* logger;

	char temp_file[255];
	strcpy(temp_file, "entrenador_");
	strcat(temp_file, nombreEntrenador);
	strcat(temp_file, ".log");
	logger = log_create(temp_file, "TEST", true, LOG_LEVEL_INFO);
	log_info(logger, "Nombre del Entrenador: %s", nombreEntrenador);
	log_info(logger, "Metadata del Entrenador: %s", pathConfig);
	log_info(logger, "Log del Entrenador: %s", temp_file);

	return logger;
}

t_coach* cargar_metadata(char* pathPokedex, char* nombre_entrenador){
	char *pathConfigEntrenador;
	char *pathConfigMapa;
	t_config* configEntrenador;
	t_config* configMapa;

	pathConfigEntrenador = string_from_format("%s/Entrenadores/%s/metadata", pathPokedex, nombre_entrenador);
	configEntrenador = config_create(pathConfigEntrenador);
	char* name = config_get_string_value(configEntrenador, "nombre");
	char* simbol = config_get_string_value(configEntrenador, "simbolo");
	int life = config_get_int_value(configEntrenador, "vidas");
	t_coach* entrenador = coach_create(name, simbol, life);

	// se obtienen los mapas de la hoja de viaje
	char** hojaDeViaje = config_get_array_value(configEntrenador, "hojaDeViaje");
	char** objetivosDelMapa;
	char* ip;
	char* port;
	char* objMapa;
	t_map* map;


	int posObj;
	int posMapa = 0;
	while(hojaDeViaje[posMapa] != NULL){
		objMapa = string_from_format("obj[%s]", hojaDeViaje[posMapa]);

		// se obtiene la información necesaria para conectarse al objMapa
		pathConfigMapa = string_from_format("%s/Mapas/%s/metadata", pathPokedex, hojaDeViaje[posMapa]);
		configMapa = config_create(pathConfigMapa);
		ip = config_get_string_value(configMapa, "IP");
		port = config_get_string_value(configMapa, "PUERTO");

		// se obtienen los objetivos de un mapa
		objetivosDelMapa = config_get_array_value(configMapa, objMapa);
		map = map_create(hojaDeViaje[posMapa], ip, port);
		posObj = 0;
		//TODO: se debe validar que no haya dos pokemones iguales sucesivamente
		while(objetivosDelMapa[posObj] != NULL){
			// se agregan los objetivos a un objMapa
			list_add(map->pokemon_list, pokemon_create("desconocido", objetivosDelMapa[posObj]));
		posObj++;
		}
		// se agrega el objMapa a la hoja de viaje
		list_add(entrenador->travel_sheet, map);

		// libero memoria
		free(objMapa);
		free(pathConfigMapa);
		free(ip);
		free(port);
		config_destroy(configMapa);
	posMapa++;
	}

	free(pathConfigEntrenador);
	config_destroy(configEntrenador);
	return entrenador;
}

int conectar_entrenador_mapa(t_coach* entrenador, t_map* mapa){
	coach_connect_to_map(entrenador, mapa);
	uint8_t operation_code;
	t_coor* coor;
	//connection_send(entrenador->conn, OC_UBICAR_ENTRENADOR, entrenador->name);
	connection_send(entrenador->conn, OC_UBICAR_ENTRENADOR, entrenador->simbol);
	connection_recv(entrenador->conn, &operation_code, &coor);

	entrenador->coor = coor;

	return 0;
}

int desconectar_entrenador_mapa(t_coach* entrenador, t_map* mapa){
	connection_destroy(entrenador->conn);
	return 0;
}

int completar_mapa(t_log* logger, t_map* mapa, t_coach* entrenador){
	log_info(logger, "Comenzando a jugar en el mapa: %s\n", mapa->name);
	t_pokemon* pokemon = map_next_pokemon(mapa);
	while(pokemon != NULL){
		log_info(logger, "Pidiendo ubicacion de PokeNest: %s", pokemon->simbol);
		map_locate_pokemon(mapa, pokemon, entrenador->conn);
		log_info(logger, "Ubicacion de PokeNest '%s'. En la posición: X->%d, Y->%d", pokemon->simbol, pokemon->coor->x, pokemon->coor->y);
		log_info(logger, "Moviendo a %s hasta PokeNest '%s'", entrenador->name, pokemon->simbol);
		coach_move_to_pokemon(entrenador, pokemon);
		log_info(logger, "Capturando a %s...", pokemon->simbol);
		coach_capture_pokemon(entrenador, pokemon);
		log_info(logger, "Capturaste a %s! En la posición: X->%d, Y->%d\n", pokemon->simbol, pokemon->coor->x, pokemon->coor->y);

		pokemon = map_next_pokemon(mapa);
	}
	coach_medal_copy(entrenador, mapa);
	log_info(logger, "Felicitaciones! completaste el mapa: %s.\n", mapa->name);

	return 0;
}

uint8_t move_to(uint8_t movement, t_coach* entrenador){
	char move[10];
	t_coor* coorEntrenador;
	uint8_t operation_code;
	uint8_t* mov = malloc( sizeof(uint8_t) );
	*mov = movement;

	connection_send(entrenador->conn, OC_AVANZAR_POSICION, mov);
	connection_recv(entrenador->conn, &operation_code, &coorEntrenador);
	free(mov);

	entrenador->coor->x = coorEntrenador->x;
	entrenador->coor->y = coorEntrenador->y;

	return 0;
}

uint8_t calcular_movimiento(uint8_t lastMovement, t_coor* coor_entrenador, t_coor* coor_pokemon){
	uint8_t mover;

	bool moverHorizontalmente = ((lastMovement == MOVE_UP || lastMovement == MOVE_DOWN) && coor_entrenador->x != coor_pokemon->x) || coor_entrenador->y == coor_pokemon->y;

	if(moverHorizontalmente){
		if(coor_entrenador->x < coor_pokemon->x) mover = MOVE_RIGHT; else mover = MOVE_LEFT;
	}else{
		if(coor_entrenador->y < coor_pokemon->y) mover = MOVE_DOWN; else mover = MOVE_UP;
	}

	return mover;
}

int copy_file(char* f_origen,char* f_destino){
	FILE *fp_org,*fp_dest;
	char c;

	if(!(fp_org=fopen(f_origen,"rb")) || !(fp_dest=fopen(f_destino,"wb")))
	{
		perror("Error de apertura de ficheros");
		exit(EXIT_FAILURE);
	}

	/* Para meter lo que vamos leyendo del fichero */
	char buffer[1000];
	/* Para guardar el número de items leidos o si ha habido error */
	int totalLeidos;
	int leidos = 0;
	leidos = fread (buffer, 1, 1000, fp_org);

	/* Mientras se haya leido algo ... */
	while (leidos!=0)
	{
		totalLeidos = totalLeidos + leidos;
	   /* ... meterlo en el fichero destino */
	   fwrite (buffer, 1, leidos, fp_dest);
	   /* y leer siguiente bloque */
	   leidos = fread (buffer, 1, 1000, fp_org);
	}

	if(ferror(fp_org) || ferror(fp_org)){
		return 1;
	} else {
		fclose(fp_org);
		fclose(fp_dest);
		return 0;
	}
}
