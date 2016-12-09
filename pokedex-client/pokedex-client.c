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

#define	OSADA_ENOENT		 		1 // no such file or directory
#define	OSADA_ISREG		 			2 // is a regular file
#define	OSADA_ISDIR		 			3 // is a directory
#define	OSADA_ENOTDIR				4 // not a directory
#define	OSADA_ENOSPC				5 // no space left on device
#define	OSADA_EMPTYDIR 				6 // empty directory
#define	OSADA_NOTEMPTYDIR 			7 // no empty directory
#define	OSADA_SEXE		 			8 // successful execution
#define	OSADA_ENAMETOOLONG		 	9 // name too long

const uint8_t REQ_MKDIR = 1;
const uint8_t REQ_READ_DIR = 2;
const uint8_t REQ_GET_ATTR = 3;
const uint8_t REQ_MKNOD = 4;
const uint8_t REQ_WRITE = 5;
const uint8_t REQ_READ = 6;
const uint8_t REQ_TRUNCATE = 7;
const uint8_t REQ_UNLINK = 8;
const uint8_t REQ_RMDIR = 9;
const uint8_t REQ_RENAME = 10;

t_config * conf;
char* pokedex_server_ip;
char* pokedex_server_port;
struct addrinfo hints;
struct addrinfo * server_info;

int server_socket;

struct t_runtime_options {
	char * welcome_msg;
} runtime_options;

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

void open_connection();
void close_connection();
void load_properties_file(void);

static int pk_mkdir(const char * path, mode_t mode) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_MKDIR;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOSPC)
		return -ENOSPC;
	if (resp_code == OSADA_ENAMETOOLONG)
			return -EINVAL;

	return 0;
}

static int pk_getattr(const char * path, struct stat * stbuf) {
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strlen(path) > 0) {

		// << sending message >>
		// operation code
		uint8_t prot_ope_code_size = 1;
		uint8_t req_ope_code = REQ_GET_ATTR;
		// path
		uint8_t prot_path_size = 4;
		uint32_t req_path_size = strlen(path);

		int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
		void * buffer = malloc(buffer_size);
		memcpy(buffer, &req_ope_code, prot_ope_code_size);
		memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
		memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
		send(server_socket, buffer, buffer_size, 0);
		free(buffer);

		// << receiving message >>
		// response code
		uint8_t prot_resp_code_size = 1;
		uint8_t resp_code = 0;
		int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
		}

		// file size
		uint32_t prot_resp_file_size = 4;
		uint32_t file_size;
		// last modification
		uint8_t prot_resp_lastmod_size = 4;
		uint32_t lastmod = 0;
		if (resp_code == OSADA_ENOENT) {
			res = -ENOENT;
		} else {
			received_bytes = recv(server_socket, &file_size, prot_resp_file_size, MSG_WAITALL);
			if (received_bytes <= 0) {
				printf("pokedex client: server %d disconnected...\n", server_socket);
				return -EIO;
			}
			received_bytes = recv(server_socket, &lastmod, prot_resp_lastmod_size, MSG_WAITALL);
			if (received_bytes <= 0) {
				printf("pokedex client: server %d disconnected...\n", server_socket);
				return -EIO;
			}
			if (resp_code == OSADA_ISDIR) {
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
			} else if (resp_code == OSADA_ISREG) {
				stbuf->st_mode = S_IFREG | 0755;
				stbuf->st_nlink = 2;
				stbuf->st_size = file_size;
			}
		}

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

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_READ_DIR;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOTDIR) {
		return -ENOTDIR;
	} else  if (resp_code == OSADA_NOTEMPTYDIR) {
		// directories and files
		uint8_t prot_resp_size = 4;
		uint32_t resp_size;
		received_bytes = recv(server_socket, &resp_size, prot_resp_size, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
		}
		char * resp = malloc(resp_size);
		received_bytes = recv(server_socket, resp, resp_size, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
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

	return 0;
}

static int pk_mknod(const char * path, mode_t mode, dev_t dev) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_MKNOD;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOSPC)
		return -ENOSPC;
	if (resp_code == OSADA_ENAMETOOLONG)
		return -EINVAL;
	if (resp_code == OSADA_ENOTDIR)
		return -ENOTDIR;

	return 0;
}

static int pk_truncate(const char * path, off_t offset) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_TRUNCATE;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);
	// offset
	uint8_t prot_offset = 4;
	uint32_t req_offset = offset;

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size + prot_offset);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size, &req_offset, prot_offset);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOENT)
		return -ENOENT;
	if (resp_code == OSADA_ENOSPC)
		return -ENOSPC;

	return 0;
}

static int pk_read(const char * path, char * buf, size_t size, off_t offset, struct fuse_file_info * fi) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_READ;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);
	// size
	uint8_t prot_size = 4;
	uint32_t req_size = size;
	// offset
	uint8_t prot_offset = 4;
	uint32_t req_offset = offset;

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size + prot_size + prot_offset);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size, &req_size, prot_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size + prot_size, &req_offset, prot_offset);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOENT)
		return -ENOENT;

	// bytes transferred
	uint8_t prot_resp_bytes_transferred_size = 4;
	uint32_t bytes_transferred;
	received_bytes = recv(server_socket, &bytes_transferred, prot_resp_bytes_transferred_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}
	if (bytes_transferred > 0) {
		// content
		char * file_content = malloc(bytes_transferred);
		received_bytes = recv(server_socket, file_content, bytes_transferred, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
		}
		memcpy(buf, file_content, bytes_transferred);
		free(file_content);
	}

	return bytes_transferred;
}

static int pk_write(const char * path, const char * buf, size_t size, off_t offset, struct fuse_file_info * fi) {
	int retstat = 0;

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_WRITE;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);
	// buf
	uint8_t prot_buf_size = 4;
	uint32_t req_buf_size = strlen(buf);
	// size
	uint8_t prot_size = 4;
	uint32_t req_size = size;
	// offset
	uint8_t prot_offset = 4;
	uint32_t req_offset = offset;

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size + req_size + prot_size + prot_offset);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size, &req_size, prot_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size + prot_size, &req_offset, prot_offset);
	memcpy(buffer + prot_ope_code_size + prot_path_size + req_path_size + prot_size + prot_offset, buf, req_size);

	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_SEXE) {
		retstat = size;
	}

	return retstat;
}

static int pk_open(const char * path, struct fuse_file_info * fi) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_GET_ATTR;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	// file size
	uint32_t prot_resp_file_size = 4;
	uint32_t file_size;
	// last modification
	uint8_t prot_resp_lastmod_size = 4;
	uint32_t lastmod = 0;
	if (resp_code == OSADA_ENOENT) {
		return -ENOENT;
	} else {
		// releasing socket queue
		received_bytes = recv(server_socket, &file_size, prot_resp_file_size, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
		}
		received_bytes = recv(server_socket, &lastmod, prot_resp_lastmod_size, MSG_WAITALL);
		if (received_bytes <= 0) {
			printf("pokedex client: server %d disconnected...\n", server_socket);
			return -EIO;
		}
	}

	if (resp_code != OSADA_ISREG)
		return -ENOENT;
//	if ((fi->flags & 3) != O_RDONLY)
//		return -EACCES;
	return 0;
}

static int pk_unlink(const char* path) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_UNLINK;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOENT)
		return -ENOENT;

	return 0;
}

static int pk_rmdir(const char* path) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_RMDIR;
	// path
	uint8_t prot_path_size = 4;
	uint32_t req_path_size = strlen(path);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_path_size + req_path_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_path_size, prot_path_size);
	memcpy(buffer + prot_ope_code_size + prot_path_size, path, req_path_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOTDIR)
		return -ENOTDIR;

	return 0;
}

static int pk_rename(const char * from, const char* to) {

	// << sending message >>
	// operation code
	uint8_t prot_ope_code_size = 1;
	uint8_t req_ope_code = REQ_RENAME;
	// from
	uint8_t prot_from_size = 4;
	uint32_t req_from_size = strlen(from);
	// to
	uint8_t prot_to_size = 4;
	uint32_t req_to_size = strlen(to);

	int buffer_size = sizeof(char) * (prot_ope_code_size + prot_from_size + req_from_size + prot_to_size + req_to_size);
	void * buffer = malloc(buffer_size);
	memcpy(buffer, &req_ope_code, prot_ope_code_size);
	memcpy(buffer + prot_ope_code_size, &req_from_size, prot_from_size);
	memcpy(buffer + prot_ope_code_size + prot_from_size, from, req_from_size);
	memcpy(buffer + prot_ope_code_size + prot_from_size + req_from_size, &req_to_size, prot_to_size);
	memcpy(buffer + prot_ope_code_size + prot_from_size + req_from_size + prot_to_size, to, req_to_size);
	send(server_socket, buffer, buffer_size, 0);
	free(buffer);

	// << receiving message >>
	// response code
	uint8_t prot_resp_code_size = 1;
	uint8_t resp_code = 0;
	int received_bytes = recv(server_socket, &resp_code, prot_resp_code_size, MSG_WAITALL);
	if (received_bytes <= 0) {
		printf("pokedex client: server %d disconnected...\n", server_socket);
		return -EIO;
	}

	if (resp_code == OSADA_ENOENT)
		return -ENOENT;
	if (resp_code == OSADA_ENOSPC)
		return -ENOSPC;
	if (resp_code == OSADA_ENAMETOOLONG)
		return -EINVAL;

	return 0;
}



static struct fuse_operations pk_oper = {
	.getattr = pk_getattr,
	.mkdir = pk_mkdir,
	.readdir = pk_readdir,
	.mknod = pk_mknod,
	.open = pk_open,
	.write = pk_write,
	.read = pk_read,
	.truncate = pk_truncate,
	.unlink = pk_unlink,
	.rmdir = pk_rmdir,
	.rename = pk_rename
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
	//load_properties_file();
	open_connection();

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1) {
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}

	return fuse_main(args.argc, args.argv, &pk_oper, NULL);
}

void open_connection() {

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char * server_ip;
	char * server_port;

	server_ip =   getenv("POKEDEX_SERVER_IP");
	server_port = getenv("POKEDEX_SERVER_PORT");

	pokedex_server_ip =	string_duplicate(server_ip);
	pokedex_server_port = string_duplicate(server_port);

	getaddrinfo(pokedex_server_ip, pokedex_server_port, &hints, &server_info);
	server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);
	freeaddrinfo(server_info);
}

void close_connection() {
	close(server_socket);
}

void load_properties_file(void) {
	conf = config_create("./conf/pokedex-client.properties");
}
