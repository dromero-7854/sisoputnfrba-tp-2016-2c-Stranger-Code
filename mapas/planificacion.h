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

void handshake(int socketCliente, t_log* logger);
void planificar();
void verificarBloqueados();
t_entrenador* atender(t_queue* cola);
void ejecutarRafagaRR();
void ejecutarRafagaSRDF();
void atenderEntrenadoresSinDistanciaDefinida();
t_entrenador* buscarEntrenadorConMenorDistancia();
#endif /* PLANIFICACION_H_ */
