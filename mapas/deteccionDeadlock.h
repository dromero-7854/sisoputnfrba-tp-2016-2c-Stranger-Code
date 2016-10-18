/*
 * deteccionDeadlock.h
 *
 *  Created on: 18/10/2016
 *      Author: utnso
 */

#ifndef MAPAS_DETECCIONDEADLOCK_H_
#define MAPAS_DETECCIONDEADLOCK_H_

#include <commons/collections/list.h>
#include <nivel-test.h>

void detectarDeadlock(t_combo comboLista);
t_entrenador * mandarAPelear(t_entrenador entrenador1, t_entrenador entrenador2);
void matarEntrenador(t_entrenador *entrenador);

#endif /* MAPAS_DETECCIONDEADLOCK_H_ */
