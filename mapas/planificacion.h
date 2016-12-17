/*
 * planificacion.h
 *
 *  Created on: 25/9/2016
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "nivel-test.h"
#include <stdint.h>
//#include <commons/log.h>
//#include <commons/collections/queue.h>
/*
#define NO_ENCONTRO_POKEMON -1
#define TURNO_NORMAL 0
#define CAPTURO_POKEMON 1
#define DESCONEXION 2*/

typedef struct pcb{
	char* nombre;
	int posx;
	int posy;
}t_pcb;

int fd_bloq_max;
fd_set set_bloqueados, set_bloq_master;

void planificar();
void verificarBloqueados();
int atender(t_entrenador* entrenador);
void ejecutarRafagaRR();
void ejecutarRafagaSRDF();
void atenderEntrenadoresSinDistanciaDefinida();
t_entrenador* buscarEntrenadorConMenorDistancia();
t_entrenador* buscarEntrenadorSinDistanciaDefinida();
t_entrenador* buscarEntrenador(int socket, t_list* lista);
void desbloquearEntrenador(t_entrenador* entrenador);
void bloquearEntrenador(t_entrenador* entrenador);
void liberarRecursos2(t_entrenador* entrenadorLiberado);

#endif /* PLANIFICACION_H_ */
