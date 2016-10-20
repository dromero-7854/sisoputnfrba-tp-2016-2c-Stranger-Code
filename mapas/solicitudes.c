/*
 * solicitudes.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "nivel-test.h"
#include "solicitudes.h"
#include <stdint.h>



void atenderSolicitud(t_entrenador* entrenador, int* captura_pokemon){
	int recibidos;
	uint32_t header, eje;

	recibidos = recv(entrenador->fd, &header, sizeof(uint32_t), 0);

	switch(header){
	case SOLICITA_UBICACION_POKENEST:
	{
		recv(entrenador->fd, &header, sizeof(uint32_t), 0);
		void *buffer = malloc(header);
		recibidos = recv(entrenador->fd, buffer, header, 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, header);

		void *msg_coordenadas = malloc(sizeof(uint32_t) * 2);
		memcpy(msg_coordenadas, pokenest->posx, sizeof(uint32_t));
		memcpy(msg_coordenadas + sizeof(uint32_t), pokenest->posy, sizeof(uint32_t));

		send(entrenador->fd, msg_coordenadas, sizeof(uint32_t) * 2, 0);
		*captura_pokemon = 0;
		break;
	}
	case NOTIFICA_MOVIMIENTO:
	{
		recv(entrenador->fd, &eje, sizeof(uint32_t), 0);
		if (eje == 0){
			entrenador->posx++;
		} else {
			entrenador->posy++;
		}
		*captura_pokemon = 0;
		break;
	}
		// volver a dibujar ??????
	case CAPTURA_POKEMON:
	{
		recv(entrenador->fd, &header, sizeof(uint32_t), 0);
		PokeNest* pokenest = buscarPokenest(listaPokenests, header);
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
		send(entrenador->fd, buffer, bytes_a_mandar, 0);
		*captura_pokemon = 1;
		break;
	}
	}
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
