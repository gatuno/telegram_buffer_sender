/*
 * socket_interface.c
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

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <sys/stat.h>

#include "socket_interface.h"
#include "main.h"

#define SOCKET_PATH "/tmp/telegrambf.socket"

int interface_setup_socket (void) {
	int sock;
	struct sockaddr_un socket_name;
	
	sock = socket (AF_UNIX, SOCK_SEQPACKET, 0);
	
	if (sock < 0) {
		perror ("Failed to create AF_UNIX SOCK_SEQPACKET socket");
		
		return -1;
	}
	
	memset (&socket_name, 0, sizeof (struct sockaddr_un));
	
	socket_name.sun_family = AF_UNIX;
	strncpy (socket_name.sun_path, SOCKET_PATH, sizeof (socket_name.sun_path) - 1);
	
	unlink (SOCKET_PATH);
	
	if (bind (sock, (struct sockaddr *) &socket_name, sizeof (struct sockaddr_un)) < 0) {
		perror ("Bind");
		
		return -1;
	}
	
	if (listen (sock, 16) < 0) {
		perror ("Listen");
		
		return -1;
	}
	
	/* TODO: Aplicar permisos aquí */
	chmod (SOCKET_PATH, 0666);
	
	return sock;
}

void interface_close (int sock) {
	close (sock);
	
	/* Remove the path, if still exists */
	unlink (SOCKET_PATH);
}

PollingInfo * interface_accept_client (int sock) {
	int cliente;
	PollingInfo *poller_cliente;
	
	cliente = accept (sock, NULL, NULL);
	
	if (cliente < 0) {
		return NULL;
	}
	
	poller_cliente = malloc (sizeof (PollingInfo));
	
	if (poller_cliente == NULL) {
		close (cliente);
		
		return NULL;
	}
	
	poller_cliente->type = TYPE_CLIENT;
	poller_cliente->fd = cliente;
	
	return poller_cliente;
}

