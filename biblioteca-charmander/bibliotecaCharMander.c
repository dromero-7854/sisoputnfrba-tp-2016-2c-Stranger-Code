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



int socket_servidor()
{
	struct addrinfo addrAux, *res, *p;
	int sockfd;


	memset(&addrAux, 0, sizeof addrAux);
	addrAux.ai_family = AF_UNSPEC;
	addrAux.ai_socktype = SOCK_STREAM;
	addrAux.ai_flags = AI_PASSIVE;


	if(getaddrinfo(NULL, "30000", &addrAux, &res)!= 0)
	{
		printf("error en addrAux");
	}
	for(p= res; p->ai_next != NULL; p = p->ai_next)
	{

		if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		{
			continue;
		}

		if(bind(sockfd,res->ai_addr, res->ai_addrlen )== -1)
		{
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
	return sockfd;
}
