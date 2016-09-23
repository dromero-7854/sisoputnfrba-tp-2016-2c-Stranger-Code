/*
 * pokedex-server.c
 *
 *  Created on: 18/9/2016
 *           Author: Dante Romero
 */
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include "osada.h"

/**
 * socket connection
 */
#define PORT "33000"
#define BACKLOG 5
#define HEADER_MSG_LENGHT 4
#define HEADER_RESP_LENGHT 4
int listenning_socket;

/**
 * osada file system
 */
#define FS_PATH "./osada-fs/osada-fs"
struct stat sbuf;
int fd;
void * osada_fs_ptr;

int open_socket_connection(void);
int close_socket_connection(void);
int map_osada_fs(void);
int unmap_osada_fs(void);
int welcome_msg(void);

int main(int argc , char * argv[]) {
	open_socket_connection();
	map_osada_fs();
	welcome_msg();
	for (;;) {

		listen(listenning_socket, BACKLOG); // blocking syscall

		// << client connection >>
		struct sockaddr_in addr; // client data (ip, port, etc.)
		socklen_t addrlen = sizeof(addr);
		int client_socket = accept(listenning_socket, (struct sockaddr *) &addr, &addrlen);
		printf("pokedex server: hi client %d!!\n", client_socket);
		printf("pokedex server: waiting messages...\n");


		// << receiving message >>
		// message lenght
		uint32_t msg_length;
		uint32_t received_bytes = recv(client_socket, &msg_length, HEADER_MSG_LENGHT, 0);
		if (received_bytes <= 0) {
			printf("pokedex server: client %d disconnected...\n", client_socket);
			return 1;
		}
		// message
		char * msg = malloc(msg_length);
		received_bytes = recv(client_socket, msg, msg_length, 0);
		if (received_bytes <= 0) {
			printf("pokedex server: client %d disconnected...\n", client_socket);
			return 1;
		}
		msg[received_bytes] = '\0';
		printf("pokedex server: please tell me, what do you need?: %s\n", msg);
		//
		// TODO OSADA-FS
		//
		free(msg);









		// << sending response >>
		char * resp = malloc(178);
		strcpy(resp, "nombre=Red\nsimbolo=@\nhojaDeViaje=[PuebloPaleta,CiudadVerde,CiudadPlateada]\nobj[PuebloPaleta]=[P,B,G]\nobj[CiudadVerde]=[C,Z,C]\nobj[CiudadPlateada]=[P,M,P,M,S]\nvidas=5\nreintentos=0");

		// serialization
		size_t resp_msg_length = strlen(resp);
		void * buffer = malloc(HEADER_RESP_LENGHT + resp_msg_length);
		memcpy(buffer, &resp_msg_length, HEADER_RESP_LENGHT);
		memcpy(buffer + HEADER_RESP_LENGHT, resp, resp_msg_length);

		// sending...
		write(client_socket , buffer, HEADER_RESP_LENGHT + resp_msg_length);
		free(buffer);
		printf("pokedex server: Bye client %d! \n", client_socket);
		close(client_socket);

	}
	unmap_osada_fs();
	close_socket_connection();
	return EXIT_SUCCESS;
}

int open_socket_connection(void) {
	struct addrinfo hints;
	struct addrinfo * server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE; //	localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, PORT, &hints, &server_info);

	listenning_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	bind(listenning_socket,server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);
	return EXIT_SUCCESS;
}

int welcome_msg(void) {
	printf("pokedex server: welcome to pokedex-server 1.0v!...\n");
	printf("pokedex server: beautiful day to hunt pokemons...\n\n");
	printf("----------------OSADA filesystem...\n");
	osada_header * header_ptr = (osada_header *) osada_fs_ptr;
	printf("----------------id: %d\n----------------version: %d\n----------------file system blocks: %d\n----------------bitmap size:%d\n"
			"----------------start block mapping table:%d\n----------------data blocks:%d\n\n", header_ptr->magic_number,
			header_ptr->version, header_ptr->fs_blocks, header_ptr->allocations_table_offset, header_ptr->data_blocks);
	printf("pokedex server: waiting for clients...\n");
	return EXIT_SUCCESS;
}

int close_socket_connection(void) {
	close(listenning_socket);
	return EXIT_SUCCESS;
}

int map_osada_fs(void) {
	if ((fd = open(FS_PATH, O_RDWR)) == -1) {
		perror("open");
		exit(1);
	}
	if (stat(FS_PATH, &sbuf) == -1) {
		perror("stat");
		exit(1);
	}
	fd = open(FS_PATH, O_RDWR);
	osada_fs_ptr = mmap ((caddr_t) 0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (osada_fs_ptr == MAP_FAILED) {
		perror ("mmap");
		return 1;
	}
	return EXIT_SUCCESS;
}

int unmap_osada_fs(void) {
	if (munmap (osada_fs_ptr, sbuf.st_size) == -1) {
		perror ("munmap");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
