/*
 * osada-fs-fuse.c
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

/**
 * TODO Remove (only for test)
 */
#define DEFAULT_FILE_CONTENT "Hello World!\n"
#define DEFAULT_FILE_NAME "hello"
#define DEFAULT_FILE_PATH "/" DEFAULT_FILE_NAME

/**
 * Almacenar parametros pasados por linea de comando a la funcion principal de FUSE
 */
struct t_runtime_options {
	char * welcome_msg;
} runtime_options;

/**
 * Macro para definir nuestros propios parametros que queremos que FUSE interprete.
 */
#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

/**
 * @DESC
 * 	Metadata de un archivo/directorio (tamaño, tipo, permisos, dueño, etc...)
 *
 * @PARAMETROS
 * 	path 	- el path es relativo al punto de montaje y es la forma mediante la cual debemos encontrar
 * 		  	  el archivo o directorio que nos solicitan
 * 	stbuf 	- estructura es la que debemos completar
 *
 * @DOCUMENTACION
 * 	memset	- http://www.tutorialspoint.com/c_standard_library/c_function_memset.htm
 *
 * @RETURN
 * 	O		- archivo/directorio encontrado
 * 	-ENOENT	- archivo/directorio no encontrado
 */
static int osada_getattr(const char * path, struct stat * stbuf) {
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	// path == "/" >> atributos punto de montaje
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, DEFAULT_FILE_PATH) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(DEFAULT_FILE_CONTENT);
	} else {
		res = -ENOENT;
	}
	return res;
}

/**
 * @DESC
 * 	Lista de archivos o directorios que se encuentra dentro de un directorio.
 *
 * @PARAMETROS
 * 	path	- el path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		   	  encontrar el archivo o directorio que nos solicitan
 * 	buf 	- buffer donde se colocaran los nombres de los archivos y directorios
 * 			  que esten dentro del directorio indicado por el path
 * 	filler 	- puntero a una función, la cual sabe como guardar una cadena dentro del campo buf
 *
 * @RETURN
 * 	O		- directorio encontrado
 * 	-ENOENT - directorio no encontrado
 */
static int osada_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi) {
	(void) offset;
	(void) fi;
	if (strcmp(path, "/") != 0)
		return -ENOENT;
	// "." y ".." entradas validas. La primera es una referencia al directorio donde estamos parados
	//  y la segunda indica el directorio padre.
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, DEFAULT_FILE_NAME, NULL, 0);
	return 0;
}

/**
 * @DESC
 * Pedido para tratar de abrir un archivo.
 *
 * @PARAMETROS
 * 	path 	- el path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		      encontrar el archivo o directorio que nos solicitan
 * 	fi 		- estructura que contiene la metadata del archivo indicado en el path
 *
 * @RETURN
 *	O 		- archivo encontrado
 *	-EACCES - archivo no accesible
 */
static int osada_open(const char * path, struct fuse_file_info * fi) {
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	return 0;
}

/**
 * @DESC
 * Pedido para obtener el contenido de un archivo.
 *
 * @PARAMETROS
 * 	path 	- el path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 			  encontrar el archivo o directorio que nos solicitan
 *	buf 	- buffer donde se va a guardar el contenido solicitado
 * 	size 	- cuanto tenemos que leer
 * 	offset 	- a partir de que posición del archivo tenemos que leer
 *
 * @RETURN
 * 	Si se usa el parametro direct_io los valores de retorno son:
 * 		0		- archivo encontrado
 * 		-ENOENT - ocurrió un error
 * 	Si el parametro direct_io no esta presente se retorna:
 * 		la cantidad de bytes leidos
 * 		-ENOENT	- ocurrió un error
 * 	Este comportamiento es igual para la funcion write
 */
static int osada_read(const char * path, char * buf, size_t size, off_t offset, struct fuse_file_info * fi) {
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

static struct fuse_operations osada_oper = {
	.getattr = osada_getattr,
	.readdir = osada_readdir,
	.open = osada_open,
	.read = osada_read
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

// Dentro de los argumentos que recibe nuestro programa obligatoriamente
// debe estar el path al directorio donde vamos a montar nuestro FS
int main(int argc, char* argv[]) {

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	// Limpio la estructura que va a contener los parámetros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1) {
		/** Parsing options error **/
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --welcome-msg el campo welcome_msg deberia tener el valor pasado
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}

	// Esta es la funcion principal de FUSE, es la que se encarga de realizar el montaje,
	// comunicarse con el kernel, delegar todo en varios threads
	return fuse_main(args.argc, args.argv, &osada_oper, NULL);

}


