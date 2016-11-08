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

#define RES_MKDIR_OK 1
#define RES_MKNOD_OK 1
#define RES_READDIR_ISDIR 1
#define RES_READDIR_ISEMPTYDIR 2
#define RES_GETATTR_ISDIR 1
#define RES_GETATTR_ISREG 2
#define RES_GETATTR_ENOENT 3
#define RES_WRITE_OK 1
#define RES_READ_OK 1

int HEADER_SIZE, BITMAP_SIZE, MAPPING_TABLE_SIZE, DATA_SIZE;
int HEADER_0, HEADER_1, BITMAP_0, BITMAP_1, FILE_TABLE_0, FILE_TABLE_1, MAPPING_TABLE_0, MAPPING_TABLE_1, DATA_0, DATA_1;
int BM_HEADER_0, BM_HEADER_1, BM_BITMAP_0, BM_BITMAP_1, BM_FILE_TABLE_0, BM_FILE_TABLE_1, BM_MAPPING_TABLE_0, BM_MAPPING_TABLE_1, BM_DATA_0, BM_DATA_1;
uint16_t ROOT = 0xffff;
uint32_t END_OF_FILE = 0xffffffff;

t_config * conf;
int listenning_socket;
struct stat sbuf;
int fd;
void * osada_fs_ptr;
t_bitarray * bitmap;

int open_socket_connection(void);
int close_socket_connection(void);
int map_osada_fs(void);
int unmap_osada_fs(void);
int read_and_set(void);
void load_properties_file(void);
void closure (char *);

int search_dir(const char *, int);
int search_node(const char *, int);
int create_dir(const char *, int);
int create_node(const char *, int);

void process_request(int *);
void osada_mkdir(int *);
void osada_readdir(int *);
void osada_getattr(int *);
void osada_mknod(int *);
void osada_write(int *);
void osada_read(int *);

int main(int argc , char * argv[]) {
	load_properties_file();
	map_osada_fs();
	read_and_set();
	open_socket_connection();
	for (;;) {
		listen(listenning_socket, config_get_int_value(conf, "backlog")); // blocking syscall

		struct sockaddr_in addr;
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

	void * bitmap_ptr = (void *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * BITMAP_0));
	bitmap = bitarray_create_with_mode(bitmap_ptr, (BITMAP_SIZE * OSADA_BLOCK_SIZE), MSB_FIRST);

	printf("----------------OSADA filesystem...\n"
			"----------------id: %u\n"
			"----------------version: %d\n"
			"----------------file system blocks: %d blocks\n"
			"---------------------header size: %d\n"
			"---------------------bitmap size: %d\n"
			"---------------------file table size: %d\n"
			"---------------------mapping table size: %d\n"
			"---------------------data table size: %d\n"
			"----------------[tables 0(start) 1(end)]\n"
			"---------------------header 0: %d, 1: %d\n"
			"---------------------bitmap 0: %d, 1: %d\n"
			"---------------------file table 0: %d, 1: %d\n"
			"---------------------mapping table 0: %d, 1: %d\n"
			"---------------------data table 0: %d, 1: %d\n"
			"----------------[bitmap 0 (start) 1 (end)]\n"
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

void closure (char * dir) {
	free(dir);
}

int search_dir(const char * dir_name, int pb_pos) {
	int node_pos = search_node(dir_name, pb_pos);
	if (node_pos < 0) return node_pos;
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	file_table_ptr = file_table_ptr + node_pos;
	if (file_table_ptr->state == DIRECTORY) {
		return node_pos;
	} else {
		return -1;
	}
}

int search_node(const char * node_name, int pb_pos) {
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	int file_block_number = 0;
	int node_size, i;
	char * fname = malloc(sizeof(char) * (OSADA_FILENAME_LENGTH + 1));
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_table_ptr->state == REGULAR || file_table_ptr->state == DIRECTORY) {
			for (node_size = 0, i = 0; i < OSADA_FILENAME_LENGTH; i++) {
				if ((file_table_ptr->fname)[i] == '\0')
					break;
				node_size++;
			}
			memcpy(fname, (char *)(file_table_ptr->fname), node_size);
			fname[node_size] = '\0';
			if ((strcmp(fname, node_name) == 0) && file_table_ptr->parent_directory == pb_pos)
				break;
		}
		file_block_number++;
		file_table_ptr++;
	}
	free(fname);
	if (file_block_number > (FILE_BLOCKS_MOUNT - 1))
		return -1;
	return file_block_number;
}

int create_dir(const char * dir_name, int pb_pos) {
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	int file_block_number = 0;
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_table_ptr->state == DELETED)
			break;
		file_block_number++;
		file_table_ptr++;
	}

	osada_file * o_file = malloc(sizeof(osada_file));
	int dir_name_size = strlen(dir_name);
	memcpy((char *)(o_file->fname), dir_name, dir_name_size);
	if (dir_name_size < OSADA_FILENAME_LENGTH) o_file->fname[dir_name_size] = '\0';
	o_file->state = DIRECTORY;
	o_file->parent_directory = pb_pos;
	o_file->file_size = 0;
	o_file->lastmod = time(NULL);
	o_file->first_block = 0;
	memcpy(file_table_ptr, o_file, sizeof(osada_file));
	free(o_file);

	return file_block_number;
}

int create_node(const char * node_name, int pb_pos) {
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	int file_block_number = 0;
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_table_ptr->state == DELETED)
			break;
		file_block_number++;
		file_table_ptr++;
	}
	osada_file * o_file = malloc(sizeof(osada_file));
	int node_name_size = strlen(node_name);
	memcpy((char *)(o_file->fname), node_name, node_name_size);
	if (node_name_size < OSADA_FILENAME_LENGTH) o_file->fname[node_name_size] = '\0';
	o_file->state = REGULAR;
	o_file->parent_directory = pb_pos;
	o_file->file_size = 0;
	o_file->lastmod = time(NULL);
	o_file->first_block = END_OF_FILE;
	memcpy(file_table_ptr, o_file, sizeof(osada_file));
	free(o_file);

	return file_block_number;
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
		case 4:
			osada_mknod(client_socket);
			break;
		case 5:
			osada_write(client_socket);
			break;
		case 6:
			osada_read(client_socket);
			break;
		default:
			break;
		}
		close(* client_socket);
	}
}

void osada_mkdir(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: mkdir %s...\n", path);

	int ft_pos;
	int pb_pos = ROOT;
	char * dir = strtok(path,"/");
	while (dir != NULL) {
		ft_pos = search_dir(dir, pb_pos);
		if (ft_pos < 0) {
			pb_pos = create_dir(dir, pb_pos);
			break;
		} else {
			pb_pos = ft_pos;
		}
		dir = strtok(NULL, "/");
	}
	free(path);

	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_MKDIR_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);
}

void osada_readdir(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
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
			dir = strtok(NULL, "/");
		}
	}
	free(path);

	t_list * node_list = list_create();
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	char * node;
	int node_size, i;
	int file_block_number = 0, buffer_size = 0;
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if ((file_table_ptr->state == REGULAR || file_table_ptr->state == DIRECTORY) && file_table_ptr->parent_directory == pb_pos) {
			for (i = 0, node_size = 0; i < OSADA_FILENAME_LENGTH; i++) {
				if ((file_table_ptr->fname)[i] == '\0') break;
				node_size++;
			}
			node = malloc(sizeof(char) * (node_size + 1));
			memcpy(node, (file_table_ptr->fname), node_size);
			node[node_size] = '\0';
			buffer_size = buffer_size + node_size;
			list_add(node_list, node);
		}
		file_block_number++;
		file_table_ptr++;
	}

	if (node_list->elements_count > 0) {
		char * buffer = malloc(sizeof(char) * (buffer_size + node_list->elements_count + 1));
		int index = 0;
		node = list_get(node_list, index);
		node_size = strlen(node);
		memcpy(buffer, node, node_size);
		memcpy(buffer + node_size, ",", 1);
		buffer_size = node_size + 1;
		index++;
		node = list_get(node_list, index);
		while(node != NULL) {
			node_size = strlen(node);
			memcpy(buffer + buffer_size, node, node_size);
			memcpy(buffer + buffer_size + node_size, ",", 1);
			buffer_size = buffer_size + node_size + 1;
			index++;
			node = list_get(node_list, index);
		}
		buffer[buffer_size] = '\0';
		list_destroy_and_destroy_elements(node_list, &closure);

		// << sending response >>
		// response code
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = RES_READDIR_ISDIR;
		// response size
		uint32_t prot_resp_size = 4;
		uint32_t resp_size = (strlen(buffer) + 1);
		void * resp = malloc(prot_resp_code_size + prot_resp_size + resp_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		memcpy(resp + prot_resp_code_size, &resp_size, prot_resp_size);
		memcpy(resp + prot_resp_code_size + prot_resp_size, buffer, resp_size);
		write(* client_socket, resp, prot_resp_code_size + prot_resp_size + resp_size);
		free(resp);
		free(buffer);
	} else {
		list_destroy(node_list);

		// << sending response >>
		// response code
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = RES_READDIR_ISEMPTYDIR;
		void * resp = malloc(prot_resp_code_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		write(* client_socket, resp, prot_resp_code_size);
		free(resp);
	}
}

void osada_getattr(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: getattr %s\n", path);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		if (node_pos < 0) {
			// << sending response >>
			// response code
			uint8_t prot_resp_code_size = 1;
			uint8_t resp_code = RES_GETATTR_ENOENT;
			void * resp = malloc(prot_resp_code_size);
			memcpy(resp, &resp_code, prot_resp_code_size);
			write(* client_socket, resp, prot_resp_code_size);
			free(resp);
		}
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path);

	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	// file size
	uint8_t prot_resp_file_size = 4;
	uint32_t file_size = 0;
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	file_table_ptr = file_table_ptr + node_pos;
	if (file_table_ptr->state == DIRECTORY) {
		resp_code = RES_GETATTR_ISDIR;
	} else if (file_table_ptr->state == REGULAR){
		resp_code = RES_GETATTR_ISREG;
		file_size = file_table_ptr->file_size;
	}
	void * resp = malloc(prot_resp_code_size + prot_resp_file_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &file_size, prot_resp_file_size);
	write(* client_socket, resp, prot_resp_code_size + prot_resp_file_size);
	free(resp);

}

void osada_mknod(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	printf("pokedex server: mknod %s...\n", path);

	int ft_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		ft_pos = search_dir(node, pb_pos);
		if (ft_pos < 0) {
			pb_pos = create_node(node, pb_pos);
			break;
		} else {
			pb_pos = ft_pos;
		}
		node = strtok(NULL, "/");
	}
	free(path);

	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_MKNOD_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);

}

void osada_write(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	// buffer size
	uint8_t prot_buf_size = 4;
	uint32_t req_buf_size;
	if (recv(* client_socket, &req_buf_size, prot_buf_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// buffer
	char * buffer = malloc(sizeof(char) * (req_buf_size));
	if (recv(* client_socket, buffer, req_buf_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// size (amount of bytes to write)
	uint8_t prot_size = 4;
	uint32_t size;
	if (recv(* client_socket, &size, prot_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// offset
	uint8_t prot_offset = 4;
	uint32_t offset;
	if (recv(* client_socket, &offset, prot_offset, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	printf("pokedex server: write %s, size %d, offset %d\n", path, size, offset);

	// search file location
	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path);

	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos));

	// variables
	int fsize = node_ptr->file_size;
	osada_block_pointer * map_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
	osada_block_pointer * aux_map_ptr;
	int bytes_availables_in_block;
	int last_free_byte_pos;
	int movs;

	if ((offset + size) > fsize) {
		//
		// expand file
		//
		int bytes_to_expand = (offset + size) - fsize;
		node_ptr->file_size = fsize + bytes_to_expand;
		// mapping file
		int free_db = BM_DATA_0;
		bool its_busy;
		if (node_ptr->first_block == END_OF_FILE) {
			//
			// empty file
			//
			// assign first block
			while (free_db <= BM_DATA_1) {
				its_busy = bitarray_test_bit(bitmap, free_db);
				if (!its_busy) {
					bitarray_set_bit(bitmap, free_db);
					node_ptr->first_block = free_db - BM_DATA_0;
					aux_map_ptr = map_ptr + (node_ptr->first_block);
					* aux_map_ptr = END_OF_FILE;
					break;
				}
				free_db++;
			}
			if (free_db > BM_DATA_1) {
				// TODO full disk
			}
		}
		aux_map_ptr = map_ptr + (node_ptr->first_block);
		movs = 0;
		while ((*aux_map_ptr) != END_OF_FILE) {
			aux_map_ptr = map_ptr + (* aux_map_ptr);
			movs++;
		}
		last_free_byte_pos = fsize - (movs * OSADA_BLOCK_SIZE);
		bytes_availables_in_block = OSADA_BLOCK_SIZE - last_free_byte_pos;
		bytes_to_expand = bytes_to_expand - bytes_availables_in_block;
		free_db = BM_DATA_0;
		while (bytes_to_expand > 0 && free_db <= BM_DATA_1) {
			//
			// adding block
			//
			its_busy = bitarray_test_bit(bitmap, free_db);
			if (!its_busy) {
				bitarray_set_bit(bitmap, free_db);
				* aux_map_ptr = free_db - BM_DATA_0;
				aux_map_ptr = map_ptr + (* aux_map_ptr);
				* aux_map_ptr = END_OF_FILE;
				bytes_to_expand = bytes_to_expand - OSADA_BLOCK_SIZE;
			}
			free_db++;
		}
		if (free_db > BM_DATA_1) {
			// TODO full disk
		}
	}

	// positioning the map pointer to the first block (considering the offset)
	aux_map_ptr = map_ptr + (node_ptr->first_block);
	movs = offset / OSADA_BLOCK_SIZE;
	int i = movs;
	while (i > 0) {
		aux_map_ptr = map_ptr + (* aux_map_ptr);
		i--;
	}

	// writing bytes
	offset = offset - (OSADA_BLOCK_SIZE * movs);
	osada_block * data_ptr = (osada_block *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * DATA_0));
	char * aux_data_ptr = (char *)(data_ptr + (* aux_map_ptr) + offset);
	bytes_availables_in_block = OSADA_BLOCK_SIZE - offset;
	int bytes_to_write = size;
	int buff_pos = 0;
	int bytes_writing;

	while (bytes_to_write > 0) {
		bytes_writing = (bytes_to_write >= bytes_availables_in_block) ? bytes_availables_in_block : bytes_to_write;
		memcpy(aux_data_ptr, buffer + buff_pos, bytes_writing);
		bytes_to_write = bytes_to_write - bytes_writing;
		aux_map_ptr = map_ptr + (* aux_map_ptr);
		if ((* aux_map_ptr) == END_OF_FILE) break;
		aux_data_ptr  = (char *)(data_ptr + (* aux_map_ptr));
		buff_pos = buff_pos + bytes_writing;
		bytes_availables_in_block = OSADA_BLOCK_SIZE;
	}

	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_WRITE_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(buffer);
	free(resp);

}

void osada_read(int * client_socket) {
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	// size (amount of bytes to read)
	uint8_t prot_size = 4;
	uint32_t size;
	if (recv(* client_socket, &size, prot_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	// offset
	uint8_t prot_offset = 4;
	uint32_t offset;
	if (recv(* client_socket, &offset, prot_offset, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	printf("pokedex server: read %s, size %d, offset %d\n", path, size, offset);

	// search file location
	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path);

	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos));

	int file_size = (node_ptr->file_size);
	void * buff;

	if (offset < file_size) {
		if (offset + size > file_size)
			size = file_size - offset;

		// mapping file
		osada_block_pointer * map_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
		osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);
		osada_block * data_ptr = (osada_block *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * DATA_0));
		char * aux_data_ptr;

		// positioning the map pointer to the first block (considering the offset)
		int movs = offset / OSADA_BLOCK_SIZE;
		int i = movs;
		while (i > 0) {
			aux_map_ptr = map_ptr + (* aux_map_ptr);
			i--;
		}

		// getting bytes
		buff = malloc(size);
		int buff_pos = 0;
		offset = offset - (OSADA_BLOCK_SIZE * movs);
		int bytes_reading = ((OSADA_BLOCK_SIZE - offset) >= size) ? size : OSADA_BLOCK_SIZE - offset;

		aux_data_ptr = (char *)(data_ptr + (* aux_map_ptr));
		memcpy(buff, aux_data_ptr + offset, bytes_reading);
		buff_pos = buff_pos + bytes_reading;
		int bytes_to_read = size - bytes_reading;
		aux_map_ptr = map_ptr + (* aux_map_ptr);

		while ((*aux_map_ptr) != END_OF_FILE && bytes_to_read > 0) {
			aux_data_ptr = (char *)(data_ptr + (* aux_map_ptr));
			bytes_reading = (bytes_to_read >= OSADA_BLOCK_SIZE) ? OSADA_BLOCK_SIZE : bytes_to_read;
			memcpy(buff + buff_pos, aux_data_ptr, bytes_reading);
			buff_pos = buff_pos + bytes_reading;
			bytes_to_read = bytes_to_read - bytes_reading;
			aux_map_ptr = map_ptr + (* aux_map_ptr);
		}
	}

	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_READ_OK;
	// bytes transferred
	uint8_t prot_bytes_transferred_size = 4;
	void * resp = malloc(prot_resp_code_size + prot_bytes_transferred_size + size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &size, prot_bytes_transferred_size);
	// content
	if (size > 0) {
		memcpy(resp + prot_resp_code_size + prot_bytes_transferred_size, buff, size);
	}
	write(* client_socket, resp, prot_resp_code_size + prot_bytes_transferred_size + size);
	free(resp);
	free(buff);
}
