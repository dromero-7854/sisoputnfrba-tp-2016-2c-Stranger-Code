/*
 * Modelo ejemplo de un servidor que espera mensajes de un proceso Cliente que se conecta a un cierto puerto.
 * Al recibir un mensaje, lo imprimira por pantalla.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "Conexion/conexion.h"

#define PUERTO "6667"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

t_coor* coorEntrenador;
t_coor* coorPokemon;

int main(){
	coorEntrenador = (t_coor*) malloc(sizeof(t_coor));
	coorPokemon = (t_coor*) malloc(sizeof(t_coor));

	/*
	 *  ¿Quien soy? ¿Donde estoy? ¿Existo?
	 *
	 *  Estas y otras preguntas existenciales son resueltas getaddrinfo();
	 *
	 *  Obtiene los datos de la direccion de red y lo guarda en serverInfo.
	 *
	 */
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE


	/*
	 * 	Descubiertos los misterios de la vida (por lo menos, para la conexion de red actual), necesito enterarme de alguna forma
	 * 	cuales son las conexiones que quieren establecer conmigo.
	 *
	 * 	Para ello, y basandome en el postulado de que en Linux TODO es un archivo, voy a utilizar... Si, un archivo!
	 *
	 * 	Mediante socket(), obtengo el File Descriptor que me proporciona el sistema (un integer identificador).
	 *
	 */
	/* Necesitamos un socket que escuche las conecciones entrantes */
	int listenningSocket;
	listenningSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	/*
	 * 	Perfecto, ya tengo un archivo que puedo utilizar para analizar las conexiones entrantes. Pero... ¿Por donde?
	 *
	 * 	Necesito decirle al sistema que voy a utilizar el archivo que me proporciono para escuchar las conexiones por un puerto especifico.
	 *
	 * 				OJO! Todavia no estoy escuchando las conexiones entrantes!
	 *
	 */
	bind(listenningSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar

	/*
	 * 	Ya tengo un medio de comunicacion (el socket) y le dije por que "telefono" tiene que esperar las llamadas.
	 *
	 * 	Solo me queda decirle que vaya y escuche!
	 *
	 */
	listen(listenningSocket, BACKLOG);		// IMPORTANTE: listen() es una syscall BLOQUEANTE.

	/*
	 * 	El sistema esperara hasta que reciba una conexion entrante...
	 * 	...
	 * 	...
	 * 	BING!!! Nos estan llamando! ¿Y ahora?
	 *
	 *	Aceptamos la conexion entrante, y creamos un nuevo socket mediante el cual nos podamos comunicar (que no es mas que un archivo).
	 *
	 *	¿Por que crear un nuevo socket? Porque el anterior lo necesitamos para escuchar las conexiones entrantes. De la misma forma que
	 *	uno no puede estar hablando por telefono a la vez que esta esperando que lo llamen, un socket no se puede encargar de escuchar
	 *	las conexiones entrantes y ademas comunicarse con un cliente.
	 *
	 *			Nota: Para que el listenningSocket vuelva a esperar conexiones, necesitariamos volver a decirle que escuche, con listen();
	 *				En este ejemplo nos dedicamos unicamente a trabajar con el cliente y no escuchamos mas conexiones.
	 *
	 */
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen);

	/*
	 * 	Ya estamos listos para recibir paquetes de nuestro cliente...
	 *
	 * 	Vamos a ESPERAR (ergo, funcion bloqueante) que nos manden los paquetes, y los imprimieremos por pantalla.
	 *
	 *	Cuando el cliente cierra la conexion, recv() devolvera 0.
	 */
	char package[PACKAGESIZE];
	int status = 1;		// Estructura que manjea el status de los recieve.

	printf("Cliente conectado. Esperando mensajes:\n");

	/*while (status != 0){
		status = recv(socketCliente, (void*) package, PACKAGESIZE, 0);
		if (status != 0) printf("%s", package);

	}*/
	t_connection* conn;
	uint8_t operation_code;
	void* message;
	char* mensaje;// = malloc(20);

	conn = connection_create("127.0.0.1", PUERTO);
	conn->socket =  socketCliente;

	while (status != 0){
		// recibe una peticion, segun el codigo de operacion casteamos el mensaje y respondemos la peticion
		status = connection_recv(conn, &operation_code, &message);
		if (status != 0){
			printf("Operacion: %d\n - Mensaje: %s\n", operation_code, (char*)message);

			switch (operation_code) {
			// peticion para ubucar a un nuevo entrenador y devolver su posicion.
				case OC_UBICAR_ENTRENADOR:
					coorEntrenador->x = 1;
					coorEntrenador->y = 1;

					printf("Enviando... -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_ENTRENADOR, coorEntrenador->x, coorEntrenador->y);
					connection_send(conn, OC_UBICACION_ENTRENADOR, coorEntrenador);
					printf("Enviado OK! -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_ENTRENADOR, coorEntrenador->x, coorEntrenador->y);
					break;
			// peticion para ubicar a un pokemon/pokenest y devuelve su posicion
				case OC_UBICAR_POKENEST:
					if(*(char*)message == 'P'){
						coorPokemon->x = 15;
						coorPokemon->y = 15;
					} else if(*(char*)message == 'R'){
						coorPokemon->x = 16;
						coorPokemon->y = 20;
					} else if(*(char*)message == 'B'){
						coorPokemon->x = 29;
						coorPokemon->y = 29;
					}

					printf("Enviando... -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_POKENEST, coorPokemon->x, coorPokemon->y);
					connection_send(conn, OC_UBICACION_POKENEST, coorPokemon);
					printf("Enviado OK! -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_POKENEST, coorPokemon->x, coorPokemon->y);
					break;
			// peticion para mover a un entrenador y devuelve su nueva posicion
				case OC_AVANZAR_POSICION:
					//int* movement = (int*) message;

					switch (*(int*)message) {
						case MOVE_UP:
							//sprintf(move, "ARRIBA");
							coorEntrenador->y--;
							break;
						case MOVE_DOWN:
							//sprintf(move, "ABAJO");
							coorEntrenador->y++;
							break;
						case MOVE_RIGHT:
							//sprintf(move, "DERECHA");
							coorEntrenador->x++;
							break;
						case MOVE_LEFT:
							//sprintf(move, "IZQUIERDA");
							coorEntrenador->x--;
							break;
					}

					printf("Enviando... -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_ENTRENADOR, coorEntrenador->x, coorEntrenador->y);
					connection_send(conn, OC_UBICACION_ENTRENADOR, coorEntrenador);
					printf("Enviado OK! -> Operacion: %d - Mensaje: Coor X=%d Coor Y=%d\n", OC_UBICACION_ENTRENADOR, coorEntrenador->x, coorEntrenador->y);
					break;
			// peticion para atrapar un pokemon
				case OC_ATRAPAR_POKEMON:
					mensaje = strdup("pokemon atrapado");
					printf("Enviando... -> Operacion: %d - Mensaje: %s\n", OC_ATRAPAR_POKEMON, mensaje);
					connection_send(conn, OC_MENSAJE, mensaje);
					printf("Enviado OK! -> Operacion: %d - Mensaje: %s\n", OC_ATRAPAR_POKEMON, mensaje);
					break;
				default:
					break;
			}
		}
	}


	/*
	 * 	Terminado el intercambio de paquetes, cerramos todas las conexiones y nos vamos a mirar Game of Thrones, que seguro nos vamos a divertir mas...
	 *
	 *
	 * 																					~ Divertido es Disney ~
	 *
	 */
	free(mensaje);
	free(coorEntrenador);
	free(coorPokemon);
	close(socketCliente);
	close(listenningSocket);

	/* See ya! */

	return 0;
}
