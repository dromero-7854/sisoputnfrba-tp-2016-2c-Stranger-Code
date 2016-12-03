/*
 * bibliotecaCharMander.c
 *
 *  Created on: 29/8/2016
 *      Author: utnso
 */


#include "bibliotecaCharMander.h"

#define MAX 1024

int socket_servidor(char* ip, char* puerto, t_log* log)
{
	struct addrinfo addrAux, *res, *p;
	int sockfd;



	memset(&addrAux, 0, sizeof addrAux);
	addrAux.ai_family = AF_UNSPEC;
	addrAux.ai_socktype = SOCK_STREAM;
	//addrAux.ai_flags = AI_PASSIVE;
	int yes = 1;

	if(getaddrinfo(ip, puerto, &addrAux, &res)!= 0)
	{
		log_error(log, "Hubo error en el getaddrinfo");
	}
	//for(p= res; p->ai_next != NULL; p = p->ai_next)
	//{

		/*if((*/sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);//) == -1)
	//	{
	//		continue;
	//	}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
		{
		perror("setsockopt");
		exit(1);
		}

		if(bind(sockfd,res->ai_addr, res->ai_addrlen )== -1)
		{
			close(sockfd);
		//	continue;
		}
	//	break;

	//}

	freeaddrinfo(res);

/*	if(p== NULL)
	{
		log_error(log, "Hubo error al conseguir socket");
		exit(1);
	}*/
	if(listen(sockfd, 15)== -1)
		{
			log_error(log, "Hubo error en el listen");
			exit(1);
		}

	log_trace(log, "Se creo servidor, escuchando en : %d", sockfd);
	return sockfd;
}

int conectar (char* socket_servidor, char* puerto_servidor, t_log* log)
{
	struct addrinfo addrAux, *res, *p;
	int sockfd;


	memset(&addrAux, 0, sizeof addrAux);
	addrAux.ai_family = AF_UNSPEC;
	addrAux.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(socket_servidor, puerto_servidor , &addrAux, &res)!= 0)
		{
			printf("error en getaddrinfo");
		}

	for(p=res; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		{
			continue;
		}

		if(connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
		{
			close(sockfd);
			log_error(log, "Fallo el connect");
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	if(p == NULL)
	{
		log_error(log, "No encuentra servidores a los que conectarse");
		printf("no se pudo conectar a ningun servidor\n");
		exit(1);
	}

	log_trace(log, "Pudo conectarse a un server");
	printf("se conecto!!!!\n");
	return sockfd;
}

int aceptar_conexion(int socket, t_log* log)
{
	struct sockaddr_in aux;
	int tamanio = sizeof(aux);
	int nuevoSocket;

	nuevoSocket = accept(socket, (struct sockaddr *) &aux, &tamanio);

	log_trace(log, "Se conecto alguien. Socket asignado: %d", nuevoSocket);


	return nuevoSocket;

}

void meterStringEnEstructura(char** stringTo, char* stringFrom){
	int cantLetras = strlen(stringFrom);
	*stringTo = malloc(cantLetras + 1);
	strcpy(*stringTo, stringFrom);
}

int comparar_strings(char* string1, char* string2){
	int i, len1, len2;
	len1 = strlen(string1);
	len2 = strlen(string2);
	if(len1 != len2){
		return strcmp(string1,string2);
	}
	for(i = 0; i < len1; i++){
		if(string1[i] == string2[i])continue;
		if(string1[i] < string2[i]){
			return 1;
		} else return 0;
	}
}
