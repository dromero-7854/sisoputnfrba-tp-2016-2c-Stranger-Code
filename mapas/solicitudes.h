/*
 * solicitudes.h
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#ifndef SOLICITUDES_H_
#define SOLICITUDES_H_

#define SOLICITA_UBICACION_POKENEST 0
#define NOTIFICA_MOVIMIENTO 1
#define CAPTURA_POKEMON 2
#define UP 72
#define RIGHT 77
#define DOWN 80
#define LEFT 75

extern t_list* listaPokenests;

PokeNest* buscarPokenest(t_list* lista, char id);
t_infoPokemon* buscarPrimerPokemon(t_list* listaPokemons);
int atenderSolicitud(t_entrenador* entrenador);
#endif /* SOLICITUDES_H_ */
