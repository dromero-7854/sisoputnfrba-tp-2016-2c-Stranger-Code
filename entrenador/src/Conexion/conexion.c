/*
 * conexion.c
 *
 *  Created on: 24/10/2016
 *      Author: utnso
 */
#include "conexion.h"

t_connection* connection_create(char* ip, char* port){
	t_connection* new = malloc( sizeof(t_connection) );
	new->ip = strdup(ip);
	new->port = strdup(port);
	new->socket = NULL;
	return new;
}

void connection_destroy(t_connection* self){
	free(self->ip);
	free(self->port);
	connection_close(self->socket);

	free(self);
}

void connection_open(t_connection* self) {
	struct addrinfo hints;
	struct addrinfo* server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(self->ip, self->port, &hints, &server_info);
	self->socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	connect(self->socket, server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);
}

void connection_close(t_connection* self) {
	close(self->socket);
}

int connection_send(t_connection* connection, int operation_code, char* message){
/**	╔═══════════════════════════════════╦══════════════════════════════════════╦════════════════════════════╗
	║ req_ope_code (prot_ope_code_size) ║ req_message_size (prot_message_size) ║ message (req_message_size) ║
	╚═══════════════════════════════════╩══════════════════════════════════════╩════════════════════════════╝ **/

	uint8_t req_ope_code = operation_code;
	uint32_t req_message_size = strlen(message);
	uint8_t prot_ope_code_size = 1;
	uint8_t prot_message_size = 4;
	void * buffer = malloc(prot_ope_code_size + prot_message_size + req_message_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_message_size, prot_message_size);
	memcpy(buffer + prot_ope_code_size + prot_message_size, message, req_message_size);
	send(connection->socket, buffer, prot_ope_code_size + prot_message_size + req_message_size, 0);
	free(buffer);
}

void connection_recv(t_connection* connection, int* operation_code, char** message){
	uint8_t prot_ope_code_size = 1;
	uint8_t prot_message_size = 4;
	uint32_t message_size;

	if (recv(connection->socket, &operation_code, prot_ope_code_size, 0) <= 0) {
		printf("Error: server %d disconnected...\n", connection->socket);
	} else {
		if (recv(connection->socket, &message_size, prot_message_size, 0) <= 0) {
			printf("Error: server %d no message size...\n", connection->socket);
		} else {
			void *buffer = malloc(message_size);
			if (recv(connection->socket, &buffer, message_size, 0) <= 0) {
				printf("Error: server %d no message...\n", connection->socket);
			} else {
				*message = buffer;
			}
		}
	}
}
