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
#define HEADER_MSG_LENGHT 4
#define HEADER_RESP_LENGHT 4

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

	// << sending message >>
	// message
	char * msg = malloc(38);
	strcpy(msg, "/mnt/pokedex/Entrenadores/Ash/metadata");

	// message serialization
	uint32_t msg_length = strlen(msg);
	void * buffer = malloc(HEADER_MSG_LENGHT + msg_length);
	memcpy(buffer, &msg_length, HEADER_MSG_LENGHT);
	memcpy(buffer + HEADER_MSG_LENGHT, msg, msg_length);

	// sending
	send(server_socket, buffer, HEADER_MSG_LENGHT + msg_length, 0);
	free(buffer);








	// << receiving message >>
	// response lenght
	uint32_t resp_length;
	uint32_t received_bytes = recv(server_socket, &resp_length, HEADER_RESP_LENGHT, 0);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return 1;
	}
	char * resp = malloc(resp_length);
	received_bytes = recv(server_socket, resp, resp_length, 0);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return 1;
	}
	resp[received_bytes] = '\0';
	printf("pokedex client: response >> %s\n", resp);
	free(resp);



	close(server_socket);
	return EXIT_SUCCESS;
}
