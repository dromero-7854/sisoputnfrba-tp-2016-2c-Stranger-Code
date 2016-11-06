/*
 * solicitudes.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "solicitudes.h"
#include <stdint.h>



int atenderSolicitud(t_entrenador* entrenador){
	int recibidos;
	uint32_t operation_code, direccion;
	int capturo_pokemon;
	recibidos = recv(entrenador->id, &operation_code, sizeof(uint32_t), 0);

	switch(operation_code){
	case OC_UBICAR_POKENEST:
	{
		char pokenest_id;
		recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		void *buffer;
		//recibidos = recv(entrenador->id, buffer, 1, 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);

		t_coor* coordenadas_pokenest = malloc(sizeof(t_coor));
		coordenadas_pokenest->x = pokenest->posx;
		coordenadas_pokenest->y = pokenest->posy;

		uint8_t oc_send = OC_UBICACION_POKENEST;
		buffer = malloc(sizeof(uint8_t) + sizeof(t_coor));
		memcpy(buffer, &oc_send, sizeof(uint8_t));
		memcpy(buffer + sizeof(uint8_t), coordenadas_pokenest, sizeof(t_coor));
		send(entrenador->id, buffer, sizeof(t_coor), 0);
		free(coordenadas_pokenest);
		free(buffer);
		capturo_pokemon = 0;
		break;
	}
	case OC_AVANZAR_POSICION:
	{
		recv(entrenador->id, &direccion, sizeof(uint32_t), 0);
		switch(direccion){
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
		char pokenest_id;
		recv(entrenador->id, &pokenest_id, sizeof(char), 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, pokenest_id);
		t_infoPokemon* infopokemon = buscarPrimerPokemon(pokenest->listaPokemons);

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
	int _id_buscado(PokeNest* pokenest, char id){
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
