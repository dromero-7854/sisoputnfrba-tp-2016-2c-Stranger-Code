/*
 * solicitudes.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "solicitudes.h"


int connection_recv(int socket, uint8_t* operation_code_value, void** message){
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

	status = recv(socket, operation_code_value, prot_ope_code_size, 0);
	if (status <= 0) {
		log_error(log_mapa, "se desconecto alguien en socket: d", socket);
		return 0;
	} else {
		ret = ret + status;
		status = recv(socket, &message_size, prot_message_size, 0);
		if (status <= 0) {
			log_error(log_mapa, "se desconecto alguien en socket: d", socket);
			return 0;
		} else {
			ret = ret + status;
			//message = (void*) malloc(message_size);
			switch ((int)*operation_code_value) {
				case OC_UBICACION_POKENEST:
				case OC_UBICACION_ENTRENADOR:
					coor = malloc(message_size);
					status = recv(socket, coor, message_size, 0);
					if(status > 0){
						*message = coor;
					}
					if(status <= 0){
						log_error(log_mapa, "se desconecto alguien en socket: d", socket);
						return 0;
					}
					//free(coor);
					break;
				case OC_UBICAR_POKENEST:
				case OC_UBICAR_ENTRENADOR:
				case OC_AVANZAR_POSICION:
				case OC_CANTIDAD_DEADLOCK:
				case OC_ATRAPAR_POKEMON:
				case OC_OBTENER_MEDALLA:
				case OC_MEDALLA:
				case OC_POKEMON:
				case OC_MENSAJE:
					buffer = malloc(message_size + 1);
					if(message_size > 0){
						status = recv(socket, buffer, message_size, 0);
					}
					if(status <= 0){
						log_error(log_mapa, "se desconecto alguien en socket: d", socket);
						return 0;
					}
					if(status > 0){
						buffer[message_size] = '\0';
						*message = buffer;
					//	*message = malloc(message_size + 1);
					//	memcpy(*message, buffer, message_size);
					//	(*message)[message_size] = '\0';
					}
					//free(buffer);
					break;
				default:
					printf("ERROR: Socket %d, Invalid operation code...\n", socket);
					break;
			}

			if (status <= 0) {
				printf("ERROR: Socket %d, no message...\n", socket);
				ret = status;
			}else{
				ret = ret + status;
			}

		}
	}

	return ret;
}

void handshake(int socketCliente, char* simbolo, char** nombre_entrenador){

	uint8_t tam_msg, operation_code;
	char *buffer,*simbolo_recibido;
	void *paquete_a_mandar;

	connection_recv(socketCliente, &operation_code, &simbolo_recibido);
	*simbolo = *simbolo_recibido;
	if(operation_code != OC_UBICAR_ENTRENADOR){
		log_error(log_mapa, "codigo de operacion incorrecto en handshake");
		exit(1);
	}

	int offset = sizeof(operation_code);
	t_coor* coordenadas = malloc(sizeof(t_coor));
	coordenadas->x = 1;
	coordenadas->y = 1;
	tam_msg = sizeof(coordenadas);
	paquete_a_mandar = malloc(sizeof(uint8_t) * 2 + tam_msg);
	operation_code = OC_UBICACION_ENTRENADOR;
	memcpy(paquete_a_mandar, &operation_code, sizeof(uint8_t));
	memcpy(paquete_a_mandar + offset, &tam_msg, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete_a_mandar + offset, coordenadas, sizeof(t_coor));
	send(socketCliente, paquete_a_mandar, sizeof(uint8_t) + sizeof(uint8_t) + sizeof(t_coor), 0);

	connection_recv(socketCliente, &operation_code, &buffer);
	if(operation_code != OC_OBTENER_MEDALLA){
		log_error(log_mapa, "codigo de operacion incorrecto en handshake");
		exit(1);
	}

	enviar_ruta_medalla(socketCliente);
	connection_recv(socketCliente, &operation_code, nombre_entrenador);

	free(paquete_a_mandar);
	free(simbolo_recibido);
	free(buffer);
	free(coordenadas);

//	return *(buffer);
}

int atenderSolicitud(t_entrenador* entrenador){
	int recibido;
	uint8_t operation_code, tam_msg;
	t_coor* coor;
	int respuesta, offset;
	void *buffer, *paquete_a_mandar;
	log_trace(log_mapa, "atendiendo solicitud");
	recibido = connection_recv(entrenador->id, &operation_code, &buffer);

	if(recibido == 0){
		return DESCONEXION;
	}
	switch(operation_code){
	case OC_UBICAR_POKENEST:
	{
		char pokenest_id = *((char*)buffer);
		//entrenador->pokenest_buscada = pokenest_id;
		//recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		//void *buffer;
		//recibidos = recv(entrenador->id, buffer, 1, 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);

		t_coor* coordenadas_pokenest = malloc(sizeof(t_coor));
		coordenadas_pokenest->x = pokenest->posx;
		coordenadas_pokenest->y = pokenest->posy;

		uint8_t oc_send = OC_UBICACION_POKENEST;
		tam_msg = sizeof(t_coor);
		paquete_a_mandar = malloc(sizeof(uint8_t) * 2+ sizeof(t_coor));
		offset = sizeof(uint8_t);
		memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));
		memcpy(paquete_a_mandar + offset, &tam_msg, sizeof(uint8_t));
		offset += sizeof(uint8_t);
		memcpy(paquete_a_mandar + offset, coordenadas_pokenest, sizeof(t_coor));
		send(entrenador->id, paquete_a_mandar, sizeof(uint8_t) + sizeof(uint8_t) + sizeof(t_coor), 0);
		entrenador->pokenest_buscada = pokenest_id;
		free(coordenadas_pokenest);
		free(paquete_a_mandar);
		free(buffer);

		respuesta = TURNO_NORMAL;
		break;
	}
	case OC_AVANZAR_POSICION:
	{

		switch(*(uint8_t*)buffer){
		case MOVE_UP:
			entrenador->posy--;
			break;
		case MOVE_DOWN:
			entrenador->posy++;
			break;
		case MOVE_RIGHT:
			entrenador->posx++;
			break;
		case MOVE_LEFT:
			entrenador->posx--;
			break;
		default:
			fprintf(stderr, "no se recibio una direccion adecuada");
			//log_error
		}
		coor = malloc(sizeof(t_coor));
		coor->x = entrenador->posx;
		coor->y = entrenador->posy;

		uint8_t oc_send = OC_AVANZAR_POSICION;
		tam_msg = sizeof(t_coor);
		paquete_a_mandar = malloc(sizeof(uint8_t) * 2+ sizeof(t_coor));
		offset = sizeof(uint8_t);
		memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));
		memcpy(paquete_a_mandar + offset, &tam_msg, sizeof(uint8_t));
		offset += sizeof(uint8_t);
		memcpy(paquete_a_mandar + offset, coor, sizeof(t_coor));
		send(entrenador->id, paquete_a_mandar, sizeof(uint8_t) + sizeof(uint8_t) + sizeof(t_coor), 0);
		free(coor);
		free(paquete_a_mandar);
		free(buffer);
		MoverPersonaje(items, entrenador->simbolo, entrenador->posx, entrenador->posy);
		respuesta = TURNO_NORMAL;
		break;
	}
		// volver a dibujar ??????
	case OC_ATRAPAR_POKEMON:
	{
		char pokenest_id = *((char*)buffer);
	//	recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);
		t_infoPokemon* infopokemon = buscarPrimerPokemon(pokenest->listaPokemons);
		if(infopokemon == NULL){
			respuesta = NO_ENCONTRO_POKEMON;
			break;
		}
		list_add(entrenador->pokemons, infopokemon);

		pthread_mutex_lock(&mutex_lista_pokenest);
		pokenest->cantidad--;
		pthread_mutex_unlock(&mutex_lista_pokenest);

		notificar_captura_pokemon(infopokemon, entrenador);
//		int len = strlen(infopokemon->pokemon->species);
		restarRecurso(items, infopokemon->id_pokenest);

		free(buffer);
		respuesta = CAPTURO_POKEMON;
		break;
	}
	case OC_ATRAPAR_ULTIMO_POKEMON:
	{
		char pokenest_id = *((char*)buffer);
		//recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);
		t_infoPokemon* infopokemon = buscarPrimerPokemon(pokenest->listaPokemons);
		if(infopokemon == NULL){
			entrenador->ultimo_pokemon = 1;
			respuesta = NO_ENCONTRO_POKEMON;
			break;
		}
		list_add(entrenador->pokemons, infopokemon);

		pthread_mutex_lock(&mutex_lista_pokenest);
		pokenest->cantidad--;
		pthread_mutex_unlock(&mutex_lista_pokenest);

		notificar_captura_pokemon(infopokemon, entrenador);
		restarRecurso(items, infopokemon->id_pokenest);

		connection_recv(entrenador->id, &operation_code, &buffer);
		if(operation_code != OC_OBTENER_CANTIDAD_DEADLOCK){
			log_error(log_mapa, "error durante solicitud de deadlocks");
			exit(1);
		}

	//	enviar_cant_deadlocks(entrenador);
		//enviar_ruta_medalla(entrenador);
		respuesta = DESCONEXION;
		free(buffer);
		break;
	}
	default:
		respuesta = TURNO_NORMAL;
		free(buffer);
		break;
	}

	return respuesta;
}

PokeNest* buscarPokenest(t_list* lista, char id){
	int _id_buscado(PokeNest* pokenest){
		return pokenest->id == id;
	}
	return list_find(lista, (void*) _id_buscado);
}

t_infoPokemon* buscarPrimerPokemon(t_list* listaDePokemons){
	int _pokemon_de_menor_nombre(t_infoPokemon* infoPokemon1, t_infoPokemon* infoPokemon2){
		return comparar_strings(infoPokemon1->nombre, infoPokemon2->nombre);
	}
	list_sort(listaDePokemons, (void*) _pokemon_de_menor_nombre);
	return list_remove(listaDePokemons, 0);
}

void notificar_captura_pokemon(t_infoPokemon* infopokemon, t_entrenador* entrenador){
	char *rutaPokenests, *rutaPokemon;
	void* paquete_a_mandar;

	uint8_t oc_send = OC_POKEMON;

	rutaPokenests = getRutaPokenests();
	rutaPokemon = getRutaPokemon(rutaPokenests, infopokemon->pokemon->species);
	free(rutaPokenests);
	string_append_with_format(&rutaPokemon, "/%s", infopokemon->nombre);
	log_trace(log_mapa,"rutaPokemon: %s",rutaPokemon);
	uint8_t tamanio_mensaje = strlen(rutaPokemon);
	int bytes_a_mandar = sizeof(uint8_t) * 2 + tamanio_mensaje;

	paquete_a_mandar = malloc(tamanio_mensaje + sizeof(uint8_t) * 2);
	memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));

	memcpy(paquete_a_mandar + sizeof(uint8_t), &tamanio_mensaje, sizeof(uint8_t));

	memcpy(paquete_a_mandar + sizeof(uint8_t) * 2, rutaPokemon, tamanio_mensaje);

	send(entrenador->id, paquete_a_mandar, bytes_a_mandar, 0);
	entrenador->pokenest_buscada = NULL;
	free(rutaPokemon);
	free(paquete_a_mandar);
}

void enviar_ruta_medalla(int socket){
	uint8_t oc_send = OC_MEDALLA;
	void* paquete_a_mandar;
	char* medalla = strdup("medalla-");
	char* extension_archivo_medalla = strdup(".jpg");

	char* ruta_medalla;

	ruta_medalla = getRutaMapa(pto_montaje, nombre_mapa);
	string_append_with_format(&ruta_medalla, "/medalla-%s.jpg", nombre_mapa);

	uint8_t tamanio = strlen(ruta_medalla);
	paquete_a_mandar = malloc(tamanio + sizeof(uint8_t) * 2);
	memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));

	memcpy(paquete_a_mandar + sizeof(uint8_t), &tamanio, sizeof(uint8_t));

	memcpy(paquete_a_mandar + sizeof(uint8_t) * 2, ruta_medalla, tamanio);

	send(socket, paquete_a_mandar, tamanio + sizeof(uint8_t) * 2, 0);
	free(medalla);
	free(extension_archivo_medalla);
	free(ruta_medalla);
	free(paquete_a_mandar);
}

void enviar_cant_deadlocks(t_entrenador* entrenador){
	void* paquete_a_mandar = malloc(10);
	uint8_t oc_send = OC_CANTIDAD_DEADLOCK;
	uint8_t tamanio = sizeof(char);
	int offset;
	memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));
	offset = sizeof(uint8_t);
	memcpy(paquete_a_mandar + offset, &tamanio, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete_a_mandar + offset, (char*)(entrenador->cantDeadlocks), sizeof(int));
	offset += sizeof(char);

	send(entrenador->id, paquete_a_mandar, offset, 0);
}
