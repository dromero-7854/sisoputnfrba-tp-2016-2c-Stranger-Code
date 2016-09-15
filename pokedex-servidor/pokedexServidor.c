/*
 * pokedexServidor.c
 *
 *  Created on: 9/9/2016
 *      Author: utnso
 */


#include <bibliotecaCharMander.c>



int main(){
	int a;
	a = socket_servidor();
	aceptar_conexion(a);

	return 0;
}
