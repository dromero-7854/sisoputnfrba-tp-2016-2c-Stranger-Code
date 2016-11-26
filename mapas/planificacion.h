/*
 * planificacion.h
 *
 *  Created on: 25/9/2016
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "nivel-test.h"

typedef struct pcb{
	char* nombre;
	int posx;
	int posy;
}t_pcb;

void planificar();
void verificarBloqueados();
int atender(t_entrenador* entrenador);
void ejecutarRafagaRR();
void ejecutarRafagaSRDF();
void atenderEntrenadoresSinDistanciaDefinida();
t_entrenador* buscarEntrenadorConMenorDistancia();
t_entrenador* buscarEntrenador(int socket, t_list* lista);
#endif /* PLANIFICACION_H_ */
