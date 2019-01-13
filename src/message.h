/*
 * message.h
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

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

typedef struct _TelegramMessage {
	char username[128];
	
	char text[8192];
	
	int tries;
	
	struct _TelegramMessage *next;
} TelegramMessage;

void message_init (void);
void message_add (char *username, char *text);
TelegramMessage *block_for_messages (void);
void message_return_to_list (TelegramMessage *list);

#endif /* __MESSAGE_H__ */
