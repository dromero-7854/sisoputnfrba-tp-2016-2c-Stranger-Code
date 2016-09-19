/*
 * pokedex-client.c
 *
 *  Created on: 18/9/2016
 *      Author: utnso
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define PORT "33000"

int main(int argc , char * argv[]) {

	struct addrinfo hints;
	struct addrinfo * server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(IP, PORT, &hints, &server_info);
	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);

	// message
	char * msg = malloc(38 + 1);
	strcpy(msg, "/mnt/pokedex/Entrenadores/Ash/metadata");

	// message serialization
	size_t msg_length = strlen(msg) + 1;
	void * buffer = malloc(4 + msg_length);
	memcpy(buffer, &msg_length, 4);
	memcpy(buffer + 4, msg, msg_length);

	// << sending message >>
	send(server_socket, buffer, 4 + msg_length, 0);

	//
	// receiving message
	//
	size_t resp_length;
	recv(server_socket, &resp_length, 4, 0);
	char resp[resp_length];
	recv(server_socket, resp, resp_length, 0);
	printf("response: %s", resp);

	free(buffer);
	free(msg);
	close(server_socket);
	return EXIT_SUCCESS;
}
