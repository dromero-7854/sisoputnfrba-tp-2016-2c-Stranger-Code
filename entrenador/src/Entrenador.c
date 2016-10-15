/*
 * Proceso Entrenador:
 *
 * Esta la estructura básica que debe cumplir para recorrer los mapas y capturar los pokemones.
 * Se está desarrollando la parte de la conexion al mapa (servidor)
 */

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

typedef struct {
	int x;
	int y;
} t_coor;

typedef struct {
	char *name;
	char *simbol;
	t_coor coor;
} t_poke;

typedef struct {
	char *name;
	char *ip;
	char *port;
	t_list *poke_list;
	int current_poke;
} t_mapa;

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

int main(int argc, char** argv){
	char* temp_file = "entrenador.log";
	t_mapa *current_mapa;
	char* path;

	logger = log_create(temp_file, "TEST",true, LOG_LEVEL_INFO);

	leer_metadata(path);

	int pos = 0;
	while(list_size(maps_list) > pos){
		current_mapa = list_get(maps_list, pos);
		//conectar_mapa(current_mapa);
		completar_mapa(current_mapa);
		//desconectar_mapa(current_mapa);
		pos++;
	}

	//liberar memoria
	eliminar_maps_list();
    log_destroy(logger);

	return 0;
}

int leer_metadata(char* path){
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

	return 0;
}

t_poke *get_current_poke(t_mapa *mapa){
	return list_get(mapa->poke_list, mapa->current_poke);
}

int completar_mapa(t_mapa *mapa){
	while( mapa->current_poke < list_size(mapa->poke_list)-1 ){
		pedir_ubicacion_pokemon(mapa);
		//moverse_hasta_pokemon( get_current_poke(mapa) );
		//capturar_pokemon( get_current_poke(mapa) );

		//log pokemon capturado
		t_poke *pokemon = get_current_poke(mapa);
		log_info(logger, "Capturaste a %s! En la posición: X->%d, Y->%d", pokemon->name, pokemon->coor.x, pokemon->coor.y);
	}
	//log de mapa completo
	log_info(logger, "Felicitaciones! completaste el mapa %s.\n", mapa->name);

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

int eliminar_maps_list(){
	log_info(logger, "Liberando memoria...");

	int lenght = list_size(maps_list);
	while( lenght > 0){
		log_info(logger, "\ttamaño de maps_list: %d", lenght);
		//conectar_mapa( list_get(maps_list, pos) );
		t_mapa *mapa = list_remove(maps_list, lenght-1);
		eliminar_poke_list(mapa->poke_list);
		mapa_destroy(mapa);
		lenght = list_size(maps_list);

	}

	log_info(logger, "Memoria liberada.\n");
	return 0;
}

int eliminar_poke_list(t_list *poke_list){
	int lenght = list_size(poke_list);
	while( lenght > 0){
		log_info(logger, "\t\ttamaño de poke_list: %d", lenght);
		t_poke *poke = list_remove(poke_list, lenght-1);
		poke_destroy(poke);
		lenght = list_size(poke_list);
	}

	return 0;
}

















//TODO: este es un ejemplo de una conexion basica a un servidor. Se debe adaptar.
int conexion(){
	/*
	 *  ¿Quien soy? ¿Donde estoy? ¿Existo?
	 *  Estas y otras preguntas existenciales son resueltas getaddrinfo();
	 *  Obtiene los datos de la direccion de red y lo guarda en serverInfo.
	 */
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion


	/*
	 * 	Obtiene un socket (un file descriptor -todo en linux es un archivo-), utilizando la estructura serverInfo que generamos antes.
	 */
	int serverSocket;
	serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	/*
	 * 	Perfecto, ya tengo el medio para conectarme (el archivo), y ya se lo pedi al sistema.
	 * 	Ahora me conecto!
	 */
	connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);	// No lo necesitamos mas

	/*
	 *	Estoy conectado! Ya solo me queda una cosa:
	 *	Enviar datos!
	 *
	 *	Vamos a crear un paquete (en este caso solo un conjunto de caracteres) de size PACKAGESIZE, que le enviare al servidor.
	 *	Aprovechando el standard input/output, guardamos en el paquete las cosas que ingrese el usuario en la consola.
	 *	Ademas, contamos con la verificacion de que el usuario escriba "exit" para dejar de transmitir.
	 */
	int enviar = 1;
	char message[PACKAGESIZE];

	//llamanos a una función de commons
	char* tiempo = temporal_get_string_time();
	log_info(logger, "Conectado al servidor a las %s. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n", tiempo);
	free(tiempo);

	while(enviar){
		fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
		if (!strcmp(message,"exit\n")) enviar = 0;			// Chequeo que el usuario no quiera salir
		if (enviar) send(serverSocket, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
	}

	/*
	 *	Ahora solo me queda cerrar la conexion con un close();
	 */
	close(serverSocket);

	return 0;
}
