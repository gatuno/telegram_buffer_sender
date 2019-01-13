/*
 * http_parser.h
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

#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

typedef struct {
	const char *data;

	/* The array of pointers that indicate where each header starts.
	   For example, given this HTTP response:

	   HTTP/1.0 200 Ok
	   Description: some
	    text
	   Etag: x

	   The headers are located like this:

	   "HTTP/1.0 200 Ok\r\nDescription: some\r\n text\r\nEtag: x\r\n\r\n"
	   ^                   ^                             ^          ^
	   headers[0]          headers[1]                    headers[2] headers[3]

	   I.e. headers[0] points to the beginning of the request,
	   headers[1] points to the end of the first header and the
	   beginning of the second one, etc.  */

	const char **headers;
} HTTPResponse;

int parser_resp_status (const HTTPResponse *resp, char *message);
int parser_resp_header (const HTTPResponse *resp, const char *name, char *value, size_t len);
HTTPResponse *parser_response_new (const char *head);
void parser_resp_free (HTTPResponse *resp);

#endif /* __HTTP_PARSER_H__ */

