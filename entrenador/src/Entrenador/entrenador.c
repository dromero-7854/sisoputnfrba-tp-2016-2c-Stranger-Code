/*
 * entrenador.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "entrenador.h"

//TODO se declara acá, pero se implementa en el main.c
void game_over(void);

t_coach *coach_create(char *name, char *simbol, int life){
	t_coach *new = malloc( sizeof(t_coach) );
	new->name = strdup(name);
	new->simbol = strdup(simbol);
	new->life = life;
	//new->coor; = coor_create(0, 0);
	new->pokemons = list_create();
	new->travel_sheet = list_create();
	new->index_current_map = -1;
	new->pokenest_time = 0;
	new->count_deadlock = 0;

	return new;
}

void coach_destroy(t_coach *self){
	//if (self->conn->socket) connection_destroy(self->conn);
	free(self->name);
	free(self->simbol);
	if(!list_is_empty(self->pokemons)) list_clean(self->pokemons);
	free(self->pokemons);
	free(self->coor);
	destroy_maps_list(self->travel_sheet);
	/*if(!list_is_empty(self->travel_sheet)) list_clean(self->travel_sheet);
	free(self->travel_sheet);*/

	free(self);
}

t_map* coach_next_map(t_coach* self){
	self->index_current_map++;
	if(list_size(self->travel_sheet) > self->index_current_map){
		t_map* current_map = list_get(self->travel_sheet, self->index_current_map);
		return current_map;
	}else{
		return NULL;
	}
}

int coach_move_to_pokemon(t_coach* entrenador, t_pokemon* pokemon){
	uint8_t last_movement = MOVE_RIGHT;
	while( !coor_equals(entrenador->coor, pokemon->coor) ){
		last_movement = calcular_movimiento(last_movement, entrenador->coor, pokemon->coor);

		move_to(last_movement, entrenador);
	}

	return 0;
}

bool esDeMayorNivel(t_pokemon* pokemon1, t_pokemon* pokemon2) {
	return pokemon1->level > pokemon2->level;
}

void coach_pokemon_battle(t_coach* entrenador){
	list_sort(entrenador->pokemons, (void*) esDeMayorNivel);

	t_pokemon* pokemon = list_get(entrenador->pokemons, 0);
	connection_send(entrenador->conn, OC_POKEMON, pokemon->name);
}

int coach_capture_pokemon(t_coach* entrenador, t_pokemon* pokemon, char* pathPokedex, uint8_t oc){
	uint8_t operation_code;
	char* pathDirDeBillOrigen;
	char* pathDirDeBillDestino;
	char** arrayPath;
	char* nombreArchivo;
	time_t beginTime;
	time_t endTime;

	beginTime = time(NULL);
	connection_send(entrenador->conn, oc, pokemon->simbol);
	do {
		//TODO se castea a void** ya que es lo que espera la función. Es para evitar el warning
		connection_recv(entrenador->conn, &operation_code, (void**)&pathDirDeBillOrigen);
		if(operation_code == OC_POKEMON_BATALLA) coach_pokemon_battle(entrenador);
		if(operation_code == OC_GANO_BATALLA) entrenador->count_deadlock++;
	} while (operation_code != OC_POKEMON && operation_code != OC_VICTIMA_DEADLOCK);

	endTime = time(NULL);
	entrenador->pokenest_time = entrenador->pokenest_time + difftime(beginTime, endTime);
	if(operation_code == OC_VICTIMA_DEADLOCK) return operation_code;

	arrayPath = string_split(pathDirDeBillOrigen, "/");
	int posArray = 0;
	while (arrayPath[posArray] != NULL) {
		posArray++;
	}
	posArray--;
	nombreArchivo = arrayPath[posArray];

	pathDirDeBillOrigen = string_from_format("%s%s", pathPokedex, pathDirDeBillOrigen);
	pathDirDeBillDestino = string_from_format("%sEntrenadores/%s/Dir de Bill/%s", pathPokedex, entrenador->name, nombreArchivo);

	int resultado = copy_file(pathDirDeBillOrigen, pathDirDeBillDestino);

	if(resultado){
		printf("El archivo no se pudo copiar\n");
		game_over();
	} else {
		printf("Archivo copiado exitosamente\n");
	}

	pokemon->name = strdup(nombreArchivo);
	t_config* config = config_create(pathDirDeBillDestino);
	pokemon->level = config_get_int_value(config, "Nivel");

	// agrego el pokemon a la lista
	list_add(entrenador->pokemons, pokemon);

	free(arrayPath);
	free(pathDirDeBillOrigen);
	free(pathDirDeBillDestino);

	return operation_code;
}

int coach_capture_last_pokemon(t_coach* entrenador, t_pokemon* pokemon, char* pathPokedex){
	uint8_t operation_code;
	//uint8_t* cant_deadlock = malloc( sizeof(uint8_t) );
	char* pathDirDeBillOrigen;
	char* pathDirDeBillDestino;
	char** arrayPath;
	char* nombreArchivo;
	time_t beginTime;
	time_t endTime;

	beginTime = time(NULL);
	connection_send(entrenador->conn, OC_ATRAPAR_ULTIMO_POKEMON, pokemon->simbol);
	connection_recv(entrenador->conn, &operation_code, (void**)&pathDirDeBillOrigen);
	endTime = time(NULL);
	entrenador->pokenest_time = entrenador->pokenest_time + difftime(endTime, beginTime);
	if(operation_code == OC_VICTIMA_DEADLOCK) return operation_code;
	// como se envió el pedido de capturar al ultimo pokemon, ahora se envia la petición
	// para saber los deadlocks en los que estuvo involucrado este entrenador
	/*connection_send(entrenador->conn, OC_OBTENER_CANTIDAD_DEADLOCK, "");
	connection_recv(entrenador->conn, &operation_code, &cant_deadlock);
	entrenador->count_deadlock = entrenador->count_deadlock + *cant_deadlock;
	free(cant_deadlock);
	if(operation_code == OC_VICTIMA_DEADLOCK) return operation_code;*/

	//TODO extraer en una funcion ya que se llama tambien en "coach_capture_pokemon()"
	arrayPath = string_split(pathDirDeBillOrigen, "/");
	int posArray = 0;
	while (arrayPath[posArray] != NULL) {
		posArray++;
	}
	posArray--;
	nombreArchivo = arrayPath[posArray];

	pathDirDeBillOrigen = string_from_format("%s%s", pathPokedex, pathDirDeBillOrigen);
	pathDirDeBillDestino = string_from_format("%sEntrenadores/%s/Dir de Bill/%s", pathPokedex, entrenador->name, nombreArchivo);

	int resultado = copy_file(pathDirDeBillOrigen, pathDirDeBillDestino);
	free(arrayPath);
	free(pathDirDeBillOrigen);
	free(pathDirDeBillDestino);

	if(resultado){
		printf("El archivo no se pudo copiar\n");
		game_over();
	} else {
		printf("Archivo copiado exitosamente\n");
	}

	return operation_code;
}

void coach_connect_to_map(t_coach* entrenador, t_map* mapa){
	entrenador->conn = connection_create(mapa->ip, mapa->port);
	if(connection_open(entrenador->conn)){
		printf("\nNo se puedo conectar con el mapa %s\n\n", mapa->name);
		game_over();
	}
}

void coach_medal_copy(t_coach* self, t_map* mapa, char* pathPokedex){
	//uint8_t operation_code;
	char* pathMedallaOrigen;
	char* pathMedallaDestino;
	char** arrayPath;
	char* nombreArchivo;

	//connection_send(self->conn, OC_OBTENER_MEDALLA, "");
	//connection_recv(self->conn, &operation_code, &pathMedallaOrigen);

	arrayPath = string_split(mapa->medal_path, "/");
	int posArray = 0;
	while (arrayPath[posArray] != NULL) {
		posArray++;
	}
	posArray--;
	nombreArchivo = arrayPath[posArray];

	pathMedallaOrigen = string_from_format("%s%s", pathPokedex, mapa->medal_path);
	pathMedallaDestino = string_from_format("%sEntrenadores/%s/medallas/%s", pathPokedex, self->name, nombreArchivo);

	int resultado = copy_file(pathMedallaOrigen, pathMedallaDestino);
	/*free(arrayPath);
	free(mapa->medal_path);
	free(pathMedallaDestino);*/

	if(resultado){
		printf("El archivo no se pudo copiar\n");
		game_over();
	} else {
		printf("Archivo copiado exitosamente\n");
	}
}

int conectar_entrenador_mapa(t_coach* entrenador, t_map* mapa){
	coach_connect_to_map(entrenador, mapa);
	handshake(entrenador, mapa);

	return EXIT_SUCCESS;
}

int handshake(t_coach* entrenador, t_map* mapa){
	uint8_t operation_code;
	t_coor* coor;

	connection_send(entrenador->conn, OC_UBICAR_ENTRENADOR, entrenador->simbol);
	connection_recv(entrenador->conn, &operation_code, (void**)&coor);
	connection_send(entrenador->conn, OC_OBTENER_MEDALLA, entrenador->simbol);
	connection_recv(entrenador->conn, &operation_code, (void**)&mapa->medal_path);
	connection_send(entrenador->conn, OC_MENSAJE, entrenador->name);
	entrenador->coor = coor;

	return EXIT_SUCCESS;
}

int desconectar_entrenador_mapa(t_coach* entrenador, t_map* mapa){
	connection_destroy(entrenador->conn);
	return 0;
}

int completar_mapa(t_log* logger, t_map* mapa, t_coach* entrenador, char* pathPokedex){
	log_info(logger, "Comenzando a jugar en el mapa: %s\n", mapa->name);
	t_pokemon* pokemon = map_next_pokemon(mapa);
	int oc;
	while(pokemon != NULL){
		log_info(logger, "Pidiendo ubicacion de PokeNest: %s", pokemon->simbol);
		map_locate_pokemon(mapa, pokemon, entrenador->conn);
		log_info(logger, "Ubicacion de PokeNest '%s'. En la posición: X->%d, Y->%d", pokemon->simbol, pokemon->coor->x, pokemon->coor->y);
		log_info(logger, "Moviendo a %s hasta PokeNest '%s'", entrenador->name, pokemon->simbol);
		coach_move_to_pokemon(entrenador, pokemon);
		log_info(logger, "Capturando a %s...", pokemon->simbol);
		if(map_is_last_pokemon(mapa)){
			oc = coach_capture_pokemon(entrenador, pokemon, pathPokedex, OC_ATRAPAR_ULTIMO_POKEMON);
		}else{
			oc = coach_capture_pokemon(entrenador, pokemon, pathPokedex, OC_ATRAPAR_POKEMON);
		}
		if(oc == OC_VICTIMA_DEADLOCK){
			return oc;
		}
		log_info(logger, "Capturaste a %s! En la posición: X->%d, Y->%d\n", pokemon->simbol, pokemon->coor->x, pokemon->coor->y);

		pokemon = map_next_pokemon(mapa);
	}
	coach_medal_copy(entrenador, mapa, pathPokedex);
	log_info(logger, "Felicitaciones! completaste el mapa: %s.\n", mapa->name);

	return oc;
}

uint8_t move_to(uint8_t movement, t_coach* entrenador){
	t_coor* coorEntrenador;
	uint8_t operation_code;
	uint8_t* mov = malloc( sizeof(uint8_t) );
	*mov = movement;

	connection_send(entrenador->conn, OC_AVANZAR_POSICION, mov);
	connection_recv(entrenador->conn, &operation_code, (void**)&coorEntrenador);
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

t_coach* cargar_metadata(t_log* logger, char* pathPokedex, char* nombre_entrenador){
	bool errorObjDelMapa = false;
	char *pathConfigEntrenador;
	char *pathConfigMapa;
	t_config* configEntrenador;
	t_config* configMapa;

	pathConfigEntrenador = string_from_format("%sEntrenadores/%s/metadata", pathPokedex, nombre_entrenador);
	configEntrenador = config_create(pathConfigEntrenador);
	//char* name = config_get_string_value(configEntrenador, "nombre");
	char* simbol = config_get_string_value(configEntrenador, "simbolo");
	int life = config_get_int_value(configEntrenador, "vidas");
	t_coach* entrenador = coach_create(nombre_entrenador, simbol, life);

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
		pathConfigMapa = string_from_format("%sMapas/%s/metadata", pathPokedex, hojaDeViaje[posMapa]);
		configMapa = config_create(pathConfigMapa);
		ip = config_get_string_value(configMapa, "IP");
		port = config_get_string_value(configMapa, "Puerto");

		// se obtienen los objetivos de un mapa
		objetivosDelMapa = config_get_array_value(configEntrenador, objMapa);
		map = map_create(hojaDeViaje[posMapa], ip, port);
		posObj = 0;
		//TODO: se debe validar que no haya dos pokemones iguales sucesivamente
		char objAnterior = NULL;
		while(objetivosDelMapa[posObj] != NULL){
			// se agregan los objetivos a un objMapa
			list_add(map->pokemon_list, pokemon_create("desconocido", objetivosDelMapa[posObj]));
			//TODO esto no esta bien. Hay que debugear para ver como arreglarlo y no romperlo en el intento xD
			if(objAnterior != NULL && objAnterior == *objetivosDelMapa[posObj]){
				errorObjDelMapa = true;
			} else {
				objAnterior = *objetivosDelMapa[posObj];
			}
		posObj++;
		}
		// se agrega el objMapa a la hoja de viaje
		list_add(entrenador->travel_sheet, map);

		// libero memoria
		/*free(objMapa);
		free(pathConfigMapa);
		free(ip);
		free(port);
		config_destroy(configMapa);*/
	posMapa++;
	}

	free(pathConfigEntrenador);
	config_destroy(configEntrenador);

	if(errorObjDelMapa){
		log_error(logger, "Archivo de metadata incorrecto debido a objetivos de mapas.");
		game_over();
	}
	return entrenador;
}
