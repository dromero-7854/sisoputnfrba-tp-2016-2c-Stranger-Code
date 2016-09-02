/*
 * hola-commons.c
 *
 *  Created on: 30/8/2016
 *      Author: Carlos
 */
#include <stdio.h>
#include <stdlib.h>
#include <commons/temporal.h>

int main(int argc, char** argv){

	char texto[256];
	//llamanos a una función de commons
	char* tiempo = temporal_get_string_time();

	printf("Hola mundo!\n");

	printf("Escribi tu nombre: ");
	fgets(texto, 256, stdin);
	printf("Hola %sSon las %s\n", texto, tiempo);

	free(tiempo);

	return EXIT_SUCCESS;
}

