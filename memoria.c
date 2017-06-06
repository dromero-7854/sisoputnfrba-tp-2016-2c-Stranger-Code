/*
 ============================================================================
 Name        : memoria.c
 Authors     : Carlos Flores, Gustavo Tofaletti, Dante Romero
 Version     :
 Description : Memory Process
 ============================================================================
 */

#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <shared-library/memory_prot.h>
#include <shared-library/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread_db.h>
#include "memoria.h"

#define	SOCKET_BACKLOG 100
const char * DELAY_CMD = "delay";
const char * DUMP_CMD  = "dump";
const char * DUMP_CACHE_CMD  = "cache";
const char * DUMP_MEMORY_STRUCT_CMD = "struct";
const char * DUMP_MEMORY_CONTENT_CMD = "content";
const char * FLUSH_CMD = "flush";
const char * SIZE_CMD = "size";
const char * SIZE_MEMORY_CMD = "memory";

int listenning_socket;
t_memory_conf * memory_conf;
t_log * logger;
void * memory_ptr;

void * invert_table;
t_list * available_frame_list;
t_list * pages_per_process_list;





void * cache_memory;
void * cache_ctrl;
t_list * cache_x_process_list;







pthread_mutex_t m_lock; // TODO : checkpoint 3 es válido atender los pedidos de forma secuencial

void load_memory_properties(void);
void create_logger(void);
void print_memory_properties(void);
void load(void);
void console(void *);
void process_request(int *);
void mem_handshake(int *);
void inicialize_process(int *);
void read_page(int *);
void write_page(int *);
void assign_page(int *);
void finalize_process(int *);
int assign_pages_to_process(int, int);
int get_available_frame(void);
void inicialize_page(int, int);
int get_frame(int, int);

int main(int argc, char * argv[]) {
	load_memory_properties();
	create_logger();
	print_memory_properties();
	load();

	// console thread
	pthread_attr_t attr;
	pthread_t thread;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread, &attr, &console, NULL);
	pthread_attr_destroy(&attr);

	// socket thread
	int * new_sock;
	listenning_socket = open_socket(SOCKET_BACKLOG, (memory_conf->port));
	for (;;) {
		new_sock = malloc(1);
		* new_sock = accept_connection(listenning_socket);

		pthread_attr_t attr;
		pthread_t thread;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&thread, &attr, &process_request, (void *) new_sock);
		pthread_attr_destroy(&attr);
	}

	close_socket(listenning_socket);
	pthread_mutex_destroy(&m_lock); // TODO : checkpoint 3 es válido atender los pedidos de forma secuencial
	return EXIT_SUCCESS;
}

/**
 * @NAME load_memory_properties
 * @DESC Carga las configuraciones del proceso, desde un archivo de configuración
 */
void load_memory_properties(void) {
	t_config * conf = config_create("/home/utnso/memoria.cfg"); // TODO : Ver porque no lo toma del workspace
	memory_conf = malloc(sizeof(t_memory_conf));
	memory_conf->port = config_get_int_value(conf, "PUERTO");
	memory_conf->frames = config_get_int_value(conf, "MARCOS");
	memory_conf->frame_size = config_get_int_value(conf, "MARCO_SIZE");
	memory_conf->cache_entries = config_get_int_value(conf, "ENTRADAS_CACHE");
	memory_conf->cache_x_process = config_get_int_value(conf, "CACHE_X_PROC");
	memory_conf->memory_delay = config_get_int_value(conf, "RETARDO_MEMORIA");
	memory_conf->cache_algorithm = config_get_string_value(conf, "REEMPLAZO_CACHE");
	memory_conf->logfile = config_get_string_value(conf, "LOGFILE");
	memory_conf->consolefile = config_get_string_value(conf, "CONSOLEFILE");
}

/**
 * @NAME print_memory_properties
 * @DESC Guarda en el archivo log del proceso, las configuraciones para el proceso
 */
void print_memory_properties(void) {
	log_info(logger, "MEMORY PROCESS" );
	log_info(logger, "------ PORT: %u" , (memory_conf->port));
	log_info(logger, "------ FRAMES: %u" , (memory_conf->frames));
	log_info(logger, "------ FRAME SIZE: %u" , (memory_conf->frame_size));
	log_info(logger, "------ CACHE ENTRIES: %u" , (memory_conf->cache_entries));
	log_info(logger, "------ CACHE_X_PROC: %u" , (memory_conf->cache_x_process));
	log_info(logger, "------ MEMORY DELAY: %u" , (memory_conf->memory_delay));
	log_info(logger, "------ CACHE ALGORITHM: %s" , (memory_conf->cache_algorithm));
	log_info(logger, "------ LOGFILE: %s" , (memory_conf->logfile));
}

/**
 * @NAME create_logger
 * @DESC Crea el archivo log del proceso
 */
void create_logger(void) {
	logger = log_create((memory_conf->logfile), "MEMORY_PROCESS", true, LOG_LEVEL_TRACE);
}

/**
 * @NAME load
 * @DESC Genera las siguientes estruturas a utilizar durante el ciclo de vida del proceso:
 *					- memoria: bloque de memoria contigua, para simular la memoria principal (tamaño configurable por archivo de configuración)
 *					- tabla de páginas invertida
 *					- lista de frames disponibles
 *					- lista de cantidad de páginas asociadas a un proceso
 *       Inicia los semáforos a utilizar
 */
void load(void) {

	// memory
	memory_ptr = malloc((memory_conf->frames) * (memory_conf->frame_size));

	// TODO : la estructura de páginas se debe encuentrar en memoria principal
	// 				(checkpoint 3: no se requiere que la estructura de páginas se encuentre en memoria principal)

	// invert table
	invert_table = malloc((memory_conf->frames) * (sizeof(t_reg_invert_table)));
	t_reg_invert_table * inv_table_ptr = (t_reg_invert_table *) invert_table;

	// available frames list
	available_frame_list = list_create();

	int i;
	t_reg_invert_table * reg;
	for (i = 0; i < (memory_conf->frames); i++) {
		reg = malloc(sizeof(t_reg_invert_table));
		reg->frame = i;
		reg->page = 0;
		reg->pid = 0;
		memcpy(inv_table_ptr, reg, sizeof(t_reg_invert_table));
		free(reg);
		inv_table_ptr++;
		list_add(available_frame_list, i);
	}












	// cache memory
	cache_memory = malloc((memory_conf->cache_entries) * (sizeof(t_cache_memory)));
	cache_ctrl = malloc((memory_conf->cache_entries) * (sizeof(t_cache_ctrl)));
	// cache entries per process
	cache_x_process_list = list_create();

	t_cache_memory * cache_memory_ptr = (t_cache_memory *) cache_memory;
	t_cache_ctrl * cache_ctrl_ptr = (t_cache_manager *) cache_manager;

	int j;
	t_cache_memory * cache_reg;
	t_cache_ctrl * cache_ctrl_reg;
	for (j = 0; j < (memory_conf->cache_entries); j++) {

		cache_reg = malloc(sizeof(t_cache_memory));
		cache_reg->pid = -1;
		cache_reg->page = -1;
		cache_reg->content = malloc((memory_conf->frame_size));
		memcpy(cache_memory_ptr, cache_reg, sizeof(t_cache_memory));
		free(cache_reg);

		cache_ctrl_reg = malloc(sizeof(t_cache_manager));
		cache_ctrl_reg->used = false;
		cache_ctrl_reg->writed = false;
		cache_ctrl_reg->last_used = NULL;
		memcpy(cache_ctrl_ptr, cache_ctrl_reg, sizeof(t_cache_ctrl));
		free(cache_ctrl_reg);

		cache_memory_ptr++;
		cache_ctrl_ptr++;
	}














	// pages per process
	pages_per_process_list = list_create();

	pthread_mutex_init(&m_lock, NULL); // TODO : checkpoint 3 es válido atender los pedidos de forma secuencial
}

/**
 * @NAME process_request
 * @DESC Procesa solicitudes de un cliente
 * @PARAMS client_socket
 */
void process_request(int * client_socket) {
	int ope_code = recv_operation_code(client_socket, logger);
	while (ope_code != DISCONNECTED_CLIENT) {
		log_info(logger, "------ CLIENT %d >> operation code : %d", * client_socket, ope_code);
		pthread_mutex_lock(&m_lock); // TODO : checkpoint 3 es válido atender los pedidos de forma secuencial
		switch (ope_code) {
			case HANDSHAKE_OC:
				mem_handshake(client_socket);
				break;
			case INIT_PROCESS_OC:
				inicialize_process(client_socket);
				break;
			case READ_OC:
				read_page(client_socket);
				break;
			case WRITE_OC:
				write_page(client_socket);
				break;
			case ASSIGN_PAGE_OC:
				assign_page(client_socket);
				break;
			case END_PROCESS_OC:
				finalize_process(client_socket);
				break;
			default:;
		}
		pthread_mutex_unlock(&m_lock); // TODO : checkpoint 3 es válido atender los pedidos de forma secuencial
		ope_code = recv_operation_code(client_socket, logger);
	}
	close_client(* client_socket);
	free(client_socket);
	return;
}

/**
 * @NAME handshake
 * @DESC
 * @PARAMS client_socket
 */
void mem_handshake(int * client_socket) {
	handshake_resp(client_socket, (memory_conf->frame_size));
}

/**
 * @NAME inicialize_process
 * @DESC
 * @PARAMS client_socket
 */
void inicialize_process(int * client_socket) {
	t_init_process_request * init_req = init_process_recv_req(client_socket, logger);
	if ((init_req->exec_code) == DISCONNECTED_CLIENT) return;
	int resp_code = assign_pages_to_process((init_req->pid), (init_req->pages));
	free(init_req);
	init_process_send_resp(client_socket, resp_code);
}

/**
 * @NAME assign_pages_to_process
 * @DESC Asigna páginas a un proceso
 * @PARAMS 					pid : process id
 *				 required_pages : cantidad de páginas requeridas
 */
int assign_pages_to_process(int pid, int required_pages) {

	if ((available_frame_list->elements_count) < required_pages) { // check available frames for request
		return ENOSPC;
	}

	int located = -1;
	t_reg_pages_process_table * pages_per_process;
	if ((pages_per_process_list->elements_count) > 0) { // searching process on pages per process list
		int index = 0;
		while (located < 0 && index < (pages_per_process_list->elements_count)) {
			pages_per_process = (t_reg_pages_process_table *) list_get(pages_per_process_list, index);
			if ((pages_per_process->pid) == pid) {
				located = 1; // process exists on list
				break;
			}
			index++;
		}
	}

	if (located < 0) {
		// process  doesn't exist on pages per process list
		// creating node for list
		pages_per_process = (t_reg_pages_process_table *) malloc (sizeof(t_reg_pages_process_table));
		pages_per_process->pid = pid;
		pages_per_process->pages_count= 0;
		list_add(pages_per_process_list, pages_per_process);
	}

	// assigning frames for process pages
	int i;
	for (i = 0; i < required_pages; i++) {
		(pages_per_process->pages_count)++;
		inicialize_page(pid, ((pages_per_process->pages_count) - 1));
	}

	return SUCCESS;
}

/**
 * @NAME inicialize_page
 * @DESC Asigna un marco (frame) a una página de un proceso (utilizando la tabla de páginas invertida)
 * @PARAMS 						pid : process id
 *				  			   page : nro. página a asignarle un marco (frame)
 */
void inicialize_page(int pid, int page) {
	int free_frame = get_available_frame(); // getting available frame
	t_reg_invert_table * inv_table_aux_ptr = (t_reg_invert_table *) invert_table;
	inv_table_aux_ptr += free_frame;
	inv_table_aux_ptr->pid = pid;
	inv_table_aux_ptr->page = page;
}

/**
 * @NAME get_available_frame
 * @DESC Obtiene un marco disponible (frame)
 */
int get_available_frame(void) {
	return list_remove(available_frame_list, 0);
}

















/**
 * @NAME write_page
 * @DESC
 * @PARAMS client_socket
 */
void write_page(int * client_socket) {
	t_write_request * w_req = write_recv_req(client_socket, logger);
	if ((w_req->exec_code) == DISCONNECTED_CLIENT) return;
	if (writing_in_cache((w_req->pid), (w_req->page)) == CACHE_W_MISS)
		writing_in_memory((w_req->pid), (w_req->page), (w_req->offset), (w_req->size), (w_req->buffer));
	free(w_req->buffer);
	free(w_req);
	write_send_resp(client_socket, SUCCESS);
}

int writing_in_memory(int pid, int page, int offset, int size, void * buffer) {
	// getting associated frame
	int frame = get_frame(pid, page, WRITE);
	// getting write position
	void * write_pos = (memory_ptr + (frame * (memory_conf->frame_size)) + offset);
	// writing
	memcpy(write_pos, buffer, size);
	pthread_write_read_UNLOCK(frame);
	return EXIT_SUCCESS;
}

int writing_in_cache(t_write_request * w_req) {
	int cache_entry = get_cache_entry(w_req, WRITE);
	if (cache_entry == CACHE_W_MISS) {
		int lru_cache_entry = assign_cache_entry((w_req->pid));
		if (lru_cache_entry == MAX_ENTRIES_EXCEEDED) {
			return CACHE_W_MISS;
		} else {
			write_cache_memory(w_req, cache_entry, true);
			pthread_write_UNLOCK(lru_cache_entry);
			return CACHE_W_HIT;
		}
	} else {
		write_cache_memory(w_req, cache_entry, false);
		pthread_write_UNLOCK(cache_entry);
		return CACHE_W_HIT;
	}
}

int read_action(t_write_request * w_req, void * buff) {
	int cache_entry = get_cache_entry(w_req, READ);
	if (cache_entry == CACHE_W_MISS) {


		// reading from memory
		void * read_pos = memory_ptr +  (frame * (memory_conf->frame_size)) + (r_req->offset);
		memcpy(buff, read_pos, (r_req->size));


		// update cache memory
		int lru_cache_entry = assign_cache_entry((w_req->pid));
		if (lru_cache_entry != MAX_ENTRIES_EXCEEDED) {
			write_cache_memory(w_req, cache_entry, false);
			pthread_write_UNLOCK(lru_cache_entry);
		}
	} else {
		t_cache_memory * cache_memory_ptr = (t_cache_memory *) cache_memory;
		t_cache_ctrl * cache_ctrl_ptr = (t_cache_ctrl *) cache_ctrl;
		cache_memory_ptr += cache_entry;
		cache_ctrl_ptr += cache_entry;
		memcpy(buff, (cache_memory_ptr->content) + (r_req->offset), (r_req->size));
		cache_ctrl_ptr->str_last_used_time = temporal_get_string_time();
		pthread_read_write_UNLOCK(cache_entry);
		return CACHE_W_HIT;
	}
}









int get_cache_entry(t_write_request * w_req, int action) {
	t_cache_memory * cache_memory_ptr = (t_cache_memory *) cache_memory;
	int cache_entry = 0;
	while (cache_entry < (memory_conf->cache_entries)) {
		pthread_read_write_LOCK(cache_entry);
		if ((cache_reg->pid) == (w_req->pid) && (cache_reg->page) == (w_req->page)) break;
		pthread_read_write_UNLOCK(cache_entry);
		cache_entry++;
		cache_memory_ptr++;
	}
	return (cache_entry < (memory_conf->cache_entries)) ? cache_entry : CACHE_W_MISS;
}



int assign_cache_entry(int pid) {

	pthread_mutex_LOCK();
	bool process_exist_on_list = false;
	t_cache_x_process * cache_x_process;
	if ((cache_x_process_list->elements_count) > 0) { // searching cache entries per process
		int pos = 0;
		while (pos < (cache_x_process_list->elements_count)) {
			cache_x_process = (t_cache_x_process *) list_get(cache_x_process_list, pos);
			if ((cache_x_process->pid) == pid) {
				process_exist_on_list = true;
				if ((cache_x_process->entries) >= ((memory_conf->cache_x_process))) {
					pthread_mutex_UNLOCK();
					return MAX_ENTRIES_EXCEEDED;
				} else {
					(cache_x_process->entries)++;
				}
			}
		}
	}
	if (!process_exist_on_list) {
		// process  doesn't exist on list
		// creating node for list
		cache_x_process = (t_reg_pages_process_table *) malloc (sizeof(t_reg_pages_process_table));
		cache_x_process->pid = pid;
		cache_x_process->entries= 1;
		list_add(cache_x_process_list, cache_x_process);
	}
	pthread_mutex_UNLOCK();


	t_cache_ctrl * cache_ctrl_ptr = (t_cache_ctrl *) cache_ctrl;
	t_cache_ctrl * lru;
	int lru_cache_entry = -1;
	int cache_entry = 0;
	while (cache_entry < (memory_conf->cache_entries)) {
		pthread_write_LOCK(cache_entry);
		if (!(cache_ctrl_ptr->used)) {
			if (lru_cache_entry >= 0) pthread_write_UNLOCK(lru_cache_entry);
			return cache_entry;
		} else {
			if (lru_cache_entry < 0) {
				lru_cache_entry = cache_entry;
			} else {
				lru = (t_cache_ctrl *) cache_ctrl;
				lru += lru_cache_entry;
				if (strcmp((cache_ctrl_ptr->str_last_used_time),(lru->str_last_used_time)) < 0) {
					pthread_write_UNLOCK(lru_cache_entry);
					lru_cache_entry = cache_entry;
				} else {
					pthread_write_UNLOCK(cache_entry);
				}
			}
		}
		cache_entry++;
		cache_ctrl_ptr++;
	}

	return lru_cache_entry;

}


int write_cache_memory(t_write_request * w_req, int cache_entry, bool update_memory) {
	t_cache_memory * cache_memory_ptr = (t_cache_memory *) cache_memory;
	t_cache_ctrl * cache_ctrl_ptr = (t_cache_ctrl *) cache_ctrl;
	cache_memory_ptr += cache_entry;
	cache_ctrl_ptr += cache_entry;

	if (update_memory) {
		//updating memory
		writing_in_memory((cache_memory_ptr->pid), (cache_memory_ptr->page), 0, (memory_conf->frame_size), (cache_memory_ptr->content));
	}

	void * cache_write_pos = ((cache_memory_ptr->content) + (w_req->offset));
	// writing cache
	memcpy(cache_write_pos, (w_req->buffer), (w_req->size));
	cache_ctrl_ptr->used = true;
	cache_ctrl_ptr->writed = true;
	cache_ctrl_ptr->str_last_used_time = temporal_get_string_time();

	return EXIT_SUCCESS;
}























/**
 * @NAME get_frame
 * @DESC Obtiene el nro. de marco (frame) para un proceso y página (buscando en la tabla de páginas invertida)
 * @PARAMS 		pid : process id
 *			   page : nro. página del proceso
 */
int get_frame(int pid, int page, int lock_type) {
	// TODO : implementar función hash para realizar la búsqueda dentro de la tabla de páginas invertida
	t_reg_invert_table * inv_table_ptr = (t_reg_invert_table *) invert_table;
	int frame = 0;
	while (frame < (memory_conf->frames)) {
		pthread_write_read_LOCK(frame, lock_type);
		if (((inv_table_ptr->pid) == pid) && ((inv_table_ptr->page) == page)) {
			break;
		} else {
			pthread_write_read_UNLOCK(frame, lock_type);
			frame++;
			inv_table_ptr++;
		}
	}
	return frame;
}

/**
 * @NAME read_page
 * @DESC
 * @PARAMS client_socket
 */
void read_page(int * client_socket) {
	t_read_request * r_req = read_recv_req(client_socket, logger);
	if ((r_req->exec_code) == DISCONNECTED_CLIENT) return;

	void * buff = malloc(sizeof(char) * (r_req->size));
	int frame = get_frame((r_req->pid), (r_req->page), READ);
	void * read_pos = memory_ptr +  (frame * (memory_conf->frame_size)) + (r_req->offset);
	memcpy(buff, read_pos, (r_req->size));
	free(r_req);

	read_send_resp(client_socket, SUCCESS, (r_req->size), buff);
	free(buff);
}

/**
 * @NAME assign_page
 * @DESC
 * @PARAMS client_socket
 */
void assign_page(int * client_socket) {
	t_assign_pages_request * assign_req = init_process_recv_req(client_socket, logger);
	if ((assign_req->exec_code) == DISCONNECTED_CLIENT) return;
	int resp_code = assign_pages_to_process((assign_req->pid), (assign_req->pages));
	free(assign_req);
	init_process_send_resp(client_socket, resp_code);
}

/**
 * @NAME finalize_process
 * @DESC
 * @PARAMS client_socket
 */
void finalize_process(int * client_socket) {
	t_finalize_process_request * finalize_req = finalize_process_recv_req(client_socket, logger);
	if ((finalize_req->exec_code) == DISCONNECTED_CLIENT) return;
	int resp_code = cleaning_process_entries((finalize_req->pid));
	finalize_process_send_resp(client_socket, resp_code);
}

int cleaning_process_entries(int pid) {
	t_reg_pages_process_table * pages_per_process;
	int index = 0;
	while (index < (pages_per_process_list->elements_count)) { // searching process on pages per process list
		pages_per_process = (t_reg_pages_process_table *) list_get(pages_per_process_list, index);
		if ((pages_per_process->pid) == pid) {
			pages_per_process = list_remove(pages_per_process_list, index);
			free(pages_per_process);
			break;
		}
		index++;
	}

	t_reg_invert_table * inv_table_ptr = (t_reg_invert_table *) invert_table;
	int frame = 0;
	while (frame < (memory_conf->frames)) {
		if (((inv_table_ptr->pid) == pid)) {
			inv_table_ptr->pid = 0;
			inv_table_ptr->page = 0;
			list_add(available_frame_list, frame);
		}
		frame++;
		inv_table_ptr++;
	}
	return SUCCESS;
}


















void console(void * unused) {
	t_log * console = log_create((memory_conf->consolefile), "MEMORY_CONSOLE", true, LOG_LEVEL_TRACE);
	char * input = NULL;
	char * command = NULL;
	char * param = NULL;
	size_t len = 0;
	ssize_t read;
	while ((read = getline(&input, &len, stdin)) != -1) {
		if (read > 0) {
			input[read-1] = '\0';

			char * token = strtok(input, " ");
			if (token != NULL) command = token;
			token = strtok(NULL, " ");
			if (token != NULL) param = token;
			if (strcmp(command, DELAY_CMD) == 0) {
				log_info(console, "DELAY COMMAND");
				log_info(console, "END DELAY COMMAND");
			} else if (strcmp(command, DUMP_CMD) == 0) {
				// DUMP COMMAND
				log_info(console, "DUMP COMMAND");
				if (strcmp(param, DUMP_CACHE_CMD) == 0) {
					log_info(console, "CACHE CONTENT");
					log_info(console, "END CACHE CONTENT");
				} else if (strcmp(param, DUMP_MEMORY_STRUCT_CMD) == 0) {
					log_info(console, "INVERT TABLE");
					log_info(console, "    #FRAME        PID           #PAG");
					log_info(console, "    ══════════    ══════════    ══════════");
					t_reg_invert_table * inv_table_ptr = (t_reg_invert_table *) invert_table;
					int frame = 0;
					while (frame < (memory_conf->frames)) {
						log_info(console, "    %10d    %10d    %10d", (inv_table_ptr->frame), (inv_table_ptr->pid), (inv_table_ptr->page));
						frame++;
						inv_table_ptr++;
					}
					log_info(console, "END INVERT TABLE");
				} else if (strcmp(param, DUMP_MEMORY_CONTENT_CMD) == 0) {
					log_info(console, "MEMORY CONTENT");
					void * read_pos = memory_ptr;
					char * frame_content;
					int frame = 0;
					while (frame < (memory_conf->frames)) {
						frame_content = malloc((memory_conf->frame_size) + 1);
						memcpy(frame_content, read_pos, (memory_conf->frame_size));
						frame_content[(memory_conf->frame_size)] = "\0";
						//log_info(console, frame_content);
						printf(":%s\n", frame_content);
						free(frame_content);
						frame++;
						read_pos += (memory_conf->frame_size);
					}
					log_info(console, "END MEMORY CONTENT");
				}
				log_info(console, "END DUMP COMMAND");
			} else if (strcmp(command, FLUSH_CMD) == 0) {
				log_info(console, "FLUSH COMMAND");
				log_info(console, "END FLUSH COMMAND");
			} else if (strcmp(command, SIZE_CMD) == 0) {
				// SIZE COMMAND
				log_info(console, "SIZE COMMAND");
				if (strcmp(param, SIZE_MEMORY_CMD) == 0) {
					log_info(console, "MEMORY SIZE");
					log_info(console, "FRAMES ------------> %10d", (memory_conf->frames));
					log_info(console, "AVAILABLE FRAMES --> %10d", (available_frame_list->elements_count));
					log_info(console, "BUSY FRAMES -------> %10d", ((memory_conf->frames) - (available_frame_list->elements_count)));
					log_info(console, "END MEMORY SIZE");
				} else {

				}
				log_info(console, "END SIZE COMMAND");
			}
		}
	}
	free(command);
}
