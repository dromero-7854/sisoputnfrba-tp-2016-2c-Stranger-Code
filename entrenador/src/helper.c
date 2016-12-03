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

t_coach* cargar_metadata(char* path, char* nombre_entrenador){
	char *pathConfig = string_new();
	string_append(&pathConfig, path);
	string_append(&pathConfig, "Entrenadores/");
	string_append(&pathConfig, nombre_entrenador);
	string_append(&pathConfig, "/metadata");

	t_config* configuracion = config_create(pathConfig);
	char* name = config_get_string_value(configuracion, "nombre");
	char* simbol = config_get_string_value(configuracion, "simbolo");
	int life = config_get_int_value(configuracion, "vidas");
	t_coach* entrenador = coach_create(name, simbol, life);

	// se obtienen los mapas de la hoja de viaje
	char** hojaDeViaje = config_get_array_value(configuracion, "hojaDeViaje");
	char** objetivosDelMapa;
	char** infoMapa;
	char *mapa;
	char *info;
	t_map* map;
	char* ip = "127.0.0.1";
	char* port = "6667";

	int posObj;
	int posMapa = 0;
	while(hojaDeViaje[posMapa] != NULL){
		mapa = string_new();
		string_append(&mapa, "obj[");
		string_append(&mapa, hojaDeViaje[posMapa]);
		string_append(&mapa, "]");

		info = string_new();
		string_append(&info, "info[");
		string_append(&info, hojaDeViaje[posMapa]);
		string_append(&info, "]");

		// se obtiene la información necesaria para conectarse al mapa
		infoMapa = config_get_array_value(configuracion, info);

		// se obtienen los objetivos de un mapa
		objetivosDelMapa = config_get_array_value(configuracion, mapa);
		map = map_create(hojaDeViaje[posMapa], infoMapa[INFO_IP], infoMapa[INFO_PORT]);
		posObj = 0;
		//TODO: se debe validar que no haya dos pokemones iguales sucesivamente
		while(objetivosDelMapa[posObj] != NULL){
			// se agregan los objetivos a un mapa
			list_add(map->pokemon_list, pokemon_create("desconocido", objetivosDelMapa[posObj]));
		posObj++;
		}
		// se agrega el mapa a la hoja de viaje
		list_add(entrenador->travel_sheet, map);
	posMapa++;
	}

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

int completar_mapa(t_log* logger, t_map* mapa, t_coach* entrenador, char* pathPokedex){
	log_info(logger, "Comenzando a jugar en el mapa: %s\n", mapa->name);
	t_pokemon* pokemon = map_next_pokemon(mapa);
	while(pokemon != NULL){
		log_info(logger, "Pidiendo ubicacion de PokeNest: %s", pokemon->simbol);
		map_locate_pokemon(mapa, pokemon, entrenador->conn);
		log_info(logger, "Ubicacion de PokeNest '%s'. En la posición: X->%d, Y->%d", pokemon->simbol, pokemon->coor->x, pokemon->coor->y);
		log_info(logger, "Moviendo a %s hasta PokeNest '%s'", entrenador->name, pokemon->simbol);
		coach_move_to_pokemon(entrenador, pokemon);
		log_info(logger, "Capturando a %s...", pokemon->name);
		coach_capture_pokemon(entrenador, pokemon);
		log_info(logger, "Capturaste a %s! En la posición: X->%d, Y->%d\n", pokemon->name, pokemon->coor->x, pokemon->coor->y);

		pokemon = map_next_pokemon(mapa);
	}
	coach_medal_copy(entrenador, mapa, pathPokedex);
	log_info(logger, "Felicitaciones! completaste el mapa: %s.\n", mapa->name);

	return 0;
}

/*int ubicar(){
	//realizar pedido al mapa de la ubicacion del pokemon

	return 0;
}*/

uint8_t move_to(uint8_t movement, t_coach* entrenador){
	char move[10];
	t_coor* coorEntrenador;
	uint8_t operation_code;
	uint8_t* mov = malloc( sizeof(uint8_t) );
	*mov = movement;

	switch (movement) {
		case MOVE_UP:
			sprintf(move, "ARRIBA");
			//entrenador->coor.y--;
			break;
		case MOVE_DOWN:
			sprintf(move, "ABAJO");
			//entrenador->coor.y++;
			break;
		case MOVE_RIGHT:
			sprintf(move, "DERECHA");
			//entrenador->coor.x++;
			break;
		case MOVE_LEFT:
			sprintf(move, "IZQUIERDA");
			//entrenador->coor.x--;
			break;
	}

	connection_send(entrenador->conn, OC_AVANZAR_POSICION, mov);
	connection_recv(entrenador->conn, &operation_code, &coorEntrenador);
	free(mov);

	entrenador->coor->x = coorEntrenador->x;
	entrenador->coor->y = coorEntrenador->y;

	return 0;
	//log_info(logger, "Movimiento del Entrenador: %s", move);
}

uint8_t calcular_movimiento(uint8_t lastMovement, t_coor* coor_entrenador, t_coor* coor_pokemon){
	uint8_t mover;

	bool moverHorizontalmente = ((lastMovement == MOVE_UP || lastMovement == MOVE_DOWN) && coor_entrenador->x != coor_pokemon->x) || coor_entrenador->y == coor_pokemon->y;
//bool moverVerticalmente = !moverHorizontalmente;coor_entrenador->x != coor_pokemon->x
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
	//while((c=fgetc(fp_org)) != EOF && !ferror(fp_org) && !ferror(fp_dest)) fputc(c, fp_dest);

	if(ferror(fp_org) || ferror(fp_org)){
		return 1;
	} else {
		fclose(fp_org);
		fclose(fp_dest);
		return 0;
	}
}
