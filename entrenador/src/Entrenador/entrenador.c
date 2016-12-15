/*
 * entrenador.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "entrenador.h"

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

	time(&beginTime);
	connection_send(entrenador->conn, oc, pokemon->simbol);
	do {
		connection_recv(entrenador->conn, &operation_code, &pathDirDeBillOrigen);
		if(operation_code == OC_POKEMON_BATALLA) coach_pokemon_battle(entrenador);
		if(operation_code == OC_GANO_BATALLA) entrenador->count_deadlock++;
	} while (operation_code != OC_POKEMON && operation_code != OC_VICTIMA_DEADLOCK);

	time(&endTime);
	entrenador->pokenest_time = entrenador->pokenest_time + difftime(endTime, beginTime);
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

	time(&beginTime);
	connection_send(entrenador->conn, OC_ATRAPAR_ULTIMO_POKEMON, pokemon->simbol);
	connection_recv(entrenador->conn, &operation_code, &pathDirDeBillOrigen);
	time(&endTime);
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
	connection_open(entrenador->conn);
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
