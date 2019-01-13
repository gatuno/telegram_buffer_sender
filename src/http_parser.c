/*
 * http_parser.c
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

#include "http_parser.h"

#undef c_isspace
#define c_isspace(c) \
	({ int __c = (c); \
		(__c == ' ' || __c == '\t' \
		|| __c == '\n' || __c == '\v' || __c == '\f' || __c == '\r'); \
	})

#undef c_isdigit
#define c_isdigit(c) \
	({ int __c = (c); \
		(__c >= '0' && __c <= '9'); \
	})

static int resp_header_locate (const HTTPResponse *resp, const char *name, int start, const char **begptr, const char **endptr) {
	int i;
	const char **headers = resp->headers;
	int name_len;

	if (!headers || !headers[1]) {
		return -1;
	}

	name_len = strlen (name);
	if (start > 0) {
		i = start;
	} else {
		i = 1;
	}

	for (; headers[i + 1]; i++) {
		const char *b = headers[i];
		const char *e = headers[i + 1];
		if (e - b > name_len && b[name_len] == ':' && 0 == strncasecmp (b, name, name_len)) {
			b += name_len + 1;
			while (b < e && c_isspace (*b))
				++b;
			while (b < e && c_isspace (e[-1]))
				--e;
			*begptr = b;
			*endptr = e;
			return i;
		}
	}
	return -1;
}

/* Find and retrieve the header named NAME in the request data.	If
   found, set *BEGPTR to its starting, and *ENDPTR to its ending
   position, and return true. Otherwise return false.

   This function is used as a building block for resp_header_copy
   and resp_header_strdup. */

static int resp_header_get (const HTTPResponse *resp, const char *name, const char **begptr, const char **endptr) {
	int pos = resp_header_locate (resp, name, 0, begptr, endptr);
	return pos != -1;
}

int parser_resp_status (const HTTPResponse *resp, char *message) {
	int status;
	const char *p, *end;

	p = resp->headers[0];
	end = resp->headers[1];

	if (!end) return -1;

	/* "HTTP" */
	if (end - p < 4 || 0 != strncmp (p, "HTTP", 3)) return -1;
	p += 4;

	/* Match the SIP version.	This is optional because Gnutella
	   servers have been reported to not specify HTTP version.	*/
	if (p < end && *p == '/') {
		++p;
		while (p < end && c_isdigit (*p))
			++p;
		if (p < end && *p == '.')
			++p;
		while (p < end && c_isdigit (*p))
			++p;
	}

	while (p < end && c_isspace (*p))
		++p;
	if (end - p < 3 || !c_isdigit (p[0]) || !c_isdigit (p[1]) || !c_isdigit (p[2]))
		return -1;

	status = 100 * (p[0] - '0') + 10 * (p[1] - '0') + (p[2] - '0');
	p += 3;

	if (message) {
		while (p < end && c_isspace (*p))
			++p;
		while (p < end && c_isspace (end[-1]))
			--end;
		
		memcpy (message, p, end - p);
		message [end - p] = '\0';
	}

	return status;
}

int parser_resp_header (const HTTPResponse *resp, const char *name, char *value, size_t len) {
	const char *b, *e;
	if (!resp_header_get (resp, name, &b, &e)) return -1;
	
	if (e - b > len - 1) {
		return -1;
	}
	memcpy (value, b, e - b);
	
	value[e - b] = '\0';
	
	return 0;
}

HTTPResponse *parser_response_new (const char *head) {
	const char *hdr;
	int count;
	
	HTTPResponse *resp = (HTTPResponse *) malloc (sizeof (HTTPResponse));
	
	if (resp == NULL) {
		return NULL;
	}
	
	hdr = head;
	count = 0;
	while (*hdr != 0) {
		if (*hdr == '\n') {
			count++;
		}
		hdr++;
	}
	
	resp->data = head;
	
	resp->headers = (const char **) malloc (sizeof (const char *) * count + 2);
	
	if (resp->headers == NULL) {
		free (resp);
		return NULL;
	}
	
	count = 0;
	hdr = head;
	while (1) {
		resp->headers[count++] = hdr;
		
		if (!hdr[0] || (hdr[0] == '\r' && hdr[1] == '\n') || hdr[0] == '\n') {
			break;
		}
		
		do {
			const char *end = strchr (hdr, '\n');
			if (end) {
				hdr = end + 1;
			} else {
				hdr += strlen (hdr);
			}
		} while (*hdr == ' ' || *hdr == '\t');
	}
	
	resp->headers[count] = NULL;
	
	return resp;
}

void parser_resp_free (HTTPResponse *resp) {
	free (resp->headers);
	
	free (resp);
}

