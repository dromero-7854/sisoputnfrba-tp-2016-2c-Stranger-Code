/*
 * bibliotecaCharMander.c
 *
 *  Created on: 29/8/2016
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "bibliotecaCharMander.h"

#define MAX 1024

int socket_servidor()
{
	struct addrinfo addrAux, *res, *p;
	int sockfd;



	memset(&addrAux, 0, sizeof addrAux);
	addrAux.ai_family = AF_UNSPEC;
	addrAux.ai_socktype = SOCK_STREAM;
	addrAux.ai_flags = AI_PASSIVE;
	int yes = 1;

	if(getaddrinfo(NULL, "6667", &addrAux, &res)!= 0)
	{
		printf("error en getaddrinfo");
	}
	for(p= res; p->ai_next != NULL; p = p->ai_next)
	{

		if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		{
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)
		{
		perror("setsockopt");
		exit(1);
		}

		if(bind(sockfd,res->ai_addr, res->ai_addrlen )== -1)
		{
			close(sockfd);
			continue;
		}
		break;

	}
	if(p== NULL)
	{
		printf("error en el socket");
		exit(1);
	}
	if(listen(sockfd, 15)== -1)
		{
			printf("error en listen");
			exit(1);
		}

	printf("se creo servidor!!!\n");
	return sockfd;
}

void conectar (char* socket_servidor, char* puerto_servidor)
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
			continue;
		}
		break;
	}

	if(p == NULL)
	{
		printf("no se pudo conectar a ningun servidor\n");
		exit(1);
	}

	printf("se conecto!!!!\n");
	char buf[MAX];
	char buf2[MAX];
	int recibido;
	while(1)
	{

		fgets(buf, MAX, stdin);
		if (!strcmp(buf,"exit\n")) exit(1);
		send(sockfd, buf, strlen(buf) + 1, 0);
		/*recibido = recv(sockfd, (void*) buf2, MAX, 0);
		if (recibido == 0) break;
		printf("%s", buf2);*/
	}
}

void aceptar_conexion(int socket)
{
	struct sockaddr_in aux;
	int tamanio = sizeof(aux);
	int nuevoSocket;
	char bufRecibido[MAX];
	char bufEnvio[MAX];
	nuevoSocket = accept(socket, (struct sockaddr *) &aux, &tamanio);
	printf("se conecto alguien!!!\n");

		int recibido;
		while(1)
			{

			/*fgets(bufEnvio, MAX, stdin);
			if (!strcmp(bufEnvio,"exit\n")) exit(1);
			send(nuevoSocket, bufEnvio, strlen(bufEnvio) + 1, 0);*/
			recibido = recv(nuevoSocket, (void*) bufRecibido, MAX, 0);
			if (recibido == 0) break;
			printf("%s", bufRecibido);
			}
	close(socket);
	printf("hubo error\n");

}
