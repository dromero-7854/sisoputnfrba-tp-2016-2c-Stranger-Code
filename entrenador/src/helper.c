/*
 * helper.c
 *
 *  Created on: 21/10/2016
 *      Author: utnso
 */
#include "helper.h"

t_log* crear_log(char* nombreEntrenador, char* pathConfig) {
	t_log* logger;

	char temp_file[255];
	strcpy(temp_file, "entrenador_");
	strcat(temp_file, nombreEntrenador);
	strcat(temp_file, ".log");
	logger = log_create(temp_file, "TEST", true, LOG_LEVEL_INFO);
	log_info(logger, "Nombre del Entrenador: %s", nombreEntrenador);
	log_info(logger, "Metadata del Entrenador: %s", pathConfig);
	log_info(logger, "Log del Entrenador: %s", temp_file);

	return logger;
}

int copy_file(char* f_origen,char* f_destino){
	FILE *fp_org,*fp_dest;

	if(!(fp_org=fopen(f_origen,"rb")) || !(fp_dest=fopen(f_destino,"wb")))
	{
		perror("Error de apertura de ficheros");
		exit(EXIT_FAILURE);
	}

	/* Para meter lo que vamos leyendo del fichero */
	char buffer[1000];
	/* Para guardar el número de items leidos o si ha habido error */
	int totalLeidos;
	int leidos = 0;
	leidos = fread (buffer, 1, 1000, fp_org);

	/* Mientras se haya leido algo ... */
	while (leidos!=0)
	{
		totalLeidos = totalLeidos + leidos;
	   /* ... meterlo en el fichero destino */
	   fwrite (buffer, 1, leidos, fp_dest);
	   /* y leer siguiente bloque */
	   leidos = fread (buffer, 1, 1000, fp_org);
	}

	if(ferror(fp_org) || ferror(fp_org)){
		return 1;
	} else {
		fclose(fp_org);
		fclose(fp_dest);
		return 0;
	}
}

void createDir(char* path) {
	struct stat st = { 0 };
	if (stat(path, &st) == -1) {
		mkdir(path, 0700);
	}
}

void deleteDir(char* path) {
	DIR *d;
	struct dirent *dir;
	char file[256];

	if( (d = opendir(path)) == NULL){
		printf("Directorio %s vacio.", path);
		closedir(d);
		return;
	}

	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (strcmp(dir->d_name, ".") == 0|| strcmp(dir->d_name, "..") == 0) {
				continue;
			}
			sprintf(file, "%s%s", path, dir->d_name);
			if (remove(file) == -1) {
				perror("Remove failed");

			}
		}
	}

	closedir(d);
}
