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
#define RES_TRUNCATE_OK 1

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
void osada_truncate(int *);

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
			"----------------[tables 0(ini) 1(end)]\n"
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
			memcpy(fname, (file_table_ptr->fname), node_size);
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
	o_file->state = REGULAR;
	strcpy((char *)(o_file->fname), node_name);
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
		case 7:
			osada_truncate(client_socket);
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
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_MKDIR_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);
}

void osada_readdir(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
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
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = RES_READDIR_ISDIR;
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
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = RES_READDIR_ISEMPTYDIR;
		void * resp = malloc(prot_resp_code_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		write(* client_socket, resp, prot_resp_code_size);
		free(resp);
	}
}

void osada_getattr(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
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
	// cod resp
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	// file size
	uint8_t prot_file_size = 4;
	uint32_t file_size = 0;
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	file_table_ptr = file_table_ptr + node_pos;
	if (file_table_ptr->state == DIRECTORY) {
		resp_code = RES_GETATTR_ISDIR;
	} else if (file_table_ptr->state == REGULAR){
		resp_code = RES_GETATTR_ISREG;
		file_size = file_table_ptr->file_size;
	}
	void * resp = malloc(prot_resp_code_size + prot_file_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &file_size, prot_file_size);
	write(* client_socket, resp, prot_resp_code_size + prot_file_size);
	free(resp);

}

void osada_mknod(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
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
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_MKNOD_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(resp);

}

void osada_write(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	uint8_t prot_buf_size = 4;
	uint32_t req_buf_size;
	if (recv(* client_socket, &req_buf_size, prot_buf_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * buf = malloc(sizeof(char) * (req_buf_size));
	if (recv(* client_socket, buf, req_buf_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	uint8_t prot_size = 4;
	uint32_t size;
	if (recv(* client_socket, &size, prot_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	uint8_t prot_offset = 4;
	uint32_t offset;
	if (recv(* client_socket, &offset, prot_offset, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	printf("pokedex server: write %s\n", path);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}

	int n_blocks = size / OSADA_BLOCK_SIZE;
	if (n_blocks == 0 || (size % OSADA_BLOCK_SIZE) > 1) n_blocks++;
	int mapping[n_blocks];

	int index = 0;
	int free_db = BM_DATA_0;
	bool its_busy;
	while ((index <= n_blocks - 1) && (free_db <= BM_DATA_1)) {
		its_busy = bitarray_test_bit(bitmap, free_db);
		if (!its_busy) {
			mapping[index] = free_db - BM_DATA_0;
			bitarray_set_bit(bitmap, free_db);
			index++;
		}
		free_db++;
	}

	if (free_db > BM_MAPPING_TABLE_1) {
		// TODO full disk
	}

	osada_file * node_ptr = (osada_file *) osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos);
	node_ptr->file_size = size;
	node_ptr->lastmod = time(NULL);
	node_ptr->first_block = mapping[0];

	osada_block_pointer * ob_mapp_ptr;
	osada_block * ob_ptr;
	index = 0;
	while (index <= (n_blocks - 1)) {
		ob_mapp_ptr = (osada_block_pointer *) osada_fs_ptr + (OSADA_BLOCK_SIZE * (MAPPING_TABLE_0 + mapping[index]));
		if (index == n_blocks-1) {
			memcpy(ob_mapp_ptr, &END_OF_FILE, OSADA_MAPP_SIZE);
		} else {
			memcpy(ob_mapp_ptr, &mapping[index+1], OSADA_MAPP_SIZE);
		}
		ob_ptr = (osada_block *) osada_fs_ptr + (OSADA_BLOCK_SIZE * (DATA_0 + mapping[index]));
		memcpy(ob_ptr, buf + (index * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
		index++;
	}

	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_WRITE_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(path);
	free(buf);
	free(resp);
}

void osada_truncate(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	uint8_t prot_size = 4;
	uint32_t new_size;
	if (recv(* client_socket, &new_size, prot_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	printf("pokedex server: truncate %s, size %d\n", path, new_size);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}

	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos));
	int old_size = node_ptr->file_size;

	if (new_size > old_size) {
		//
		// adding file blocks
		//
		// getting the number of blocks that are necessary
		int n_blocks = new_size / OSADA_BLOCK_SIZE;
		if (n_blocks == 0 || (old_size % OSADA_BLOCK_SIZE) > 1) n_blocks++;
		int mapping[n_blocks];
		// pointing to the first block
		osada_block_pointer * ob_mapp_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
		osada_block_pointer * aux_ptr = ob_mapp_ptr + (node_ptr->first_block);
		// mapping old blocks
		int index = 0;
		mapping[index] = (node_ptr->first_block);
		index++;
		while ((*aux_ptr) != END_OF_FILE) {
			mapping[index] = (*aux_ptr);
			index++;
			aux_ptr = ob_mapp_ptr + (*aux_ptr);
		}
		// adding new blocks
		int free_db = BM_DATA_0;
		bool its_busy;
		while ((index <= n_blocks - 1) && (free_db <= BM_DATA_1)) {
			its_busy = bitarray_test_bit(bitmap, free_db);
			if (!its_busy) {
				mapping[index] = free_db - BM_DATA_0;
				index++;
			}
			free_db++;
		}
		if (free_db > BM_MAPPING_TABLE_1) {
			//
			// TODO full disk
			//
		} else {
			int i;
			for (i = 0; i <= (n_blocks - 1); i++) {
				bitarray_set_bit(bitmap, (BM_DATA_0 + mapping[i]));
			}
		}
	} else if (new_size < old_size) {
		//
		// removing file blocks
		//
		// getting the old number of blocks that were necessary
		int n_blocks = old_size / OSADA_BLOCK_SIZE;
		if (n_blocks == 0 || (old_size % OSADA_BLOCK_SIZE) > 1) n_blocks++;
		int mapping[n_blocks];
		// mapping old file
		osada_block_pointer * ob_mapp_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
		osada_block_pointer * aux_ptr = ob_mapp_ptr + (node_ptr->first_block);
		// mapping old blocks
		int index = 0;
		mapping[index] = (node_ptr->first_block);
		index++;
		while ((*aux_ptr) != END_OF_FILE) {
			mapping[index] = (*aux_ptr);
			index++;
			aux_ptr = ob_mapp_ptr + (*aux_ptr);
		}
		// releasing remaining blocks
		// getting the number of blocks that are necessary
		n_blocks = new_size / OSADA_BLOCK_SIZE;
		if ((new_size % OSADA_BLOCK_SIZE) > 1) n_blocks++;
		while(index > (n_blocks - 1)) {
			bitarray_clean_bit(bitmap, (BM_DATA_0 + mapping[index]));
			index--;
		}
		aux_ptr = ob_mapp_ptr + mapping[index];
		* aux_ptr = END_OF_FILE;
	}

	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_TRUNCATE_OK;
	void * resp = malloc(prot_resp_code_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, prot_resp_code_size);
	free(path);
	free(resp);

}

void osada_read(int * client_socket) {
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	if (recv(* client_socket, &req_path_size, prot_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	if (recv(* client_socket, path, req_path_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	path[req_path_size] = '\0';
	uint8_t prot_size = 4;
	uint32_t size;
	if (recv(* client_socket, &size, prot_size, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	uint8_t prot_offset = 4;
	uint32_t offset;
	if (recv(* client_socket, &offset, prot_offset, 0) <= 0) {
		printf("pokedex server: client %d disconnected...\n", * client_socket);
	}
	printf("pokedex server: read %s, size %d, offset %d\n", path, size, offset);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path,"/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path);

	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos));
	int file_size = (node_ptr->file_size);

	int bytes_transferred = 0;
	// TODO change to void *
	char * buff;

	if (offset < file_size) {
		if (offset + size > file_size)
			size = file_size - offset;

		// getting the number of blocks that are necessary
		int n_blocks = file_size / OSADA_BLOCK_SIZE;
		if (n_blocks == 0 || (file_size % OSADA_BLOCK_SIZE) > 1) n_blocks++;
		int mapping[n_blocks];
		// mapping file
		osada_block_pointer * ob_mapp_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
		osada_block_pointer * aux_ptr = ob_mapp_ptr + (node_ptr->first_block);
		// mapping blocks
		int index = 0;
		mapping[index] = (node_ptr->first_block);
		index++;
		while ((*aux_ptr) != END_OF_FILE) {
			mapping[index] = (*aux_ptr);
			index++;
			aux_ptr = ob_mapp_ptr + (*aux_ptr);
		}

		// getting bytes
		buff = malloc(size);
		osada_block * data_ptr = (osada_block *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * DATA_0));

		osada_block * aux_data_ptr;
		int buff_index = 0;
		int pending = size;
		int to_transfer;
		index = offset / OSADA_BLOCK_SIZE;
		do {
			aux_data_ptr = data_ptr + mapping[index];
			if (pending >= OSADA_BLOCK_SIZE) {
				to_transfer = OSADA_BLOCK_SIZE;
				pending = pending - OSADA_BLOCK_SIZE;
			} else {
				to_transfer = pending;
				pending = 0;
			}
			memcpy(buff + buff_index, aux_data_ptr, to_transfer);
			buff_index = buff_index + to_transfer;
			bytes_transferred = bytes_transferred + to_transfer;
			index++;
		} while (bytes_transferred < size);

	}

	// << sending response >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = RES_READ_OK;
	uint8_t prot_bytes_transferred_size = 4;
	void * resp = malloc(prot_resp_code_size + prot_bytes_transferred_size + bytes_transferred);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &bytes_transferred, prot_bytes_transferred_size);
	if (bytes_transferred > 0) {
		memcpy(resp + prot_resp_code_size + prot_bytes_transferred_size, buff, bytes_transferred);
	}
	write(* client_socket, resp, prot_resp_code_size + prot_bytes_transferred_size + bytes_transferred);
	free(resp);
	free(buff);
}

