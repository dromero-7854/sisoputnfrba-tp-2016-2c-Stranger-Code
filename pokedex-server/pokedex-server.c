/*
 * pokedex-server.c
 *
 *  Created on: 18/9/2016
 *      Author: Dante Romero
 */
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "33000"
#define BACKLOG 5

int main(int argc , char * argv[]) {

	struct addrinfo hints;
	struct addrinfo * server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE; //	localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, PORT, &hints, &server_info);

	int listenning_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	bind(listenning_socket,server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);

	//TODO for(;;){}
	printf("pokedex server: welcome!...\n");
	printf("pokedex server: beautiful day to hunt pokemons...\n");
	printf("pokedex server: waiting for clients...\n");
	listen(listenning_socket, BACKLOG); // blocking syscall

	// << client connection >>
	struct sockaddr_in addr; // client data (ip, port, etc.)
	socklen_t addrlen = sizeof(addr);
	int client_socket = accept(listenning_socket, (struct sockaddr *) &addr, &addrlen);

	printf("pokedex server: connected client! waiting messages...\n");

	int status = 1;
	size_t msg_length;
	recv(client_socket, &msg_length, 4, 0);
	char msg[msg_length];
	while (status != 0){
		status = recv(client_socket, msg, msg_length, 0);
		if (status != 0) {
			printf("pokedex server: Hi client %d\n", client_socket);
			printf("pokedex server: please tell me, what do you need?: %s\n", msg);

			// TODO

			// response
			char * resp = malloc(178 + 1);
			strcpy(resp, "nombre=Red\nsimbolo=@\nhojaDeViaje=[PuebloPaleta,CiudadVerde,CiudadPlateada]\nobj[PuebloPaleta]=[P,B,G]\nobj[CiudadVerde]=[C,Z,C]\nobj[CiudadPlateada]=[P,M,P,M,S]\nvidas=5\nreintentos=0");

			// response serialization
			size_t resp_length = strlen(resp) + 1;
			void * buffer = malloc(4 + resp_length);
			memcpy(buffer, &resp_length, 4);
			memcpy(buffer + 4, resp, resp_length);

			// << sending response >>
			write(client_socket , buffer, 4 + resp_length);

			free(buffer);
		}
	}
	printf("pokedex server: Bye client %d! \n", client_socket);
	close(client_socket);

	close(listenning_socket);
	return EXIT_SUCCESS;
}





















