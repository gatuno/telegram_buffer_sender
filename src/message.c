/*
 * message.c
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

#include <pthread.h>

#include "message.h"

static TelegramMessage *lista_inicio = NULL, *lista_final = NULL;

static pthread_mutex_t message_list_mutex;
static pthread_cond_t message_list_cond;

void message_init (void) {
	pthread_mutex_init (&message_list_mutex, NULL);
	pthread_cond_init (&message_list_cond, NULL);
}

void message_add (char *username, char *text) {
	TelegramMessage *new;
	
	new = (TelegramMessage *) malloc (sizeof (TelegramMessage));
	
	if (new == NULL) {
		printf ("Warning: No se pudo reservar memoria para un mensaje\n");
		
		return;
	}
	
	new->next = NULL;
	new->tries = 0;
	
	strncpy (new->username, username, sizeof (new->username));
	new->username[sizeof (new->username) - 1] = 0;
	
	strncpy (new->text, text, sizeof (new->text));
	new->text[sizeof (new->text) - 1] = 0;
	
	pthread_mutex_lock (&message_list_mutex);
	/* Ligar */
	if (lista_final == NULL) {
		/* Primero de la lista */
		lista_final = lista_inicio = new;
	} else {
		lista_final->next = new;
		lista_final = new;
	}
	
	pthread_cond_signal (&message_list_cond);
	pthread_mutex_unlock (&message_list_mutex);
}

void message_return_to_list (TelegramMessage *list) {
	TelegramMessage *last;
	
	if (list == NULL) return;
	
	last = list;
	
	while (last->next != NULL) last = last->next;
	
	pthread_mutex_lock (&message_list_mutex);
	
	if (lista_final == NULL) {
		lista_inicio = list;
		lista_final = last;
	} else {
		lista_final->next = list;
		lista_final = last;
	}
	
	pthread_mutex_unlock (&message_list_mutex);
}

TelegramMessage *block_for_messages (void) {
	TelegramMessage *list;
	
	pthread_mutex_lock (&message_list_mutex);
	while (lista_inicio == NULL) {
		pthread_cond_wait (&message_list_cond, &message_list_mutex);
	}
	
	/* Tomar la lista y ponerla a NULL */
	list = lista_inicio;
	lista_inicio = lista_final = NULL;
	
	pthread_mutex_unlock (&message_list_mutex);
	
	return list;
}

