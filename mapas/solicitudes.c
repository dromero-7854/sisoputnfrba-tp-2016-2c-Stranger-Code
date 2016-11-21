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
		printf("ERROR: Socket %d, disconnected...\n", socket);
	} else {
		ret = ret + status;
		status = recv(socket, &message_size, prot_message_size, 0);
		if (status <= 0) {
			printf("ERROR: Socket %d, no message size...\n", socket);
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

char handshake(int socketCliente, t_log* logger){

	uint8_t tam_msg;
	uint8_t operation_code;
	char *buffer;
	void *paquete_a_mandar;

	//recv(socketCliente, &operation_code, sizeof(operation_code), 0);
	connection_recv(socketCliente, &operation_code, &buffer);
	if(operation_code != OC_UBICAR_ENTRENADOR){
		log_error(logger, "codigo de operacion incorrecto en handshake");
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

	//printf("FUNCIONOO\n");
	//printf("%s", buffer);
	//free(buffer);
	free(coordenadas);

	return *(buffer);
}

int atenderSolicitud(t_entrenador* entrenador){
	int recibidos;
	uint8_t operation_code, tam_msg;
	int capturo_pokemon, offset;
	void *buffer, *paquete_a_mandar;
	connection_recv(entrenador->id, &operation_code, &buffer);

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
		free(coordenadas_pokenest);
		free(buffer);
		capturo_pokemon = 0;
		break;
	}
	case OC_AVANZAR_POSICION:
	{

		switch(*((int*)buffer)){
		case MOVE_UP:
			entrenador->posy--;
		case MOVE_DOWN:
			entrenador->posy++;
		case MOVE_RIGHT:
			entrenador->posx++;
		case MOVE_LEFT:
			entrenador->posx--;
		default:
			fprintf(stderr, "no se recibio una direccion adecuada");
			//log_error
		}
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
		list_add(entrenador->pokemons, infopokemon);

		int len = strlen(infopokemon->pokemon->species);
		int bytes_a_mandar = len + sizeof(t_level) + sizeof(t_pokemon_type) * 2;

		void* buffer = malloc(bytes_a_mandar);
		memcpy(buffer, infopokemon->pokemon->species, len);
		int offset = len;
		memcpy(buffer + offset, &(infopokemon->pokemon->type), sizeof(t_pokemon_type));
		offset += sizeof(t_pokemon_type);
		memcpy(buffer + offset, &(infopokemon->pokemon->second_type), sizeof(t_pokemon_type));
		offset += sizeof(t_pokemon_type);
		memcpy(buffer + offset, &(infopokemon->pokemon->level), sizeof(t_level));
		send(entrenador->id, buffer, bytes_a_mandar, 0);
		capturo_pokemon = 1;
	}
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
	int _pokemon_de_menor_nombre(t_infoPokemon* infoPokemon1, t_infoPokemon* infoPokemon2){
		return infoPokemon1->nombre < infoPokemon2->nombre;
	}
	return list_find(listaDePokemons, (void*) _pokemon_de_menor_nombre);
}
