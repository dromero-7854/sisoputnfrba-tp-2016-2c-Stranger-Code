/*
 * Proceso Entrenador:
 *
 * Esta la estructura básica que debe cumplir para recorrer los mapas y capturar los pokemones.
 * Se está desarrollando la parte de la conexion al mapa (servidor)
 */
/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#define IP "127.0.0.1"
#define PUERTO "6667"
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

#define UP 72
#define RIGHT 77
#define DOWN 80
#define LEFT 75

typedef struct {
	int x;
	int y;
} t_coor;

typedef struct {
	char *name;
	char *ip;
	char *port;
	t_list *poke_list;
	int current_poke;
} t_mapa;

typedef struct {
	char *name;
	char *simbol;
	t_coor coor;
} t_poke;

static t_poke *poke_create(char *name, char *simbol){
	t_poke *new = malloc( sizeof(t_poke) );
	new->name = strdup(name);
	new->simbol = strdup(simbol);
	return new;
}

static void poke_destroy(t_poke *self){
	free(self->name);
	free(self);
}

static t_mapa *mapa_create(char *name, char *ip, char *port){
	t_mapa *new = malloc( sizeof(t_mapa) );
	new->name = strdup(name);
	new->ip = strdup(ip);
	new->port = strdup(port);
	new->poke_list = list_create();
	new->current_poke = -1;
	return new;
}

static void mapa_destroy(t_mapa *self){
	free(self->name);
	free(self->ip);
	free(self->port);
	if(!list_is_empty(self->poke_list)) list_clean(self->poke_list);
	free(self->poke_list);
	free(self);
}

t_mapa *find_mapa_by_name(t_list *maps_list, char *name) {
	int _is_the_one(t_mapa *m) {
		return string_equals_ignore_case(m->name, name);
	}

	return list_find(maps_list, (void*) _is_the_one);
}

t_list* maps_list;
t_coor current_coor;
t_log* logger;

int chau(int argc, char** argv){
	if(argc!=3) {
		printf("Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./Entrenador <nombre_entrenador> <ruta_archivo_metadata>\n");
		exit(1);
	}

	char* nombreEntrenador = argv[1];
	char* pathConfig = argv[2];
	t_mapa *current_map;
	char temp_file[255];

	strcpy(temp_file, "entrenador_");
	strcat(temp_file, nombreEntrenador);
	strcat(temp_file, ".log");

	logger = log_create(temp_file, "TEST", true, LOG_LEVEL_INFO);
	log_info(logger, "Nombre del Entrenador: %s", nombreEntrenador);
	log_info(logger, "Metadata del Entrenador: %s", pathConfig);
	log_info(logger, "Log del Entrenador: %s\n", temp_file);

	leer_metadata(pathConfig);

	int pos = 0;
	while(list_size(maps_list) > pos){
		current_map = list_get(maps_list, pos);
		conectar_mapa(current_map);
		completar_mapa(current_map);
		//desconectar_mapa(current_mapa);
		pos++;
	}

	//liberar memoria
	destroy_maps_list();
    log_destroy(logger);

	return 0;
}

int conectar_mapa(t_mapa* current_map){
	log_info(logger, "Conectando con el mapa: %s", current_map->name);
	current_coor.x = 0;
	current_coor.y = 0;
	log_info(logger, "Conexión establecida con el mapa: %s", current_map->name);

	return 0;
}

int leer_metadata(char* path){
	log_info(logger, "Cargando archivo de metadata: %s", path);

	char *ip = "127.0.0.1";
	char *port = "6700";
	t_poke *poke_list;

	maps_list = list_create();
	list_add(maps_list, mapa_create("pueblo paleta", ip, port));
    list_add(maps_list, mapa_create("algun otro pueblo", ip, port));
    list_add(maps_list, mapa_create("el mejor pueblo", ip, port));
    list_add(maps_list, mapa_create("pueblo capital", ip, port));
    list_add(maps_list, mapa_create("pueblo olvidado", ip, port));

	poke_list = find_mapa_by_name(maps_list, "pueblo paleta")->poke_list;
	list_add(poke_list, poke_create("Picachu", "P"));
	list_add(poke_list, poke_create("Raychu", "R"));
	list_add(poke_list, poke_create("Bulbasaur", "B"));

	log_info(logger, "Archivo de metadata cargado correctamente.");
	return 0;
}

t_poke *get_current_poke(t_mapa *mapa){
	return list_get(mapa->poke_list, mapa->current_poke);
}

int completar_mapa(t_mapa *mapa){
	while( mapa->current_poke < list_size(mapa->poke_list)-1 ){
		pedir_ubicacion_pokemon(mapa);
		moverse_hasta_pokemon( get_current_poke(mapa) );
		capturar_pokemon( get_current_poke(mapa) );

		//log pokemon capturado
		t_poke *pokemon = get_current_poke(mapa);
		log_info(logger, "Capturaste a %s! En la posición: X->%d, Y->%d", pokemon->name, pokemon->coor.x, pokemon->coor.y);
	}
	//log de mapa completo
	log_info(logger, "Felicitaciones! completaste el mapa: %s.\n", mapa->name);

	return 0;
}

int capturar_pokemon( t_poke* pokemon ){
	//log_info(logger, "Se ha capturado al pokemon: %s (%d,%d).\n", pokemon->name, current_coor.x, current_coor.y);

	return 0;
}

int pedir_ubicacion_pokemon(t_mapa *mapa){
	mapa->current_poke = mapa->current_poke + 1;
	ubicar( get_current_poke(mapa) );

	return 0;
}

int ubicar(t_poke *poke){
	//realizar pedido al mapa de la ubicacion del pokemon

	//setear las coordenadas de dicho pokemon, recibidas del mapa
	poke->coor.x = 10;
	poke->coor.y = 10;

	return 0;
}

int moverse_hasta_pokemon( t_poke* pokemon ){
	int movement = RIGHT;
	while( !coor_equals(current_coor, pokemon->coor) ){
		movement = calcular_movimiento(movement, pokemon->coor);

		move_to(movement);
	}

	return 0;
}

void move_to(int movement){
	char move[20];

	switch (movement) {
		case UP:
			sprintf(move, "ARRIBA");
			current_coor.y = current_coor.y - 1;
			break;
		case DOWN:
			sprintf(move, "ABAJO");
			current_coor.y = current_coor.y + 1;
			break;
		case RIGHT:
			sprintf(move, "DERECHA");
			current_coor.x = current_coor.x + 1;
			break;
		case LEFT:
			sprintf(move, "IZQUIERDA");
			current_coor.x = current_coor.x - 1;
			break;
	}

	log_info(logger, "Movimiento del Entrenador: %s", move);
}

int coor_equals(t_coor coorOne, t_coor coorTwo){
	return coorOne.x == coorTwo.x && coorOne.y == coorTwo.y;
}

int calcular_movimiento(int lastMovement, t_coor coor_pokemon){
	int mover;

	bool moverHorizontalmente = (lastMovement == UP || lastMovement == DOWN) && current_coor.x != coor_pokemon.x;
	bool moverVerticalmente = !moverHorizontalmente;

	if(moverHorizontalmente){
		if(current_coor.x < coor_pokemon.x) mover = RIGHT; else mover = LEFT;
	}else{
		if(current_coor.y < coor_pokemon.y) mover = DOWN; else mover = UP;
	}

	return mover;
}

int destroy_maps_list(){
	log_info(logger, "Liberando memoria...");

	int lenght = list_size(maps_list);
	while( lenght > 0){
		log_info(logger, "\ttamaño de maps_list: %d", lenght);
		//conectar_mapa( list_get(maps_list, pos) );
		t_mapa *mapa = list_remove(maps_list, lenght-1);
		destroy_poke_list(mapa->poke_list);
		mapa_destroy(mapa);
		lenght = list_size(maps_list);

	}

	log_info(logger, "Memoria liberada.\n");
	return 0;
}

int destroy_poke_list(t_list *poke_list){
	int lenght = list_size(poke_list);
	while( lenght > 0){
		log_info(logger, "\t\ttamaño de poke_list: %d", lenght);
		t_poke *poke = list_remove(poke_list, lenght-1);
		poke_destroy(poke);
		lenght = list_size(poke_list);
	}

	return 0;
}
*/

















