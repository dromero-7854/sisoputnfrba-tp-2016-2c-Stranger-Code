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
#include <commons/log.h>
#include <commons/string.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <unistd.h>
#include <thread_db.h>
#include <libgen.h>
#include "osada.h"

#define	LOCK_READ 0
#define	LOCK_WRITE 1
#define	UNLOCK 2

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
t_log * logger;
t_log * semaphore_logger;
pthread_rwlock_t * locks;
pthread_mutex_t m_lock;

int close_socket_connection(void);
int map_osada_fs(void);
int unmap_osada_fs(void);
int read_and_set(void);
int init_locks(void);
void load_properties_file(void);
void create_logger(void);
void closure (char *);
void unlock_block(int *);
void semaphore (int, int);

int search_dir(const char *, int, int, int);
int search_node(const char *, int, int, int);
int get_free_file_block(int);
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
void osada_unlink(int *);
void osada_rmdir(int *);
void osada_rename(int *);

int main(int argc , char * argv[]) {
	load_properties_file();
	create_logger();
	map_osada_fs();
	read_and_set();
	init_locks();

	int client_sock, c, * new_sock;
	struct sockaddr_in server, client;
	listenning_socket = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(config_get_int_value(conf, "port"));

	bind(listenning_socket, (struct sockaddr *) &server, sizeof(server));
	listen(listenning_socket, config_get_int_value(conf, "backlog"));
	c = sizeof(struct sockaddr_in);

	for (;;) {

		client_sock = accept(listenning_socket, (struct sockaddr *) &client, (socklen_t *) &c);

		new_sock = malloc(1);
		* new_sock = client_sock;

		pthread_attr_t attr;
		pthread_t thread;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread, &attr, &process_request, (void *) new_sock);
		pthread_attr_destroy(&attr);
	}
	close_socket_connection();
	free(locks);
	pthread_mutex_destroy(&m_lock);
	return EXIT_SUCCESS;
}

int init_locks() {
	int i;
	locks = (pthread_rwlock_t *) malloc(FILE_BLOCKS_MOUNT * sizeof(pthread_rwlock_t));

	for (i = 0 ;i < FILE_BLOCKS_MOUNT; i++) {
		if (pthread_rwlock_init(locks + i, NULL) != 0) {
			log_error(semaphore_logger, "Error starting semaphore %d", i);
			exit(1);
		}
	}

	pthread_mutex_init(&m_lock, NULL);

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
	log_info(logger, "welcome to pokedex-server 1.0v!...");
	log_info(logger, "beautiful day to hunt pokemons...");
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

	log_info(logger, "----------------OSADA filesystem...");
	log_info(logger, "----------------id: %u", (header_ptr->magic_number));
	log_info(logger, "----------------version: %d", (header_ptr->version));
	log_info(logger, "----------------file system blocks: %d blocks", (header_ptr->fs_blocks));
	log_info(logger, "---------------------header size: %d", HEADER_SIZE);
	log_info(logger, "---------------------bitmap size: %d", BITMAP_SIZE);
	log_info(logger, "---------------------file table size: %d", FILE_TABLE_SIZE);
	log_info(logger, "---------------------mapping table size: %d", MAPPING_TABLE_SIZE);
	log_info(logger, "---------------------data table size: %d", DATA_SIZE);
	log_info(logger, "----------------[tables 0(start) 1(end)]");
	log_info(logger, "---------------------header 0: %d, 1: %d", HEADER_0, HEADER_1);
	log_info(logger, "---------------------bitmap 0: %d, 1: %d", BITMAP_0, BITMAP_1);
	log_info(logger, "---------------------file table 0: %d, 1: %d", FILE_TABLE_0, FILE_TABLE_1);
	log_info(logger, "---------------------mapping table 0: %d, 1: %d", MAPPING_TABLE_0, MAPPING_TABLE_1);
	log_info(logger, "---------------------data table 0: %d, 1: %d", DATA_0, DATA_1);
	log_info(logger, "----------------[bitmap 0 (start) 1 (end)]");
	log_info(logger, "---------------------header 0: %d, 1: %d", BM_HEADER_0, BM_HEADER_1);
	log_info(logger, "---------------------bitmap 0: %d, 1: %d", BM_BITMAP_0, BM_BITMAP_1);
	log_info(logger, "---------------------file table 0: %d, 1: %d", BM_FILE_TABLE_0, BM_FILE_TABLE_1);
	log_info(logger, "---------------------mapping table 0: %d, 1: %d", BM_MAPPING_TABLE_0, BM_MAPPING_TABLE_1);
	log_info(logger, "---------------------data table 0: %d, 1: %d", BM_DATA_0, BM_DATA_1);
	log_info(logger, "waiting for clients...");
	return EXIT_SUCCESS;
}

void load_properties_file(void) {
	conf = config_create("./conf/pokedex-server.properties");
}

void create_logger(void) {
	char * file = config_get_string_value(conf, "filelog.name");
	char * semaphore_file = config_get_string_value(conf, "filelog.semaphore.name");
	logger = log_create(file, "pokedex-server", true, LOG_LEVEL_TRACE);
	semaphore_logger = log_create(semaphore_file, "pokedex-server-semaphores", true, LOG_LEVEL_TRACE);
}

void closure(char * dir) {
	free(dir);
}

void unlock_block(int * ptr_file_block_number) {
	semaphore(UNLOCK, * ptr_file_block_number);
	free(ptr_file_block_number);
}

void semaphore(int action, int pos) {
	switch (action) {
	case LOCK_READ :
		pthread_rwlock_rdlock(locks + pos);
		break;
	case LOCK_WRITE :
		pthread_rwlock_wrlock(locks + pos);
		break;
	case UNLOCK :
		pthread_rwlock_unlock(locks + pos);
		break;
	}
}

int search_dir(const char * dir_name, int pb_pos, int lock_type, int ignore_pos) {
	int node_pos = search_node(dir_name, pb_pos, lock_type, ignore_pos );
	if (node_pos == -OSADA_ENOENT) // no such  file or directory
		return -OSADA_ENOTDIR; // not a directory
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	file_table_ptr = file_table_ptr + node_pos;
	if (file_table_ptr->state == DIRECTORY) {
		return node_pos; // locked block
	} else {
		semaphore(UNLOCK, node_pos);
		return -OSADA_ENOTDIR; // not a directory
	}
}

int search_node(const char * node_name, int pb_pos, int lock_type, int ignore_pos) {

	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	int file_block_number = 0;
	int node_size, i;
	char * fname = malloc(sizeof(char) * (OSADA_FILENAME_LENGTH + 1));
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_block_number != pb_pos && file_block_number != ignore_pos) {
			semaphore(lock_type, file_block_number);
			if (file_table_ptr->state == REGULAR || file_table_ptr->state == DIRECTORY) {
				for (node_size = 0, i = 0; i < OSADA_FILENAME_LENGTH; i++) {
					if ((file_table_ptr->fname)[i] == '\0')
						break;
					node_size++;
				}
				memcpy(fname, (char *)(file_table_ptr->fname), node_size);
				fname[node_size] = '\0';
				if ((strcmp(fname, node_name) == 0) && file_table_ptr->parent_directory == pb_pos){
					break;
				}
			}
			semaphore(UNLOCK, file_block_number);
		}
		file_block_number++;
		file_table_ptr++;
	}
	free(fname);

	if (file_block_number > (FILE_BLOCKS_MOUNT - 1))
		return -OSADA_ENOENT; // no such file or directory

	return file_block_number; // located block is locked
}

int get_free_file_block(int pb_pos) {

	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	int file_block_number = 0;
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_block_number != pb_pos) {
			semaphore(LOCK_WRITE, file_block_number);
			if (file_table_ptr->state == DELETED)
				break; // available block is locked
			semaphore(UNLOCK, file_block_number);
		}
		file_block_number++;
		file_table_ptr++;
	}
	return file_block_number;
}

int create_dir(const char * dir_name, int pb_pos) {

	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	int file_block_number = get_free_file_block(pb_pos);
	if (file_block_number > (FILE_BLOCKS_MOUNT - 1))
		return -OSADA_ENOSPC; // no space left on device

	file_table_ptr = file_table_ptr + file_block_number;
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

	semaphore(UNLOCK, file_block_number);
	return file_block_number;
}

int create_node(const char * node_name, int pb_pos) {

	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	int file_block_number = get_free_file_block(pb_pos);
	if (file_block_number > (FILE_BLOCKS_MOUNT - 1))
		return -OSADA_ENOSPC; // no space left on device

	file_table_ptr = file_table_ptr + file_block_number;
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

	semaphore(UNLOCK, file_block_number);
	return file_block_number;
}

void process_request(int * client_socket) {
	uint8_t op_code;
	uint8_t prot_ope_code_size = 1;
	int received_bytes = recv(* client_socket, &op_code, prot_ope_code_size, MSG_WAITALL);
	while (received_bytes > 0) {
		log_info("client %d >> OPERATION CODE %d", * client_socket, op_code);
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
		case 8:
			osada_unlink(client_socket);
			break;
		case 9:
			osada_rmdir(client_socket);
			break;
		case 10:
			osada_rename(client_socket);
			break;
		default:;
		}
		received_bytes = recv(* client_socket, &op_code, prot_ope_code_size, MSG_WAITALL);
	}
	log_error(logger, "client %d disconnected...", * client_socket);
	close(* client_socket);
	free(client_socket);
	return;
}

void osada_mkdir(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, mkdir %s...", * client_socket, path);

	if (strcmp(path, "/") != 0) {

		// creating a copy to work with strtok (it modifies the original str)
		char * path_c = malloc(sizeof(char) * (req_path_size + 1));
		strcpy(path_c, path);

		int dir_pos;
		int pb_pos = ROOT;
		char * dir = strtok(path_c, "/");
		while (dir != NULL) {
			dir_pos = search_dir(dir, pb_pos, LOCK_READ, pb_pos);
			if (dir_pos == -OSADA_ENOTDIR) {
				if (strlen(dir) > OSADA_FILENAME_LENGTH) {
					log_error(logger, "client %d, mkdir '%s', creating directory '%s' : name too long", * client_socket, path, dir);
					if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENAMETOOLONG; // name too long

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, prot_resp_code_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
				dir_pos = create_dir(dir, pb_pos);
				if (pb_pos == -OSADA_ENOSPC) {
					log_error(logger, "client %d, mkdir '%s', creating directory '%s' : no space left on device", * client_socket, path, dir);
					if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOSPC; // no space left on device

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, prot_resp_code_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
			}
			if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
			pb_pos = dir_pos;
			dir = strtok(NULL, "/");
		}
		free(path_c);
	}
	free(path);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_ISDIR; // is a directory

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);
}

void osada_readdir(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, readdir %s", * client_socket, path);

	int dir_pos;
	int pb_pos = ROOT;
	if (strcmp(path, "/") != 0) {

		// creating a copy to work with strtok (it modifies the original str)
		char * path_c = malloc(sizeof(char) * (req_path_size + 1));
		strcpy(path_c, path);

		char * dir = strtok(path_c, "/");
		while (dir != NULL) {
			dir_pos = search_dir(dir, pb_pos, LOCK_READ, pb_pos);
			if (dir_pos == -OSADA_ENOTDIR) {
				log_error(logger, "client %d, readdir '%s', directory '%s' : not a directory", * client_socket, path, dir);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOTDIR; // not a directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				return;
			}
			if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
			pb_pos = dir_pos;
			dir = strtok(NULL, "/");
		}
		free(path_c);
	}
	free(path);

	t_list * node_list = list_create();
	t_list * locked_blocks_list = list_create();
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));

	char * node;
	int node_size, i;
	int file_block_number = 0, buffer_size = 0;
	int * ptr_file_block_number;
	while (file_block_number <= (FILE_BLOCKS_MOUNT - 1)) {
		if (file_block_number != pb_pos) {
			semaphore(LOCK_READ, file_block_number);
			if ((file_table_ptr->state == REGULAR || file_table_ptr->state == DIRECTORY) && file_table_ptr->parent_directory == pb_pos) {
				for (i = 0, node_size = 0; i < OSADA_FILENAME_LENGTH; i++) {
					if ((file_table_ptr->fname)[i] == '\0') break;
					node_size++;
				}
				node = malloc(sizeof(char) * (node_size + 1));
				memcpy(node, (file_table_ptr->fname), node_size);
				node[node_size] = '\0';
				buffer_size = buffer_size + node_size;

				ptr_file_block_number = malloc(sizeof(int));
				* ptr_file_block_number = file_block_number;
				list_add(locked_blocks_list, ptr_file_block_number);
				list_add(node_list, node);
			} else {
				semaphore (UNLOCK, file_block_number);
			}
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
		list_destroy_and_destroy_elements(locked_blocks_list, &unlock_block);
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		//
		// << sending response >>
		// response code
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = OSADA_NOTEMPTYDIR; // no empty directory
		// response size
		uint32_t prot_resp_size = 4;
		uint32_t resp_size = (strlen(buffer) + 1);

		int response_size = sizeof(char) * (prot_resp_code_size + prot_resp_size + resp_size);
		void * resp = malloc(response_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		memcpy(resp + prot_resp_code_size, &resp_size, prot_resp_size);
		memcpy(resp + prot_resp_code_size + prot_resp_size, buffer, resp_size);
		write(* client_socket, resp, response_size);
		free(resp);
		free(buffer);

	} else {
		list_destroy(node_list);
		list_destroy(locked_blocks_list);
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		//
		// << sending response >>
		// response code
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = OSADA_EMPTYDIR; // empty directory

		int response_size = sizeof(char) * (prot_resp_code_size);
		void * resp = malloc(response_size);
		memcpy(resp, &resp_code, prot_resp_code_size);
		write(* client_socket, resp, response_size);
		free(resp);
	}
}

void osada_getattr(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, getattr %s", * client_socket, path);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos, LOCK_READ, pb_pos);
		if (node_pos == -OSADA_ENOENT) {
			log_error(logger, "client %d, getattr '%s', node '%s' : no such file or directory", * client_socket, path, node);
			if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
			//
			// << sending response >>
			// response code
			uint8_t prot_resp_code_size = 1;
			uint8_t resp_code = OSADA_ENOENT; // no such file or directory

			int response_size = sizeof(char) * (prot_resp_code_size);
			void * resp = malloc(response_size);
			memcpy(resp, &resp_code, prot_resp_code_size);
			write(* client_socket, resp, response_size);
			free(resp);
			free(path_c);
			free(path);
			return;
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);
	free(path);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	// file size
	uint8_t prot_resp_file_size = 4;
	uint32_t file_size = 0;
	// last modification
	uint8_t prot_resp_lastmod_size = 4;
	uint32_t lastmod = 0;
	osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
	file_table_ptr = file_table_ptr + node_pos;

	if (file_table_ptr->state == DIRECTORY) {
		resp_code = OSADA_ISDIR; // is a directory
	} else if (file_table_ptr->state == REGULAR){
		resp_code = OSADA_ISREG; // is a regular file
		file_size = file_table_ptr->file_size;
	}
	lastmod = file_table_ptr->lastmod;

	semaphore(UNLOCK, node_pos);

	int response_size = sizeof(char) * (prot_resp_code_size + prot_resp_file_size + prot_resp_lastmod_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &file_size, prot_resp_file_size);
	memcpy(resp + prot_resp_code_size + prot_resp_file_size, &lastmod, prot_resp_lastmod_size);
	write(* client_socket, resp, response_size);
	free(resp);

}

void osada_mknod(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, mknod %s...", * client_socket, path);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		node_pos = search_dir(node, pb_pos, LOCK_READ, pb_pos);
		if (node_pos == -OSADA_ENOTDIR) {
			char * rfile = node;
			node = strtok(NULL, "/");
			if (node == NULL) {
				//
				// final node, the regular file to create
				//
				// checking file name size
				if (strlen(rfile) > OSADA_FILENAME_LENGTH) {
					// name too long
					log_error(logger, "client %d, mknod '%s', creating regular file '%s' : name too long", * client_socket, path, rfile);
					if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENAMETOOLONG; // name too long

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, prot_resp_code_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
				node_pos = create_node(rfile, pb_pos);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				if (node_pos == -OSADA_ENOSPC) {
					log_error(logger, "client %d, mknod '%s', creating regular file '%s' : no space left on device", * client_socket, path, rfile);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOSPC; // no space left on device

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
				break;
			} else {
				log_error(logger, "client %d, mknod '%s', directory '%s' : not a directory", * client_socket, path, rfile);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				// the directory where the user want to create the file doesn't exist
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOTDIR; // not a directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				return;
			}
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);
	free(path);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_ISREG; // is a regular file

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);

}

void osada_write(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';

	// size (amount of bytes to write)
	uint8_t prot_size = 4;
	uint32_t size;
	received_bytes = recv(* client_socket, &size, prot_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// offset
	uint8_t prot_offset = 4;
	uint32_t offset;
	received_bytes = recv(* client_socket, &offset, prot_offset, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}

	// buffer
	char * buffer = malloc(sizeof(char) * (size));
	received_bytes = recv(* client_socket, buffer, size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}

	log_info(logger, "client %d, write %s, size %d, offset %d", * client_socket, path, size, offset);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	// search file location
	int dir_pos, file_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		dir_pos = search_dir(node, pb_pos, LOCK_READ, pb_pos);
		if (dir_pos == -OSADA_ENOTDIR) {
			char * rfile = node;
			node = strtok(NULL, "/");
			if (node == NULL) {
				//
				// final node, the regular file to write
				//
				file_pos = search_node(rfile, pb_pos, LOCK_WRITE, pb_pos);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				if (file_pos == -OSADA_ENOENT) {
					log_error(logger, "client %d, write '%s', node '%s' : no such file or directory", * client_socket, path, rfile);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOENT; // no such file or directory

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(path_c);
					free(path);
					free(buffer);
					return;
				}
				break;
			} else {
				log_error(logger, "client %d, write '%s', node '%s' : no such file or directory", * client_socket, path, node);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOENT; // no such file or directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				free(buffer);
				return;
			}
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = dir_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);


	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * file_pos));

	node_ptr->lastmod = time(NULL);
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
		int pos;
		bool its_busy;
		if (node_ptr->first_block == END_OF_FILE) {
			//
			// empty file
			//
			// assign first block
			pthread_mutex_lock(&m_lock);
			while (free_db <= BM_DATA_1) {
				pos = free_db - BM_DATA_0;
				its_busy = bitarray_test_bit(bitmap, pos);
				if (!its_busy) {
					bitarray_set_bit(bitmap, pos);
					node_ptr->first_block = pos;
					aux_map_ptr = map_ptr + (node_ptr->first_block);
					* aux_map_ptr = END_OF_FILE;
					break;
				}
				free_db++;
			}
			pthread_mutex_unlock(&m_lock);
			if (free_db > BM_DATA_1) {
				node_ptr->file_size = fsize;
				log_error(logger, "client %d, write '%s' : no space left on device", * client_socket, path);
				semaphore(UNLOCK, file_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOSPC; // no space left on device

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, prot_resp_code_size);
				free(resp);
				free(path);
				free(buffer);
				return;
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
		pthread_mutex_lock(&m_lock);
		while (bytes_to_expand > 0 && free_db <= BM_DATA_1) {
			//
			// adding block
			//
			pos = free_db - BM_DATA_0;
			its_busy = bitarray_test_bit(bitmap, pos);
			if (!its_busy) {
				bitarray_set_bit(bitmap, pos);
				* aux_map_ptr = pos;
				aux_map_ptr = map_ptr + (* aux_map_ptr);
				* aux_map_ptr = END_OF_FILE;
				bytes_to_expand = bytes_to_expand - OSADA_BLOCK_SIZE;
			}
			free_db++;
		}
		pthread_mutex_unlock(&m_lock);
		if (free_db > BM_DATA_1) {
			log_error(logger, "client %d, write '%s' : no space left on device", * client_socket, path);
			// no space left on device
			// releasing blocks
			node_ptr->file_size = fsize;
			int necessary_blocks = fsize / OSADA_BLOCK_SIZE;
			// considering file size = 0 (empty file)
			if ((fsize > 0) && ((necessary_blocks == 0) || ((fsize % OSADA_BLOCK_SIZE) >= 1))) necessary_blocks++;

			osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);
			while (necessary_blocks > 0) {
				necessary_blocks--;
				aux_map_ptr = map_ptr + (* aux_map_ptr);
			}

			osada_block_pointer aux;
			pthread_mutex_lock(&m_lock);
			while ((* aux_map_ptr) != END_OF_FILE) {
				aux = (* aux_map_ptr);
				bitarray_clean_bit(bitmap, (* aux_map_ptr));
				* aux_map_ptr = END_OF_FILE;
				aux_map_ptr = map_ptr + aux;
			}
			pthread_mutex_unlock(&m_lock);
			semaphore(UNLOCK, file_pos);
			//
			// << sending response >>
			// response code
			uint8_t prot_resp_code_size = 1;
			uint8_t resp_code = OSADA_ENOSPC; // no space left on device

			int response_size = sizeof(char) * (prot_resp_code_size);
			void * resp = malloc(response_size);
			memcpy(resp, &resp_code, prot_resp_code_size);
			write(* client_socket, resp, prot_resp_code_size);
			free(resp);
			free(path);
			free(buffer);
			return;
		}
	}

	// positioning the map pointer to the first block (considering the offset)
	aux_map_ptr = &(node_ptr->first_block);
	movs = offset / OSADA_BLOCK_SIZE;
	int i = movs;
	while (i > 0) {
		aux_map_ptr = map_ptr + (* aux_map_ptr);
		i--;
	}

	// writing bytes
	offset = offset - (OSADA_BLOCK_SIZE * movs);
	osada_block * data_ptr = (osada_block *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * DATA_0));
	char * aux_data_ptr = (char *)(data_ptr + (* aux_map_ptr));
	bytes_availables_in_block = OSADA_BLOCK_SIZE - offset;
	int bytes_to_write = size;
	int buff_pos = 0;
	int bytes_writing;

	while (bytes_to_write > 0) {
		bytes_writing = (bytes_to_write >= bytes_availables_in_block) ? bytes_availables_in_block : bytes_to_write;
		memcpy(&aux_data_ptr[offset], buffer + buff_pos, bytes_writing);
		bytes_to_write = bytes_to_write - bytes_writing;
		aux_map_ptr = map_ptr + (* aux_map_ptr);
		if ((* aux_map_ptr) == END_OF_FILE) break;
		aux_data_ptr  = (char *)(data_ptr + (* aux_map_ptr));
		buff_pos = buff_pos + bytes_writing;
		bytes_availables_in_block = OSADA_BLOCK_SIZE;
		offset = 0;
	}
	semaphore(UNLOCK, file_pos);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_SEXE; // successful execution

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);
	free(path);
	free(buffer);

}

void osada_read(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	// size (amount of bytes to read)
	uint8_t prot_size = 4;
	uint32_t size;
	received_bytes = recv(* client_socket, &size, prot_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// offset
	uint8_t prot_offset = 4;
	uint32_t offset;
	received_bytes = recv(* client_socket, &offset, prot_offset, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	log_info(logger, "client %d, read %s, size %d, offset %d", * client_socket, path, size, offset);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	// search file location
	int node_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		node_pos = search_node(node, pb_pos, LOCK_READ, pb_pos);
		if (node_pos == -OSADA_ENOENT) {
			log_error(logger, "client %d, read '%s', node '%s' : no such file or directory", * client_socket, path, node);
			if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
			//
			// << sending response >>
			// response code
			uint8_t prot_resp_code_size = 1;
			uint8_t resp_code = OSADA_ENOENT; // no such file or directory

			int response_size = sizeof(char) * (prot_resp_code_size);
			void * resp = malloc(response_size);
			memcpy(resp, &resp_code, prot_resp_code_size);
			write(* client_socket, resp, response_size);
			free(resp);
			free(path_c);
			free(path);
			return;
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = node_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);
	free(path);

	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * node_pos));

	int file_size = (node_ptr->file_size);
	void * buff;
	int bytes_transferred = 0;

	if (offset < (file_size - 1)) {
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

		bytes_transferred = size;
	}
	semaphore (UNLOCK, node_pos);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_SEXE; // successful execution
	// bytes transferred
	uint8_t prot_bytes_transferred_size = 4;

	int response_size = sizeof(char) * (prot_resp_code_size + prot_bytes_transferred_size + bytes_transferred);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	memcpy(resp + prot_resp_code_size, &bytes_transferred, prot_bytes_transferred_size);
	// content
	if (bytes_transferred > 0) {
		memcpy(resp + prot_resp_code_size + prot_bytes_transferred_size, buff, bytes_transferred);
		free(buff);
	}
	write(* client_socket, resp, response_size);
	free(resp);
}

void osada_truncate(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	// new size
	uint8_t prot_size = 4;
	uint32_t new_size;
	received_bytes = recv(* client_socket, &new_size, prot_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	log_info(logger, "client %d, truncate %s, size %d", * client_socket, path, new_size);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	// search file location
	int dir_pos, file_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		dir_pos = search_dir(node, pb_pos, LOCK_READ, pb_pos);
		if (dir_pos == -OSADA_ENOTDIR) {
			char * rfile = node;
			node = strtok(NULL, "/");
			if (node == NULL) {
				//
				// final node, the regular file to write
				//
				file_pos = search_node(rfile, pb_pos, LOCK_WRITE, pb_pos);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				if (file_pos == -OSADA_ENOENT) {
					log_error(logger, "client %d, truncate '%s', node '%s' : no such file or directory", * client_socket, path, rfile);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOENT; // no such file or directory

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
				break;
			} else {
				log_error(logger, "client %d, truncate '%s', node '%s' : no such file or directory", * client_socket, path, node);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOENT; // no such file or directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				return;
			}
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = dir_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);

	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * file_pos));
	int old_size = node_ptr->file_size;
	node_ptr->file_size = new_size;

	if (new_size != old_size) {
		// mapping file
		osada_block_pointer * map_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
		osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);
		// gettingg current blocks
		int current_blocks = 0;
		while ((* aux_map_ptr) != END_OF_FILE) {
			current_blocks++;
			aux_map_ptr = map_ptr + (* aux_map_ptr);
		}
		// getting necessary blocks
		int necessary_blocks = new_size / OSADA_BLOCK_SIZE;
		// considering new size = 0 (empty file)
		if ((new_size > 0) && ((necessary_blocks == 0) || ((new_size % OSADA_BLOCK_SIZE) >= 1))) necessary_blocks++;
		if (new_size > old_size && necessary_blocks > current_blocks) {
			//
			// adding blocks
			//
			necessary_blocks = necessary_blocks - current_blocks;
			int free_db = BM_DATA_0;
			int pos;
			bool its_busy;
			pthread_mutex_lock(&m_lock);
			while (necessary_blocks > 0 && (free_db <= BM_DATA_1)) {
				pos = free_db - BM_DATA_0;
				its_busy = bitarray_test_bit(bitmap, pos);
				if (!its_busy) {
					bitarray_set_bit(bitmap, pos);
					* aux_map_ptr = pos;
					aux_map_ptr = map_ptr + (* aux_map_ptr);
					* aux_map_ptr = END_OF_FILE;
					necessary_blocks--;
				}
				free_db++;
			}
			pthread_mutex_unlock(&m_lock);
			if (free_db > BM_DATA_1) {
				log_error(logger, "client %d, truncate '%s' : no space left on device", * client_socket, path);
				//
				// no space left on device
				// releasing blocks
				//
				node_ptr->file_size = old_size;
				necessary_blocks = old_size / OSADA_BLOCK_SIZE;
				// considering old file size = 0 (empty file)
				if ((old_size > 0) && ((necessary_blocks == 0) || ((old_size % OSADA_BLOCK_SIZE) >= 1))) necessary_blocks++;

				osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);
				while (necessary_blocks > 0) {
					necessary_blocks--;
					aux_map_ptr = map_ptr + (* aux_map_ptr);
				}

				osada_block_pointer aux;
				pthread_mutex_lock(&m_lock);
				while ((* aux_map_ptr) != END_OF_FILE) {
					aux = (* aux_map_ptr);
					bitarray_clean_bit(bitmap, (* aux_map_ptr));
					* aux_map_ptr = END_OF_FILE;
					aux_map_ptr = map_ptr + aux;
				}
				pthread_mutex_unlock(&m_lock);
				semaphore (UNLOCK, file_pos);

				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOSPC; // no space left on device

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, prot_resp_code_size);
				free(resp);
				free(path);
				return;
			}
		} else if (new_size < old_size && necessary_blocks < current_blocks) {
			//
			// releasing blocks
			//
			osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);
			while (necessary_blocks > 0) {
				necessary_blocks--;
				aux_map_ptr = map_ptr + (* aux_map_ptr);
			}

			osada_block_pointer aux;
			pthread_mutex_lock(&m_lock);
			while ((* aux_map_ptr) != END_OF_FILE) {
				aux = (* aux_map_ptr);
				bitarray_clean_bit(bitmap, (* aux_map_ptr));
				* aux_map_ptr = END_OF_FILE;
				aux_map_ptr = map_ptr + aux;
			}
			pthread_mutex_unlock(&m_lock);
		}
	}
	semaphore (UNLOCK, file_pos);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_SEXE;	// successful execution

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);
	free(path);
}

void osada_unlink(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, unlink %s", * client_socket, path);

	// creating a copy to work with strtok (it modifies the original str)
	char * path_c = malloc(sizeof(char) * (req_path_size + 1));
	strcpy(path_c, path);

	// search file location
	int dir_pos, file_pos;
	int pb_pos = ROOT;
	char * node = strtok(path_c, "/");
	while (node != NULL) {
		dir_pos = search_dir(node, pb_pos, LOCK_READ, pb_pos);
		if (dir_pos == -OSADA_ENOTDIR) {
			char * rfile = node;
			node = strtok(NULL, "/");
			if (node == NULL) {
				//
				// final node, the regular file to write
				//
				file_pos = search_node(rfile, pb_pos, LOCK_WRITE, pb_pos);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				if (file_pos == -OSADA_ENOENT) {
					log_error(logger, "client %d, unlink '%s', node '%s' : no such file or directory", * client_socket, path, node);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOENT; // no such file or directory

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(path_c);
					free(path);
					return;
				}
				break;
			} else {
				log_error(logger, "client %d, unlink '%s', node '%s' : no such file or directory", * client_socket, path, node);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOENT; // no such file or directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				return;
			}
		}
		if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
		pb_pos = dir_pos;
		node = strtok(NULL, "/");
	}
	free(path_c);
	free(path);


	// set pointer to file node
	osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * file_pos));

	// mapping file
	osada_block_pointer * map_ptr = (osada_block_pointer *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * MAPPING_TABLE_0));
	osada_block_pointer * aux_map_ptr = &(node_ptr->first_block);

	// releasing blocks
	uint32_t aux;
	pthread_mutex_lock(&m_lock);
	while ((* aux_map_ptr) != END_OF_FILE) {
		aux = (* aux_map_ptr);
		bitarray_clean_bit(bitmap, (* aux_map_ptr));
		* aux_map_ptr = END_OF_FILE;
		aux_map_ptr = map_ptr + aux;
	}
	pthread_mutex_unlock(&m_lock);

	node_ptr->state = DELETED;
	semaphore (UNLOCK, file_pos);

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_SEXE;	// successful execution

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);

}

void osada_rmdir(int * client_socket) {
	//
	// << receiving message >>
	// path size
	uint8_t prot_path_size = 4;
	uint32_t req_path_size;
	int received_bytes = recv(* client_socket, &req_path_size, prot_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// path
	char * path = malloc(sizeof(char) * (req_path_size + 1));
	received_bytes = recv(* client_socket, path, req_path_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	path[req_path_size] = '\0';
	log_info(logger, "client %d, rmdir %s", * client_socket, path);

	if (strcmp(path, "/") != 0) {

		// creating a copy to work with strtok (it modifies the original str)
		char * path_c = malloc(sizeof(char) * (req_path_size + 1));
		strcpy(path_c, path);

		int dir_pos;
		int pb_pos = ROOT;
		char * dir = strtok(path_c, "/");
		char * next_dir;
		int lock_type = LOCK_READ;
		while (dir != NULL) {
			next_dir = strtok(NULL, "/");
			if (next_dir == NULL) {
				//
				// final directory, the directory to delete
				//
				lock_type = LOCK_WRITE;
			}
			dir_pos = search_dir(dir, pb_pos, lock_type, pb_pos);
			if (dir_pos == -OSADA_ENOTDIR) {
				log_error(logger, "client %d, rmdir '%s', dir '%s' : no such file or directory", * client_socket, path, dir);
				if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOTDIR;	// not a directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(path_c);
				free(path);
				return;
			}
			if (pb_pos != ROOT) semaphore(UNLOCK, pb_pos);
			pb_pos = dir_pos;
			dir = next_dir;
		}
		free(path_c);
		free(path);

		// set pointer to file node
		osada_file * node_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0) + (OSADA_FILE_BLOCK_SIZE * dir_pos));
		node_ptr->state = DELETED;

		semaphore(UNLOCK, dir_pos);
	}

	//
	// << sending response >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = OSADA_SEXE;	// successful execution

	int response_size = sizeof(char) * (prot_resp_code_size);
	void * resp = malloc(response_size);
	memcpy(resp, &resp_code, prot_resp_code_size);
	write(* client_socket, resp, response_size);
	free(resp);
}

void osada_rename(int * client_socket) {
	//
	// << receiving message >>
	// from size
	uint8_t prot_from_size = 4;
	uint32_t req_from_size;

	int received_bytes = recv(* client_socket, &req_from_size, prot_from_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// from
	char * from = malloc(sizeof(char) * (req_from_size + 1));
	received_bytes = recv(* client_socket, from, req_from_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	from[req_from_size] = '\0';

	// to size
	uint8_t prot_to_size = 4;
	uint32_t req_to_size;
	received_bytes = recv(* client_socket, &req_to_size, prot_to_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	// to
	char * to = malloc(sizeof(char) * (req_to_size + 1));
	received_bytes = recv(* client_socket, to, req_to_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		log_error(logger, "client %d disconnected...", * client_socket);
		return;
	}
	to[req_to_size] = '\0';

	log_info(logger, "client %d, rename %s to %s", * client_socket, from, to);

	// creating a copy to work with strtok (it modifies the original str)
	char * from_c = malloc(sizeof(char) * (req_from_size + 1));
	strcpy(from_c, from);

	// search file location "from"
	int node_pos_from, file_pos_from;
	int pb_pos_from = ROOT;
	char * node_from = strtok(from_c, "/");
	while (node_from != NULL) {
		node_pos_from = search_dir(node_from, pb_pos_from, LOCK_READ, pb_pos_from);
		if (node_pos_from == -OSADA_ENOTDIR) {
			char * rfile = node_from;
			node_from = strtok(NULL, "/");
			if (node_from == NULL) {
				//
				// final node, the regular file or directory to move
				//
				node_pos_from = search_node(rfile, pb_pos_from, LOCK_WRITE, pb_pos_from);
				if (pb_pos_from != ROOT) semaphore(UNLOCK, pb_pos_from);
				if (node_pos_from == -OSADA_ENOENT) {
					log_error(logger, "client %d, rename '%s', node '%s' : no such file or directory", * client_socket, from, node_from);
					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_ENOENT; // no such file or directory

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(from_c);
					free(from);
					return;
				}
				break;
			} else {
				log_error(logger, "client %d, rename '%s', node '%s' : no such file or directory", * client_socket, from, node_from);
				if (pb_pos_from != ROOT) semaphore(UNLOCK, pb_pos_from);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOENT; // no such file or directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(from_c);
				free(from);
				return;
			}
		}
		if (pb_pos_from != ROOT) semaphore(UNLOCK, pb_pos_from);
		pb_pos_from = node_pos_from;
		node_from = strtok(NULL, "/");
	}
	free(from_c);



	// creating a copy to work with strtok (it modifies the original str)
	char * to_c = malloc(sizeof(char) * (req_to_size + 1));
	strcpy(to_c, to);

	// search file location "to"
	int node_pos_to;
	int pb_pos_to = ROOT;
	char * node_to = strtok(to_c, "/");
	while (node_to != NULL) {
		node_pos_to = search_dir(node_to, pb_pos_to, LOCK_READ, node_pos_from);
		if (node_pos_to == -OSADA_ENOTDIR) {
			char * rfile = node_to;
			node_to = strtok(NULL, "/");
			if (node_to == NULL) {
				//
				// final node, the regular file "to"
				//
				node_pos_to = search_node(rfile, pb_pos_to, LOCK_READ, node_pos_from);
				if (pb_pos_to != ROOT) semaphore(UNLOCK, pb_pos_to);
				if (node_pos_to == -OSADA_ENOENT) {
					//Primero reviso extension del nuevo nombre
					if (strlen(rfile) > OSADA_FILENAME_LENGTH) {
						log_error(logger, "client %d, rename '%s' to '%s' : name too long", * client_socket, from, to);
						semaphore(UNLOCK, node_pos_from);
						//
						// << sending response >>
						// response code
						uint8_t prot_resp_code_size = 1;
						uint8_t resp_code = OSADA_ENAMETOOLONG; // name too long

						int response_size = sizeof(char) * (prot_resp_code_size);
						void * resp = malloc(response_size);
						memcpy(resp, &resp_code, prot_resp_code_size);
						write(* client_socket, resp, prot_resp_code_size);
						free(resp);
						free(to_c);
						free(to);
						free(from);
						return;
					}
					//log_error(logger, "client %d, rename '%s', node '%s' : ", * client_socket, to, node);
					//Archivo destino no existe, se le cambia nombre y parent directory a archivo origen
					//TODO validar tamanio del nuevo nombre
					//TODO corregir semaforos
					osada_file * file_table_ptr = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
					file_table_ptr = file_table_ptr + node_pos_from;
					int node_name_size = strlen(rfile);
					file_table_ptr->parent_directory = pb_pos_to;
					memcpy((char *)(file_table_ptr->fname), rfile, node_name_size);
					if (node_name_size < OSADA_FILENAME_LENGTH) file_table_ptr->fname[node_name_size] = '\0';

/*					osada_file * o_file = malloc(sizeof(osada_file));
					int node_name_size = strlen(rfile);
					memcpy((char *)(o_file->fname), rfile, node_name_size);
					if (node_name_size < OSADA_FILENAME_LENGTH) o_file->fname[node_name_size] = '\0';
					o_file->state = file_table_ptr->state;
					o_file->parent_directory = pb_pos_to;
					o_file->file_size = file_table_ptr->file_size;
					o_file->lastmod = time(NULL);
					o_file->first_block = file_table_ptr->first_block;
					memcpy(file_table_ptr, o_file, sizeof(osada_file));
					free(o_file);
*/
					semaphore(UNLOCK, node_pos_from);

					//
					// << sending response >>
					// response code
					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code = OSADA_SEXE;	// successful execution


					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(to_c);
					free(to);
					free(from);
					return;
				} else {
					//Archivo destino existe, si es un directorio se le cambia solo parent directory a archivo origen
					//si destino es un archivo devuelve error
					osada_file * file_table_ptr_to = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
					file_table_ptr_to = file_table_ptr_to + node_pos_to;

					osada_file * file_table_ptr_from = (osada_file *) (osada_fs_ptr + (OSADA_BLOCK_SIZE * FILE_TABLE_0));
					file_table_ptr_from = file_table_ptr_from + node_pos_from;

					uint8_t prot_resp_code_size = 1;
					uint8_t resp_code;

					if (file_table_ptr_to->state == DIRECTORY) {
						file_table_ptr_from->parent_directory = pb_pos_to;
						file_table_ptr_from->lastmod = time(NULL);
						resp_code = OSADA_SEXE;	// successful execution
					} else if (file_table_ptr_to->state == REGULAR){
						resp_code = OSADA_ISREG; // is a regular file
					}

					semaphore(UNLOCK, node_pos_from);
					semaphore(UNLOCK, node_pos_to);

					int response_size = sizeof(char) * (prot_resp_code_size);
					void * resp = malloc(response_size);
					memcpy(resp, &resp_code, prot_resp_code_size);
					write(* client_socket, resp, response_size);
					free(resp);
					free(to_c);
					free(to);
					return;


				}
				break;
			} else {
				log_error(logger, "client %d, rename '%s', node '%s' : no such file or directory", * client_socket, to, node_to);
				if (pb_pos_to != ROOT) semaphore(UNLOCK, pb_pos_to);
				//
				// << sending response >>
				// response code
				uint8_t prot_resp_code_size = 1;
				uint8_t resp_code = OSADA_ENOENT; // no such file or directory

				int response_size = sizeof(char) * (prot_resp_code_size);
				void * resp = malloc(response_size);
				memcpy(resp, &resp_code, prot_resp_code_size);
				write(* client_socket, resp, response_size);
				free(resp);
				free(to_c);
				free(to);
				return;
			}
		}
		if (pb_pos_to != ROOT) semaphore(UNLOCK, pb_pos_to);
		pb_pos_to = node_pos_to;
		node_to = strtok(NULL, "/");
	}
	free(to_c);
	free(to);
	free(from);

}
