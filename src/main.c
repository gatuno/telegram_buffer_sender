/*
 * main.c
 * This file is part of Telegram Buffer Sender
 *
 * Copyright (C) 2019 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

#include <sys/epoll.h>

#include "main.h"
#include "socket_interface.h"
#include "message.h"
#include "https.h"
#include "network_parser.h"

void *message_loop (void *);

pthread_t thread_message;

int setnonblock (int fd) {
	int flags;
	
	flags = fcntl (fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl (fd, F_SETFL, flags);
}

void main_loop (int epoll_fd);

int main (int argc, char *argv[]) {
	int epoll_fd;
	struct epoll_event event;
	int res;
	int accept_socket;
	PollingInfo *poller_accept;
	pthread_attr_t thread_attr;
	
	/* Crear el mutex que proteja los mensajes en cola */
	message_init ();
	
	https_init ();
	
	accept_socket = interface_setup_socket ();
	
	if (accept_socket < 0) {
		return EXIT_FAILURE;
	}
	
	setnonblock (accept_socket);
	
	epoll_fd = epoll_create1(0);
	
	if (epoll_fd < 0) {
		fprintf (stderr, "Failed to create epoll descriptor\n");
		
		interface_close (accept_socket);
		return EXIT_FAILURE;
	}
	
	poller_accept = malloc (sizeof (PollingInfo));
	if (poller_accept == NULL) {
		fprintf (stderr, "Failed to reserve memory\n");
		
		interface_close (accept_socket);
		close (epoll_fd);
		
		return EXIT_FAILURE;
	}
	
	poller_accept->type = TYPE_LISTEN_SOCKET;
	poller_accept->fd = accept_socket;
	
	event.events = EPOLLIN | EPOLLONESHOT | EPOLLET;
	event.data.ptr = poller_accept;
	
	res = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, accept_socket, &event);
	
	if (res < 0) {
		fprintf (stderr, "Failed to add socket to watch\n");
		
		interface_close (accept_socket);
		close (epoll_fd);
		
		return EXIT_FAILURE;
	}
	
	/* Lanzar los threads */
	pthread_attr_init (&thread_attr);
	pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE);
	
	res = pthread_create (&thread_message, &thread_attr, message_loop, NULL);  
	if (res) {
		printf ("ERROR; return code from pthread_create() is %d\n", res);
		return EXIT_FAILURE;
	}
	
	main_loop (epoll_fd);
	
	return 0;
}

#define MAX_EVENTS 50

void main_loop (int epoll_fd) {
	struct epoll_event events[MAX_EVENTS];
	struct epoll_event new_event;
	int res_epoll;
	int g;
	int res;
	char buffer[8192];
	PollingInfo *poller, *poller_cliente;
	
	do {
		res_epoll = epoll_wait (epoll_fd, events, MAX_EVENTS, -1);
		
		if (res_epoll < 0) {
			if (errno == EINTR) {
				/* Señal, manejar */
			} else {
				break;
			}
		} else if (res_epoll == 0) {
			continue;
		}
		
		for (g = 0; g < res_epoll; g++) {
			poller = events[g].data.ptr;
			
			if (poller->type == TYPE_LISTEN_SOCKET) {
				/* Es el socket donde se reciben las conexiones entrantes, aceptar a todos */
				while (poller_cliente = interface_accept_client (poller->fd), poller_cliente != NULL) {
					setnonblock (poller_cliente->fd);
					
					new_event.events = EPOLLIN | EPOLLONESHOT | EPOLLET | EPOLLPRI ;
					new_event.data.ptr = poller_cliente;
					
					res = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, poller_cliente->fd, &new_event);
					
					/* TODO: revisar errores del epoll aquí */
				}
				
				/* Rearmar el socket para el epoll */
				events[g].events = EPOLLIN | EPOLLONESHOT | EPOLLET;
				
				res = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, poller->fd, &events[g]);
			} else if (poller->type == TYPE_CLIENT) {
				do {
					res = read (poller->fd, buffer, sizeof (buffer));
					
					if (res == EAGAIN || res == EWOULDBLOCK) {
						break;
					}
					
					if (res == 0) {
						/* Conexión cerrada */
						res = epoll_ctl (epoll_fd, EPOLL_CTL_DEL, poller->fd, NULL);
						
						close (poller->fd);
						
						free (poller);
						poller = NULL;
						break;
					}
					
					printf ("Data del cliente [%i]\n", poller->fd);
					
					/* Procesar data aquí */
					parse_network_message (buffer, res);
				} while (1);
				
				if (poller != NULL) {
					/* Rearmar el socket para el epoll */
					events[g].events = EPOLLIN | EPOLLONESHOT | EPOLLET;
				
					res = epoll_ctl (epoll_fd, EPOLL_CTL_MOD, poller->fd, &events[g]);
				}
			}
		}
	} while (1);
}

void *message_loop (void *param) {
	/* Instalar el manejador de señales del thread */
	TelegramMessage *lista, *next, *remain;
	int res;
	
	printf ("Thread de los mensajes\n");
	while (1) {
		lista = block_for_messages ();
		
		printf ("Tengo mensajes que procesar\n");
		remain = NULL;
		while (lista != NULL) {
			printf ("Tratando de enviar un mensaje\n");
			res = https_send_message (lista->username, lista->text);
			
			next = lista->next;
			
			if (res == 0) {
				/* Mensaje enviado, se puede ir el mensaje */
				free (lista);
				printf ("Correctamente enviado\n");
			} else {
				lista->tries++;
				
				if (lista->tries == 3) {
					/* No reintentar mas de 3 veces */
					free (lista);
					printf ("Agotó los intentos\n");
				} else {
					lista->next = remain;
					remain = lista;
					printf ("se reintentará otra vez\n");
				}
			}
			lista = next;
		}
		
		/* Enviar los mensajes restantes de regreso a la cola de envio */
		if (remain != NULL) {
			message_return_to_list (remain);
		}
		
		/* TODO: Agregar aquí el sleep */
	}
}
