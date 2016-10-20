/*
 * pokedex-client.c
 *
 *  Created on: 16/9/2016
 *      Author: Dante Romero
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>

/**
 * TODO Remove (only for test)
 */
#define DEFAULT_FILE_CONTENT "Hello World!\n"
#define DEFAULT_FILE_NAME "hello"
#define DEFAULT_FILE_PATH "/" DEFAULT_FILE_NAME

#define RES_MKDIR_OK 1
#define RES_READDIR_ISDIR 1
#define RES_READDIR_ISEMPTYDIR 2
#define RES_GETATTR_ISDIR 1
#define RES_GETATTR_ENOTDIR 2

const uint8_t REQ_MKDIR = 1;
const uint8_t REQ_READ_DIR = 2;
const uint8_t REQ_GET_ATTR = 3;

t_config * conf; 					// properties file
struct addrinfo hints;				// pokedex-server: socket connection
struct addrinfo * server_info;		//

struct t_runtime_options {
	char * welcome_msg;
} runtime_options;

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

void open_connection(int *);
void close_connection(int *);
void load_properties_file(void);

int pk_mkdir(const char * path, mode_t mode) {
	int server_socket;
	open_connection(&server_socket);
	// << sending message >>
	uint8_t prot_ope_code_size = 1;
	uint8_t prot_path_size = 4;
	uint8_t req_ope_code = REQ_MKDIR;
	uint32_t req_path_size = strlen(path);
	void * buffer = malloc(prot_ope_code_size + prot_path_size + req_path_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, prot_ope_code_size + prot_path_size + req_path_size, 0);
	free(buffer);
	// << receiving message >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	if (recv(server_socket, &resp_code, prot_resp_code_size, 0) <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
	}
	if (resp_code == RES_MKDIR_OK) {
		// TODO
	}
	close_connection(&server_socket);
	return 0;
}

static int pk_getattr(const char * path, struct stat * stbuf) {
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strlen(path) > 0) {
		int server_socket;
		open_connection(&server_socket);
		// << sending message >>
		uint8_t req_ope_code = REQ_GET_ATTR;
		uint32_t req_path_size = strlen(path);
		uint8_t prot_ope_code_size = 1;
		uint8_t prot_path_size = 4;
		void * buffer = malloc(prot_ope_code_size + prot_path_size + req_path_size);
		memcpy(buffer, &req_ope_code, prot_ope_code_size);
		memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
		memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
		send(server_socket, buffer, prot_ope_code_size + prot_path_size + req_path_size, 0);
		free(buffer);
		// << receiving message >>
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = 0;
		if (recv(server_socket, &resp_code, prot_resp_code_size, 0) <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
		}
		if (resp_code == RES_GETATTR_ISDIR) {
			stbuf->st_mode = S_IFDIR | 7777;
			stbuf->st_nlink = 2;
		} else {
			res = -ENOENT;
		}
		close_connection(&server_socket);
	} else {
		res = -ENOENT;
	}
	return res;
}


static int pk_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
	(void) offset;
	(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	int server_socket;
	open_connection(&server_socket);
	// << sending message >>
	uint8_t req_ope_code = REQ_READ_DIR;
	uint32_t req_path_size = strlen(path);
	uint8_t prot_ope_code_size = 1;
	uint8_t prot_path_size = 4;
	void * buffer = malloc(prot_ope_code_size + prot_path_size + req_path_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, prot_ope_code_size + prot_path_size + req_path_size, 0);
	free(buffer);

	// << receiving message >>
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	if (recv(server_socket, &resp_code, prot_resp_code_size, 0) <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return 1;
	}
	if (resp_code == RES_READDIR_ISDIR) {
		uint32_t prot_resp_size = 4;
		uint32_t resp_size;
		if (recv(server_socket, &resp_size, prot_resp_size, 0) <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return 1;
		}
		char * resp = malloc(resp_size);
		recv(server_socket, resp, resp_size, 0);
		if (recv(server_socket, resp, resp_size, 0)) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return 1;
		}

		char * dir = strtok(resp, ",");
		int dir_len;
		while (dir != NULL) {
			dir_len = strlen(dir);
			if (dir_len > 0) {
				filler(buf, dir, NULL, 0);
			}
			dir = strtok (NULL, ",");
		}

		free(resp);
	}
	close_connection(&server_socket);
	return 0;
}

static int pk_open(const char * path, struct fuse_file_info * fi) {
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	return 0;
}

static int pk_read(const char * path, char * buf, size_t size, off_t offset, struct fuse_file_info * fi) {
	size_t len;
	(void) fi;
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;
	len = strlen(DEFAULT_FILE_CONTENT);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, DEFAULT_FILE_CONTENT + offset, size);
	} else
		size = 0;
	return size;
}

static struct fuse_operations pk_oper = {
	.getattr = pk_getattr,
	.mkdir = pk_mkdir,
	.readdir = pk_readdir,
	.open = pk_open,
	.read = pk_read
};

enum {
	KEY_VERSION,
	KEY_HELP
};

static struct fuse_opt fuse_options[] = {
	CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

	FUSE_OPT_KEY("-V", KEY_VERSION),
	FUSE_OPT_KEY("--version", KEY_VERSION),
	FUSE_OPT_KEY("-h", KEY_HELP),
	FUSE_OPT_KEY("--help", KEY_HELP),
	FUSE_OPT_END
};

int main(int argc, char* argv[]) {
	load_properties_file();

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1) {
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	return fuse_main(args.argc, args.argv, &pk_oper, NULL);
}

void open_connection(int * server_socket) {
	getaddrinfo(config_get_string_value(conf, "pokedex.server.ip"), config_get_string_value(conf, "pokedex.server.port"), &hints, &server_info);
	* server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	connect(* server_socket, server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);
}

void close_connection(int * server_socket) {
	close(* server_socket);
}

void load_properties_file(void) {
	conf = config_create("./conf/pokedex-client.properties");
}