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
	connection_close(self);

	free(self);
}

int connection_open(t_connection* self) {
	int result;
	struct addrinfo hints;
	struct addrinfo* server_info;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(self->ip, self->port, &hints, &server_info);
	self->socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	result = connect(self->socket, server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);

	return result;
}

void connection_close(t_connection* self) {
	close(self->socket);
}

int connection_send(t_connection* connection, uint8_t operation_code, void* message){
/**	╔══════════════════════════════════════════════╦══════════════════════════════════════════╦══════════════════════════════╗
	║ operation_code_value (operation_code_length) ║ message_size_value (message_size_length) ║ message (message_size_value) ║
	╚══════════════════════════════════════════════╩══════════════════════════════════════════╩══════════════════════════════╝ **/

	uint8_t operation_code_value = operation_code;
	uint8_t message_size_value;

	switch ((int)operation_code) {
		case OC_UBICACION_POKENEST:
		case OC_UBICACION_ENTRENADOR:
			message_size_value = sizeof(t_coor);
			break;
		case OC_UBICAR_POKENEST:
		case OC_UBICAR_ENTRENADOR:
		case OC_AVANZAR_POSICION:
		case OC_ATRAPAR_POKEMON:
		case OC_OBTENER_MEDALLA:
		case OC_MEDALLA:
		case OC_MENSAJE:
		case OC_POKEMON:
			message_size_value = string_length((char*)message);
			break;
		default:
			printf("ERROR: Socket %d, Invalid operation code...\n", connection->socket);
			break;
	}

	uint8_t operation_code_length = sizeof(uint8_t);
	uint8_t message_size_length = sizeof(uint8_t);
	void * buffer = malloc(operation_code_length + message_size_length + message_size_value);
	memcpy(buffer, &operation_code_value, operation_code_length);
	memcpy(buffer + operation_code_length, &message_size_value, message_size_length);
	memcpy(buffer + operation_code_length + message_size_length, message, message_size_value);
	int ret = send(connection->socket, buffer, operation_code_length + message_size_length + message_size_value, 0);
	free(buffer);

	return ret;
}

int connection_recv(t_connection* connection, uint8_t* operation_code_value, void** message){
/**	╔══════════════════════════════════════════════╦══════════════════════════════════════════╦══════════════════════════════╗
	║ operation_code_value (operation_code_length) ║ message_size_value (message_size_length) ║ message (message_size_value) ║
	╚══════════════════════════════════════════════╩══════════════════════════════════════════╩══════════════════════════════╝ **/

	uint8_t prot_ope_code_size = sizeof(uint8_t);
	uint8_t prot_message_size = sizeof(uint8_t);
	uint8_t message_size;
	int status = 1;
	int ret = 0;
	char* buffer;
	t_coor* coor;

	status = recv(connection->socket, operation_code_value, prot_ope_code_size, 0);
	if (status <= 0) {
		printf("ERROR: Socket %d, disconnected...\n", connection->socket);
		game_over();
	} else {
		ret = ret + status;
		status = recv(connection->socket, &message_size, prot_message_size, 0);
		if (status <= 0) {
			printf("ERROR: Socket %d, no message size...\n", connection->socket);
		} else {
			ret = ret + status;
			//message = (void*) malloc(message_size);
			switch ((int)*operation_code_value) {
				case OC_UBICACION_POKENEST:
				case OC_UBICACION_ENTRENADOR:
					coor = malloc(message_size);
					status = recv(connection->socket, coor, message_size, 0);
					if(status > 0){
						*message = coor;
					}
					//free(coor);
					break;
				case OC_UBICAR_POKENEST:
				case OC_VICTIMA_DEADLOCK:
				case OC_UBICAR_ENTRENADOR:
				case OC_AVANZAR_POSICION:
				case OC_CANTIDAD_DEADLOCK:
				case OC_ATRAPAR_POKEMON:
				case OC_OBTENER_MEDALLA:
				case OC_MEDALLA:
				case OC_MENSAJE:
				case OC_GANO_BATALLA:
				case OC_PERDIO_BATALLA:
				case OC_POKEMON_BATALLA:
				case OC_POKEMON:
					buffer = malloc(message_size + 1);
					if(message_size > 0){
						status = recv(connection->socket, buffer, message_size, 0);
					}
					if(status > 0){
						buffer[message_size] = '\0';
						*message = buffer;
					}
					//free(buffer);
					break;
				default:
					printf("ERROR: Socket %d, Invalid operation code(%d)...\n", connection->socket, (int)*operation_code_value);
					game_over();
					break;
			}

			if (status <= 0) {
				printf("ERROR: Socket %d, no message...\n", connection->socket);
				ret = status;
			}else{
				ret = ret + status;
			}

		}
	}

	return ret;
}
