/*
 * nivel-test.c


 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */


#include "nivel-test.h"
#include "planificacion.h"
#include "deteccionDeadlock.h"
#include "dibujarNivel.h"


#define QUANTUM 5


void releerConfiguracion(int n){
	leerConfiguracion(conf_metadata, rutaMetadata);
}

void sigint(){
	pthread_kill(planificador, SIGINT);
	pthread_kill(pth, SIGINT);
	pthread_kill(guipth, SIGINT);
	list_clean_and_destroy_elements(listaPokenests, (void*)_borrar_pokenest);
	list_destroy(listaPokenests);
	nivel_gui_terminar();
	exit(1);
}

int main(int argc, char* argv[]) {

	if(argc!=3) {
		printf("Faltan ingresar parametos. Se debe ejecutar de la sig. manera:\n ./mapa <nombre_mapa> <punto_montaje>\n");
		exit(1);
	}


	signal(SIGUSR1, releerConfiguracion);
	struct sigaction s;
	s.sa_handler = sigint;
	sigemptyset(&s.sa_mask);
	s.sa_flags = 0;
	sigaction(SIGINT, &s, NULL);

	log_mapa = crear_log(argv[1]);

	nombre_mapa = argv[1];
	pto_montaje = argv[2];

	ruta_mapa = getRutaMapa(pto_montaje, nombre_mapa);

	items  = list_create();
	entrenadores = list_create();
	listaPokenests = list_create();

	colaListos = queue_create();
	colaBloqueados = queue_create();


	t_combo comboListas;
	comboListas.entrenadores = entrenadores;
	comboListas.pokenests = listaPokenests;

	log_trace(log_mapa, "Se iniciaron las colas y listas");

	rutaMetadata = getRutaMetadata();
	log_trace(log_mapa, "Se obtuvo la ruta a metadata %s", rutaMetadata);
	conf_metadata = malloc(sizeof(metadata));
	leerConfiguracion(conf_metadata, rutaMetadata);
	log_trace(log_mapa, "se cargo la metadata");

	char *rutaPokenests;
	rutaPokenests = getRutaPokenests();
	t_pkmn_factory* fabrica = create_pkmn_factory();
	cargarPokenests(rutaPokenests, fabrica);
	free(rutaPokenests);



	pthread_attr_t attr;
	pthread_attr_t attr2;
	pthread_attr_t attr3;

	sem_init(&sem_dibujo, 0, 1);
	sem_init(&sem_turno, 0, 1);

	pthread_mutex_init(&mutex_lista_entrenador, NULL);
	pthread_mutex_init(&mutex_lista_pokenest, NULL);
	pthread_mutex_init(&dibujo, NULL);

	pthread_mutex_init(&mutex_cola_listos, NULL);
	pthread_mutex_init(&mutex_cola_bloqueados, NULL);
	pthread_mutex_init(&mutex_turno_desbloqueo, NULL);
	pthread_mutex_init(&deadlock_ejecutando, NULL);

	pthread_attr_init(&attr);
	pthread_attr_init(&attr2);
	pthread_attr_init(&attr3);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&planificador, NULL, (void *) planificar, NULL) != 0){
		log_error(log_mapa, "problemas al crear hilo planificador");
	}

	pthread_attr_destroy(&attr);

	pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&guipth, NULL, (void *) dibujarNivel, &comboListas)) {
				log_error(log_mapa, "Error creando el hilo de la GUI");
				return 1;
	}

	pthread_attr_destroy(&attr2);
	log_trace(log_mapa, "se creo hilo de dibujo");

	pthread_attr_setdetachstate(&attr3, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&pth, NULL, (void *)detectarDeadlock, &comboListas)) {

		log_error(log_mapa, "Error creando hilo deadlock\n");
		return 1;
	}

	pthread_attr_destroy(&attr3);

	log_trace(log_mapa, "se creo hilo deadlock");


	sigset_t sigset, oldset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	pthread_sigmask(SIG_BLOCK, &sigset, &oldset);

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	int listener;
	listener = socket_servidor(conf_metadata->ip, conf_metadata->puerto, log_mapa);
	manejar_select(listener, log_mapa);

	liberar_variables_globales();
	//liberar conf_metadata
	return EXIT_SUCCESS;
}

void manejar_select(int socket, t_log* log){
	//fd_set lectura, master;
	int nuevaConexion, a, recibido;
	char buf[512];
	char *nombre_entrenador;
	char simbolo;
	t_entrenador* entrenador;
	set_fd_max = socket;
	FD_ZERO(&lectura);
	FD_ZERO(&master);
	FD_SET(socket, &master);
	while(1){
		lectura = master;
		select(set_fd_max +1, &lectura, NULL, NULL, &tv);
		for(a = 0 ; a <= set_fd_max ; a++){
			if(FD_ISSET(a, &lectura)){
					if(a == socket){
						nuevaConexion = aceptar_conexion(socket, log);
						handshake(nuevaConexion, &simbolo, &nombre_entrenador);
						t_entrenador* nuevoEntrenador = crearEntrenador(nuevaConexion, simbolo, nombre_entrenador);
						free(nombre_entrenador);
						CrearPersonaje(items, nuevoEntrenador->simbolo, nuevoEntrenador -> posx, nuevoEntrenador -> posy);
						list_add(entrenadores, nuevoEntrenador);
						sem_post(&sem_dibujo);
						pthread_mutex_lock(&mutex_cola_listos);
						queue_push(colaListos, nuevoEntrenador);
						informar_contenido_cola(colaListos);
						pthread_mutex_unlock(&mutex_cola_listos);
					} else {
						pthread_mutex_lock(&deadlock_ejecutando);
						pthread_mutex_lock(&mutex_turno_desbloqueo);
						entrenador = buscarEntrenador(a, colaBloqueados->elements);
						pthread_mutex_unlock(&mutex_turno_desbloqueo);
						pthread_mutex_unlock(&deadlock_ejecutando);
						if(entrenador == NULL)continue;
						recibido = recv(a, buf, 512, 0);
						if(recibido == 0){
							FD_CLR(entrenador->id, &master);
							pthread_mutex_lock(&mutex_turno_desbloqueo);
							pthread_mutex_lock(&mutex_cola_bloqueados);
							liberarRecursos2(entrenador);
							pthread_mutex_unlock(&mutex_cola_bloqueados);
							pthread_mutex_unlock(&mutex_turno_desbloqueo);
							sem_post(&sem_dibujo);
						}

					}
				}
		}
	}
}

void leerConfiguracion(metadata* conf_metadata, char* ruta){
	t_config* configuracion = config_create(ruta);
	conf_metadata->tiempoChequeoDeadlock = config_get_int_value(configuracion, "TiempoChequeoDeadlock");
	conf_metadata->batalla = config_get_int_value(configuracion, "Batalla");
	meterStringEnEstructura(&(conf_metadata->algoritmo), config_get_string_value(configuracion, "algoritmo"));
	meterStringEnEstructura(&(conf_metadata->ip), config_get_string_value(configuracion, "IP"));
	meterStringEnEstructura(&(conf_metadata->puerto), config_get_string_value(configuracion, "Puerto"));
	conf_metadata->retardo = config_get_int_value(configuracion, "retardo");
	conf_metadata->retardo = conf_metadata->retardo * 1000;
	conf_metadata->quantum = config_get_int_value(configuracion, "quantum");
	config_destroy(configuracion);
}

char* getRutaMapa(char* ptoMnt, char* nombreMapa){
	char* ruta = string_new();
	string_append_with_format(&ruta, "Mapas/%s", nombreMapa);
	return ruta;
}
char* getRutaMetadata(){
	char* ruta = string_duplicate(ruta_mapa);
	//char* archivo = string_new();
	string_append(&ruta, "/metadata");
	char* rutaAbsoluta = getRutaAbsoluta(ruta);
	//free(directorio);
	free(ruta);
	return rutaAbsoluta;
}

char* getRutaPokenests(){
	char* ruta2 = string_new();
	string_append_with_format(&ruta2, "%s/PokeNests", ruta_mapa);
	return ruta2;
}

char* getRutaPokemon(char* rutaPokenests, char* pokemon){
	char* ruta2 = string_new();
	char* nombre_pokemon = string_duplicate(pokemon);
	string_append_with_format(&ruta2, "%s/%s", rutaPokenests, nombre_pokemon);
	free(nombre_pokemon);
	return ruta2;
}

PokeNest* crearPokenest(char* rutaPokenest){
	PokeNest* pokeNest = malloc(sizeof(PokeNest));
	char** array;
	char* metadata = string_duplicate(rutaPokenest);
	string_append(&metadata, "/metadata");
	t_config* config = config_create(metadata);
	pokeNest->id = *(config_get_string_value(config, "Identificador"));
	char *posiciones, *posicion ;

	meterStringEnEstructura(&posiciones, config_get_string_value(config, "Posicion"));
	array = string_split(posiciones, ";");
	pokeNest->posx = atoi(array[0]);
	pokeNest->posy = atoi(array[1]);

	/*posicion = strtok(posiciones, ";");
	pokeNest->posx = atoi(posicion);
	posicion = strtok(NULL, ";");
	pokeNest->posy = atoi(posicion);*/

	//free(posiciones);
	vaciar_array(array);
	free(array);
	free(posiciones);
	free(metadata);
	config_destroy(config);
	return pokeNest;
}

t_list* crearPokemons(char* rutaPokemon, t_pkmn_factory* fabrica, char* nombrePokemon, char pokenest_id){
	DIR* dir;
	struct dirent* directorio;
	dir = opendir(rutaPokemon);
	int lvl;
	t_list* listaPokemons = list_create();
	while((directorio = readdir(dir)) != NULL){
		if(!strcmp(directorio->d_name, ".") || !strcmp(directorio->d_name, "..")) continue;
		if(!strcmp(directorio->d_name, "metadata")) continue;

		char* metadataPokemon = string_duplicate(rutaPokemon);
		string_append_with_format(&metadataPokemon, "/%s", directorio->d_name);

		t_config* config = config_create(metadataPokemon);
		lvl = config_get_int_value(config, "Nivel");

		t_infoPokemon* infoPokemon = malloc(sizeof(t_infoPokemon));
		infoPokemon->pokemon = create_pokemon(fabrica, nombrePokemon, lvl);
		infoPokemon->id_pokenest = pokenest_id;
		meterStringEnEstructura(&(infoPokemon->nombre), directorio->d_name);

		list_add(listaPokemons, infoPokemon);

		//free(infoPokemon);
		free(metadataPokemon);
		config_destroy(config);
	}
	closedir(dir);
	return listaPokemons;
}

t_entrenador* crearEntrenador(int file_descriptor, char simbolo, char* nombre){
	t_entrenador* entrenador = malloc(sizeof(t_entrenador));
	entrenador->nombre = string_duplicate(nombre);
	entrenador->id = file_descriptor;
	entrenador->posx = 1;
	entrenador->posy = 1;
	entrenador->pokenest_buscada = NULL;
	entrenador->ultimo_pokemon = 0;
	entrenador->pokemons = list_create();
	//entrenador->tiempos = malloc(sizeof(t_tiempos));
	//entrenador->tiempos->inicio = time(NULL);
	entrenador->simbolo = simbolo;
	entrenador->cantDeadlocks = 0;
	return entrenador;
}

void eliminarEntrenador(int fd_entrenador){
	buscar_entrenador_y_borrar(colaListos, fd_entrenador);
	buscar_entrenador_y_borrar(colaBloqueados, fd_entrenador);
}

void buscar_entrenador_y_borrar(t_queue* cola, int file_descriptor){
	//t_list* listaAux;
	t_entrenador* entrenadorAux;

	pthread_mutex_lock(&mutex_cola_listos);
	entrenadorAux = buscarEntrenador(file_descriptor, cola->elements);
	pthread_mutex_unlock(&mutex_cola_listos);
	liberarEntrenador(entrenadorAux);
}

void liberarEntrenador(t_entrenador* entrenador){

	free(entrenador->nombre);
	int i;
	for(i = 0; i < list_size(entrenadores); i++) {
		t_entrenador *entr = list_get(entrenadores, i);
		if(entr->simbolo == entrenador->simbolo) {
			pthread_mutex_lock(&mutex_lista_entrenador);
			list_remove(entrenadores, i);
			pthread_mutex_unlock(&mutex_lista_entrenador);
		}
	}
	BorrarItem(items, entrenador->simbolo);
	free(entrenador);
}

void cargarPokenests(char* rutaPokenests_relativa, t_pkmn_factory* fabrica){
	DIR* d;
	char* rutaPokemon, *rutaPokenests_absoluta, *nombre_pokemon;
	rutaPokenests_absoluta = getRutaAbsoluta(rutaPokenests_relativa);
	struct dirent *directorio;
	d = opendir(rutaPokenests_absoluta);

	while((directorio = readdir(d)) != NULL){
		if((!strcmp(directorio->d_name, ".")) || (!strcmp(directorio->d_name, ".."))) continue;
		rutaPokemon = getRutaPokemon(rutaPokenests_absoluta, directorio->d_name);

		PokeNest* pokenest = crearPokenest(rutaPokemon);
		nombre_pokemon = strdup(directorio->d_name);
		pokenest->listaPokemons = crearPokemons(rutaPokemon, fabrica, nombre_pokemon, pokenest->id);
		pokenest->cantidad = list_size(pokenest->listaPokemons);

		pthread_mutex_lock(&mutex_lista_pokenest);
		list_add(listaPokenests, pokenest);
		pthread_mutex_unlock(&mutex_lista_pokenest);

		CrearCaja(items, pokenest->id, pokenest->posx, pokenest->posy, pokenest->cantidad);
		free(rutaPokemon);
	}
	free(rutaPokenests_absoluta);
	closedir(d);
}

t_log* crear_log(char* nombre){
	char nombre_archivo[256];
	snprintf((char *)&nombre_archivo, 256, "%s.log", nombre);
	t_log* logger = log_create(nombre_archivo, "MAPA", false, LOG_LEVEL_TRACE);
	return logger;
}
void destruir_config(metadata* config){
	free(config->algoritmo);
	free(config->ip);
	free(config->puerto);
	free(config);
}

void liberar_variables_globales(){
	free(ruta_mapa);
	free(pto_montaje);
	free(nombre_mapa);
}

void informar_contenido_cola(t_queue* cola){
	int len;
	char* contenido_cola = string_new();
	if(cola == colaListos){
		string_append(&contenido_cola, "cola Listos: [ ");
	} else {
		string_append(&contenido_cola, "cola Bloqueados [ ");
	}
	void _append_to_queue(t_entrenador* e){
		string_append_with_format(&contenido_cola, "%s,", e->nombre);
	}
	list_iterate(cola->elements, (void*)_append_to_queue);
	len = strlen(contenido_cola);
	contenido_cola[len-1] = ']';
	//string_append(&contenido_cola, "]");
	log_trace(log_mapa, "%s", contenido_cola);
	free(contenido_cola);
}



char* getRutaAbsoluta(char* rutaRelativa){
	char* rutaAbsoluta;
	rutaAbsoluta = string_duplicate(pto_montaje);
	string_append(&rutaAbsoluta, rutaRelativa);
	return rutaAbsoluta;
}

void _borrar_pokenest(PokeNest* pokenest){
	free(pokenest->nombrePokemon);
	list_clean_and_destroy_elements(pokenest->listaPokemons, (void*)_borrar_pokemon);
	list_destroy(pokenest->listaPokemons);
	free(pokenest);
}

void _borrar_pokemon(t_infoPokemon* infopokemon){
	free(infopokemon->nombre);
	free(infopokemon->pokemon->species);
	free(infopokemon->pokemon);
	free(infopokemon);
}

void vaciar_array(char** array){
	char* palabra;
	int i = 0;
	while((palabra = array[i]) != NULL){
		free(palabra);
		i++;
	}
}
/*t_infoPokemon* crear_infopokemon(){
	t_infoPokemon* infopokemon = malloc(sizeof(t_infoPokemon));
	infopokemon->pokemon = malloc(sizeof(t_pokemon));
	infopokemon->nombre = malloc
}*/
