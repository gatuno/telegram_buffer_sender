/*
 * network_parser.c
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

#include "message.h"

void parse_network_message (char *buffer, int size) {
	char type;
	int endings[4];
	int g;
	int count;
	
	if (size < 1) {
		/* Mensaje anormal */
		return;
	}
	
	type = buffer[0];
	
	count = 0;
	for (g = 1; g < size && count < 4; g++) {
		if (buffer[g] == 0) {
			endings[count] = g;
			count++;
		}
	}
	
	if (type == 16) {
		if (count < 2) {
			/* No tiene suficientes campos */
			return;
		}
		
		if (endings[0] == 1 || endings[1] == endings[0] + 1) {
			/* Campos vacios */
			return;
		}
		
		/* Mandar el mensaje a la cola de mensajes por enviar */
		message_add (&buffer[1], &buffer[endings[0] + 1]);
	}
}
