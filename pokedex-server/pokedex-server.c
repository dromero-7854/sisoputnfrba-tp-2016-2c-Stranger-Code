/*
 * pokedex-server.c
 *
 *  Created on: 18/9/2016
 *      Author: Dante Romero
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
#include <commons/config.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <unistd.h>
#include <thread_db.h>
#include "osada.h"

int HEADER_SIZE;
int BITMAP_SIZE;
int MAPPING_TABLE_SIZE;
int DATA_SIZE;

int HEADER_0;
int HEADER_1;
int BITMAP_0;
int BITMAP_1;
int FILE_TABLE_0;
int FILE_TABLE_1;
int MAPPING_TABLE_0;
int MAPPING_TABLE_1;
int DATA_0;
int DATA_1;

int BM_HEADER_0;
int BM_HEADER_1;
int BM_BITMAP_0;
int BM_BITMAP_1;
int BM_FILE_TABLE_0;
int BM_FILE_TABLE_1;
int BM_MAPPING_TABLE_0;
int BM_MAPPING_TABLE_1;
int BM_DATA_0;
int BM_DATA_1;

int ROOT = 0xFFFF;

t_config * conf; 		// properties file
int listenning_socket;	// socket connection
struct stat sbuf; 		// osada filesystem
int fd;					// osada filesystem
void * osada_fs_ptr;	// osada filesystem >> pointer to the first block

t_bitarray * bitmap;

#define RES_MKDIR_OK 1
#define RES_READDIR_ISDIR 1
#define RES_READDIR_ISEMPTYDIR 2
#define RES_GETATTR_ISDIR 1
#define RES_GETATTR_ENOTDIR 2

int open_socket_connection(void);
int close_socket_connection(void);
int map_osada_fs(void);
int unmap_osada_fs(void);
int read_and_set(void);
void load_properties_file(void);


void process_request(int *);
//mkdir
void osada_mkdir(int *);
int search_dir(const char *, int);
int create_dir(const char *, int);
//reddir
int osada_readdir(int *);
void closure (char *);
//getattr
int osada_getattr(int *);

int main(int argc , char * argv[]) {
	load_properties_file();
	open_socket_connection();
	map_osada_fs();
	read_and_set();
	for (;;) {
		listen(listenning_socket, config_get_int_value(conf, "backlog")); // blocking syscall

		struct sockaddr_in addr; // client data (ip, port, etc.)
		socklen_t addrlen = sizeof(addr);
		int client_socket = accept(listenning_socket, (struct sockaddr *) &addr, &addrlen);

		printf("pokedex server: hi client %d!!\n", client_socket);
		pthread_attr_t attr;
		pthread_t thread;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread, &attr, &process_request, &client_socket);
		pthread_attr_destroy(&attr);
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

	getaddrinfo(NULL, config_get_string_value(conf, "port"), &hints, &server_info);

	listenning_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	bind(listenning_socket,server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);
	return EXIT_SUCCESS;
}

int read_and_set(void) {
	printf("pokedex server: welcome to pokedex-server 1.0v!...\n");
	printf("pokedex server: beautiful day to hunt pokemons...\n\n\n\n");
	osada_header * header_ptr = (osada_header *) osada_fs_ptr;

	HEADER_SIZE = 1;
	BITMAP_SIZE = (header_ptr->fs_blocks / 8) / OSADA_BLOCK_SIZE;
	MAPPING_TABLE_SIZE = 1 + ((((header_ptr->fs_blocks - HEADER_SIZE - BITMAP_SIZE - FILE_TABLE_SIZE) * 4) - 1) / OSADA_BLOCK_SIZE);
	DATA_SIZE = header_ptr->fs_blocks - HEADER_SIZE - BITMAP_SIZE - FILE_TABLE_SIZE - MAPPING_TABLE_SIZE;

	HEADER_0 = 0;
	HEADER_1 = 0;
	BITMAP_0 = 1;
	BITMAP_1 = BITMAP_0 + (BITMAP_SIZE - 1);
	FILE_TABLE_0 = BITMAP_SIZE + 1;
	FILE_TABLE_1 = FILE_TABLE_0 + (FILE_TABLE_SIZE - 1);
	MAPPING_TABLE_0 = FILE_TABLE_0 + FILE_TABLE_SIZE;
	MAPPING_TABLE_1 = MAPPING_TABLE_0 + (MAPPING_TABLE_SIZE - 1);
	DATA_0 = MAPPING_TABLE_0 + MAPPING_TABLE_SIZE;
	DATA_1 = DATA_0 + (DATA_SIZE - 1);

	BM_HEADER_0 = 0;
	BM_HEADER_1 = 0;
	BM_BITMAP_0 = BM_HEADER_1 + 1;
	BM_BITMAP_1 = BM_BITMAP_0 + (BITMAP_SIZE - 1);
	BM_FILE_TABLE_0 = BM_BITMAP_1 + 1;
	BM_FILE_TABLE_1 = BM_FILE_TABLE_0 + (FILE_TABLE_SIZE - 1);
	BM_MAPPING_TABLE_0 = BM_FILE_TABLE_1 + 1;
	BM_MAPPING_TABLE_1 = BM_MAPPING_TABLE_0 + (MAPPING_TABLE_SIZE - 1);
	BM_DATA_0 = BM_MAPPING_TABLE_1 + 1;
	BM_DATA_1 = BM_DATA_0 + (DATA_SIZE - 1);

	void * bitmap_ptr = (void *) osada_fs_ptr + OSADA_BLOCK_SIZE + BITMAP_0;
	bitmap = bitarray_create(bitmap_ptr, (BITMAP_SIZE * OSADA_BLOCK_SIZE));

	printf("----------------OSADA filesystem...\n"
			"----------------id: %d\n"
			"----------------version: %d\n"
			"----------------file system blocks: %d blocks\n"
			"---------------------header size: %d\n"
			"---------------------bitmap size: %d\n"
			"---------------------file table size: %d\n"
			"---------------------mapping table size: %d\n"
			"---------------------data table size: %d\n"
			"----------------[tables 0(ini) 1(end)] index \n"
			"---------------------header 0: %d, 1: %d\n"
			"---------------------bitmap 0: %d, 1: %d\n"
			"---------------------file table 0: %d, 1: %d\n"
			"---------------------mapping table 0: %d, 1: %d\n"
			"---------------------data table 0: %d, 1: %d\n"
			"----------------[bitmap 0 (ini) 1 (end)]\n"
			"---------------------header 0: %d, 1: %d\n"
			"---------------------bitmap 0: %d, 1: %d\n"
			"---------------------file table 0: %d, 1: %d\n"
			"---------------------mapping table 0: %d, 1: %d\n"
			"---------------------data table 0: %d, 1: %d\n\n\n\n",
			header_ptr->magic_number, header_ptr->version, header_ptr->fs_blocks,
			HEADER_SIZE, BITMAP_SIZE, FILE_TABLE_SIZE, MAPPING_TABLE_SIZE, DATA_SIZE,
			HEADER_0, HEADER_1, BITMAP_0, BITMAP_1, FILE_TABLE_0, FILE_TABLE_1, MAPPING_TABLE_0, MAPPING_TABLE_1, DATA_0, DATA_1,
			BM_HEADER_0, BM_HEADER_1, BM_BITMAP_0, BM_BITMAP_1, BM_FILE_TABLE_0, BM_FILE_TABLE_1,
			BM_MAPPING_TABLE_0, BM_MAPPING_TABLE_1, BM_DATA_0, BM_DATA_1);

	printf("pokedex server: waiting for clients...\n");
	return EXIT_SUCCESS;
}

void load_properties_file(void) {
	conf = config_create("./conf/pokedex-server.properties");
}

int close_socket_connection(void) {
	close(listenning_socket);
	return EXIT_SUCCESS;
}

int map_osada_fs(void) {
	char * fs_path = config_get_string_value(conf, "fs.path");
	if ((fd = open(fs_path, O_RDWR)) == -1) {
		perror("open");
		exit(1);
	}
	if (stat(fs_path, &sbuf) == -1) {
		perror("stat");
		exit(1);
	}
	fd = open(fs_path, O_RDWR);
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

void process_request(int * client_socket) {
	uint8_t op_code;
	uint8_t prot_ope_code_size = 1;
	if (recv(* client_socket, &op_code, prot_ope_code_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	} else {
		switch (op_code) {
		case 1:
			osada_mkdir(client_socket);
			break;
		case 2:
			osada_readdir(client_socket);
			break;
		case 3:
			osada_getattr(client_socket);
			break;
		default:
			break;
		}
		close(* client_socket);
	}
}

void osada_mkdir(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(req_path_size + 1);
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: mkdir %s...\n", path);

	int ft_pos;
	int pb_pos = ROOT; // root
	char * dir = strtok(path,"/");
	while (dir != NULL) {
		ft_pos = search_dir(dir, pb_pos);
		if (ft_pos < 0) {
			pb_pos = create_dir(dir, pb_pos);
		} else {
			pb_pos = ft_pos;
		}
		dir = strtok (NULL, "/");
	}
	free(path);
	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_MKDIR_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);
}

int search_dir(const char * dir_name, int pb_pos) {
	osada_file * file_table_ptr = (osada_file *) osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0);
	int file_block_number = 1;
	while (file_block_number <= FILE_BLOCKS_MOUNT) {
		if (file_table_ptr->state == DIRECTORY  && file_table_ptr->parent_directory == pb_pos
				&& (strcmp(file_table_ptr->fname, dir_name) == 0)) {
			break;
		} else {
			file_block_number++;
			file_table_ptr = file_table_ptr + OSADA_FILE_BLOCK_SIZE;
		}
	}
	if (file_block_number > FILE_BLOCKS_MOUNT)
		return -1;
	return file_block_number;
}

int create_dir(const char * dir_name, int pb_pos) {
	osada_file * file_table_ptr = (osada_file *) osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0);
	int file_block_number = 1;
	while (file_block_number <= FILE_BLOCKS_MOUNT) {
		if (file_table_ptr->state == REGULAR || file_table_ptr->state == DIRECTORY) {
			file_block_number++;
			file_table_ptr = file_table_ptr + OSADA_FILE_BLOCK_SIZE;
		} else {
			break;
		}
	}
	osada_file * o_file = malloc(sizeof(osada_file));
	o_file->state = DIRECTORY;
	strcpy(o_file->fname, dir_name);
	o_file->parent_directory = pb_pos;
	o_file->file_size = 0;
	o_file->lastmod = time(NULL);
	o_file->first_block = 0;
	memcpy(file_table_ptr, o_file, OSADA_FILE_BLOCK_SIZE);
	free(o_file);
	return file_block_number;
}

int osada_readdir(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(req_path_size + 1);
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: readdir %s\n", path);

	int pb_pos = ROOT;
	if (strcmp(path, "/") != 0) {
		char * dir = strtok(path,"/");
		while (dir != NULL) {
			pb_pos = search_dir(dir, pb_pos);
			dir = strtok (NULL, "/");
		}
	}
	free(path);

	t_list * dir_list = list_create();
	osada_file * file_table_ptr = (osada_file *) osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0);

	char * dir;
	int dir_size;
	int buffer_size = 0;
	int file_block_number = 1;
	while (file_block_number <= FILE_BLOCKS_MOUNT) {
		if (file_table_ptr->state == DIRECTORY && file_table_ptr->parent_directory == pb_pos) {
			dir_size = strlen(file_table_ptr->fname);
			buffer_size = buffer_size + dir_size;
			dir = malloc(sizeof(char) * (dir_size + 1));
			strcpy(dir, file_table_ptr->fname);
			list_add(dir_list, dir);
		}
		file_block_number++;
		file_table_ptr = file_table_ptr + OSADA_FILE_BLOCK_SIZE;
	}

	if (dir_list->elements_count > 0) {
		char * buffer = malloc(buffer_size + dir_list->elements_count + 1);
		int index = 0;
		dir = list_get(dir_list, index);
		strcat(dir,",");
		strcpy(buffer, dir);
		index++;
		dir = list_get(dir_list, index);
		while(dir != NULL) {
			strcat(dir,",");
			strcat(buffer, dir);
			index++;
			dir = list_get(dir_list, index);
		}
		list_destroy_and_destroy_elements(dir_list, &closure);

		// << sending response >>
		uint8_t prot_resp_code_size = 1;
		uint32_t prot_resp_size = 4;
		uint8_t resp_code = RES_READDIR_ISDIR;
		uint32_t resp_size = (strlen(buffer) + 1);
		void * resp = malloc(prot_resp_code_size + prot_resp_size + resp_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		memcpy(resp + prot_resp_code_size, &resp_size, prot_resp_size);
		memcpy(resp + prot_resp_code_size + prot_resp_size, buffer, resp_size);
		write(* client_socket, resp, prot_resp_code_size + prot_resp_size + resp_size);

		free(resp);
		free(buffer);
	} else {
		list_destroy(dir_list);
		// << sending response >>
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = RES_READDIR_ISEMPTYDIR;
		void * resp = malloc(prot_resp_code_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		write(* client_socket, resp, prot_resp_code_size);
		free(resp);
	}
	return 1;
}

void closure (char * dir) {
	free(dir);
}

int osada_getattr(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(req_path_size + 1);
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: getattr %s\n", path);

	int ft_pos;
	int pb_pos = ROOT; // root
	char * dir = strtok(path,"/");
	while (dir != NULL) {
		ft_pos = search_dir(dir, pb_pos);
		if (ft_pos < 0) {
			// << sending response >>
			uint8_t prot_resp_code_size = 1;
			uint8_t resp_code = RES_GETATTR_ENOTDIR;
			void * resp = malloc(prot_resp_code_size);
			memcpy(resp, &resp_code, prot_resp_code_size);
			write(* client_socket, resp, prot_resp_code_size);
			free(resp);
			return 1;
		}
		pb_pos = ft_pos;
		dir = strtok (NULL, "/");
	}
	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_GETATTR_ISDIR;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);
	return 1;
}
