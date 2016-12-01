/*
 * solicitudes.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "solicitudes.h"
#include <stdint.h>

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
				case OC_ATRAPAR_POKEMON:
				case OC_OBTENER_MEDALLA:
				case OC_MEDALLA:
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

char handshake(int socketCliente, char* objetivos){

	uint8_t tam_msg, operation_code;
	char *buffer;
	void *paquete_a_mandar;

	//recv(socketCliente, &operation_code, sizeof(operation_code), 0);
	connection_recv(socketCliente, &operation_code, &buffer);
	if(operation_code != OC_UBICAR_ENTRENADOR){
		log_error(log_mapa, "codigo de operacion incorrecto en handshake");
		exit(1);
	}

	int offset = sizeof(operation_code);
	t_coor* coordenadas = malloc(sizeof(t_coor));
	coordenadas->x = 1;
	coordenadas->y = 1;
	paquete_a_mandar = malloc(sizeof(uint8_t) * 2 + tam_msg);
	tam_msg = sizeof(coordenadas);
	operation_code = OC_UBICACION_ENTRENADOR;
	memcpy(paquete_a_mandar, &operation_code, sizeof(uint8_t));
	memcpy(paquete_a_mandar + offset, &tam_msg, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete_a_mandar + offset, coordenadas, sizeof(t_coor));
	send(socketCliente, paquete_a_mandar, sizeof(uint8_t) + sizeof(uint8_t) + sizeof(t_coor), 0);

	operation_code = OC_OBTENER_OBJETIVOS;
	tam_msg = 0;

	memset(paquete_a_mandar, 0, sizeof(paquete_a_mandar));
	memcpy(paquete_a_mandar, &operation_code, sizeof(uint8_t));
	memcpy(paquete_a_mandar, &tam_msg, sizeof(uint8_t));
	send(socketCliente, paquete_a_mandar, sizeof(uint8_t) + sizeof(uint8_t), 0);

	connection_recv(socketCliente, &operation_code, &buffer);
	objetivos = buffer;

	//printf("FUNCIONOO\n");
	//printf("%s", buffer);
	//free(buffer);
	free(coordenadas);

	return *(buffer);
}

int atenderSolicitud(t_entrenador* entrenador){
	int recibido;
	uint8_t operation_code, tam_msg;
	t_coor* coor;
	int capturo_pokemon, offset;
	void *buffer, *paquete_a_mandar;
	log_trace(log_mapa, "atendiendo solicitud");
	recibido = connection_recv(entrenador->id, &operation_code, &buffer);

	switch(operation_code){
	case OC_UBICAR_POKENEST:
	{
		char pokenest_id = *((char*)buffer);
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

		capturo_pokemon = 0;
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
		capturo_pokemon = 0;
		break;
	}
		// volver a dibujar ??????
	case OC_ATRAPAR_POKEMON:
	{
		char pokenest_id = *((char*)buffer);
		//recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);
		t_infoPokemon* infopokemon = buscarPrimerPokemon(pokenest->listaPokemons);
		if(infopokemon == NULL){
			return -1;
		}
		list_add(entrenador->pokemons, infopokemon);
		pokenest->cantidad--;
		entrenador->objetivoActual++;

		int len = strlen(infopokemon->pokemon->species);


		uint8_t oc_send = OC_MENSAJE;

		uint8_t tamanio_mensaje = strlen("/home/utnso/git/tp-2016-2c-Stranger-Code/mapas/PuebloPaleta/PokeNests/Picachu/pikachu001.dat");
		int bytes_a_mandar = sizeof(uint8_t) * 2 + tamanio_mensaje;
		char* mensaje ;
		mensaje = strdup("/home/utnso/git/tp-2016-2c-Stranger-Code/mapas/PuebloPaleta/PokeNests/Picachu/pikachu001.dat");
		paquete_a_mandar = malloc(strlen(mensaje) + sizeof(uint8_t) * 2);
		memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));
		//int offset = len;
		memcpy(paquete_a_mandar + sizeof(uint8_t), &tamanio_mensaje, sizeof(uint8_t));
		//offset += sizeof(t_pokemon_type);
		memcpy(paquete_a_mandar + sizeof(uint8_t) * 2, mensaje, tamanio_mensaje);
		//offset += sizeof(t_pokemon_type);
		//memcpy(buffer + offset, &(infopokemon->pokemon->level), sizeof(t_level));
		send(entrenador->id, paquete_a_mandar, bytes_a_mandar, 0);
		entrenador->pokenest_buscada = NULL;
		//free(mensaje);
		free(paquete_a_mandar);
		free(buffer);
		capturo_pokemon = 1;
		break;
	}
	case OC_OBTENER_MEDALLA:
	{
		uint8_t oc_send = OC_MEDALLA;
		char* medalla = strdup("medalla-");
		char* extension_archivo_medalla = strdup(".jpg");
		uint8_t tamanio = strlen(ruta_mapa) + 1 + strlen(medalla) + strlen(nombre_mapa) + strlen(extension_archivo_medalla);
		char* ruta_medalla = malloc(tamanio + 1);
		snprintf(ruta_medalla, tamanio + 1, "%s/%s%s%s", ruta_mapa, medalla, nombre_mapa, extension_archivo_medalla);

		uint8_t tamanio_mensaje = strlen("/home/utnso/pokedex/Mapas/Pueblo Paleta/medalla.jpg");

		//char* mensaje  = malloc(tamanio_mensaje + 1);
		//mensaje = strdup("/home/utnso/git/tp-2016-2c-Stranger-Code/mapas/PuebloPaleta/PokeNests/Picachu/pikachu001.dat");
		paquete_a_mandar = malloc(tamanio + sizeof(uint8_t) * 2);
		memcpy(paquete_a_mandar, &oc_send, sizeof(uint8_t));
		//int offset = len;
		memcpy(paquete_a_mandar + sizeof(uint8_t), &tamanio, sizeof(uint8_t));
		//offset += sizeof(t_pokemon_type);
		memcpy(paquete_a_mandar + sizeof(uint8_t) * 2, ruta_medalla, tamanio);
		//offset += sizeof(t_pokemon_type);
		//memcpy(buffer + offset, &(infopokemon->pokemon->level), sizeof(t_level));
		send(entrenador->id, paquete_a_mandar, tamanio + sizeof(uint8_t) * 2, 0);
		capturo_pokemon = 2;
		break;
	}
	default:
		capturo_pokemon = 2;
		break;
	}

	return capturo_pokemon;
}

PokeNest* buscarPokenest(t_list* lista, char id){
	int _id_buscado(PokeNest* pokenest){
		return pokenest->id == id;
	}
	return list_find(lista, (void*) _id_buscado);
}

t_infoPokemon* buscarPrimerPokemon(t_list* listaDePokemons){
	t_list* listaAux;
	t_infoPokemon* infoPokemon;
	//list_add_all(listaAux, listaDePokemons);
	int _pokemon_de_menor_nombre(t_infoPokemon* infoPokemon1, t_infoPokemon* infoPokemon2){
		return infoPokemon1->nombre < infoPokemon2->nombre;
	}
	list_sort(listaDePokemons, (void*) _pokemon_de_menor_nombre);
	//infoPokemon = list_get(listaAux)
	return list_remove(listaDePokemons, 0);
}
