#include <stdio.h>

int main(int argc, char **argv) {

	char texto[25];

	printf("Escribi tu nombre: ");
	fgets(texto, 256, stdin);

	printf("Hola %s!", texto);

	return 0;
}
