/*
 * solicitudes.h
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#ifndef SOLICITUDES_H_
#define SOLICITUDES_H_

#include "nivel-test.h"

#define TURNO_NORMAL 1
#define DESCONEXION -1
#define CAPTURO_POKEMON 2
#define NO_ENCONTRO_POKEMON -2


extern t_list* listaPokenests;
//char handshake(int socketCliente, char* objetivos);
void handshake(int socketCliente, char* simbolo, char** nombre_entrenador);
int connection_recv(int socket, uint8_t* operation_code_value, void** message);
PokeNest* buscarPokenest(t_list* lista, char id);
t_infoPokemon* buscarPrimerPokemon(t_list* listaPokemons);
int atenderSolicitud(t_entrenador* entrenador);
void notificar_captura_pokemon(t_infoPokemon* infopokemon, t_entrenador* entrenador);
void enviar_ruta_medalla(int socket);

#endif /* SOLICITUDES_H_ */
