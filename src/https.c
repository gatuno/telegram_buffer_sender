/*
 * https.c
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

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "https.h"
#include "uri_encode.h"
#include "http_parser.h"

#include "jsmn.h"

#define MIN(i, j) ((i) <= (j) ? (i) : (j))

static SSL_CTX *ssl_context;

static const char * bot_key_api = "";

void https_init (void) {
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	
	ssl_context = SSL_CTX_new (TLS_client_method ());
}

int https_connect_tcp (const char *hostname, int port) {
	char buffer_port[32];
	
	int sock, res;
	
	struct addrinfo hints, *results, *res_save;
	
	memset (&hints, 0, sizeof (hints));
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	sprintf (buffer_port, "%i", port);
	
	res = getaddrinfo (hostname, buffer_port, &hints, &results);
	
	if (res != 0) {
		return -1;
	}
	
	res_save = results;
	
	sock = -1;
	
	while (results != NULL) {
		sock = socket (results->ai_family, results->ai_socktype, 0);
		
		res = connect (sock, results->ai_addr, results->ai_addrlen);
		
		if (res == 0) {
			break;
		}
		
		close (sock);
		
		sock = -1;
		
		results = results->ai_next;
	}
	
	freeaddrinfo (res_save);
	
	return sock;
}

size_t https_build_post_send_message (const char *user, const char *text, char *buffer, size_t size) {
	const char *body_template = "chat_id=%s&text=%s";
	char *encoded_user;
	char *encoded_text;
	size_t total;
	
	encoded_user = malloc (sizeof (char) * strlen (user) * 3 + 2);
	if (encoded_user == NULL) {
		return 0;
	}
	encoded_text = malloc (sizeof (char) * strlen (text) * 3 + 2);
	if (encoded_text == NULL) {
		free (encoded_user);
		return 0;
	}
	
	uri_encode (user, strlen (user), encoded_user);
	uri_encode (text, strlen (text), encoded_text);
	
	total = strlen (body_template) + strlen (encoded_user) + strlen (encoded_text) + 1;
	
	if (size < total) {
		free (encoded_user);
		free (encoded_text);
		
		return 0;
	}
	
	total = sprintf (buffer, body_template, encoded_user, encoded_text);
	
	free (encoded_user);
	free (encoded_text);
	
	return total;
}

size_t https_build_http_header_send_message (const char *url, const char *host, size_t body_size, char *buffer, size_t size) {
	const char *http_template =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Connection: Close\r\n"
		"User-Agent: Telegram Buffer Sender 0.1\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: %d\r\n"
		"\r\n";
	
	size_t total = strlen (http_template) + strlen (url) + strlen (host) + 8 + 1; /* 8 worst case for body_size */
	
	if (size < total) {
		return 0;
	}
	
	return sprintf (buffer, http_template, url, host, body_size);
}

static const char *response_head_terminator (const char *start, const char *peeked, int peeklen) {
	const char *p, *end;

	/* If at first peek, verify whether HUNK starts with "HTTP".  If
	not, this is a HTTP/0.9 request and we must bail out without
	reading anything.  */
	if (start == peeked && 0 != memcmp (start, "HTTP", MIN (peeklen, 4)))
		return start;

	/* Look for "\n[\r]\n", and return the following position if found.
	Start two chars before the current to cover the possibility that
	part of the terminator (e.g. "\n\r") arrived in the previous
	batch.  */
	p = peeked - start < 2 ? start : peeked - 2;
	end = peeked + peeklen;

	/* Check for \n\r\n or \n\n anywhere in [p, end-2). */
	for (; p < end - 2; p++) {
		if (*p == '\n') {
			if (p[1] == '\r' && p[2] == '\n') {
				return p + 3;
			} else if (p[1] == '\n') {
				return p + 2;
			}
		}
	}
	
	/* p==end-2: check for \n\n directly preceding END. */
	if (p[0] == '\n' && p[1] == '\n')
		return p + 2;

	return NULL;
}

char * https_get_http_headers_response (SSL *ssl, char *hunk, size_t bufsize) {
	size_t pklen, rdlen, remain;
	const char *end;
	int tail = 0;
	int res;
	
	/* Leer hasta que por lo menos tengamos respuesta */
	while (1) {
		pklen = SSL_peek (ssl, hunk + tail, bufsize - 1 - tail);
		if (pklen <= 0) {
			res = SSL_get_error (ssl, pklen);
			
			if (res != SSL_ERROR_ZERO_RETURN) {
				/* We don't have answer */
				return NULL;
			}
		}
		
		end = response_head_terminator (hunk, hunk + tail, pklen);
		
		if (end) {
			/* The data contains the terminator: we'll drain the data up
			to the end of the terminator.  */
			remain = end - (hunk + tail);
			//assert (remain >= 0);
			if (remain == 0) {
				/* No more data needs to be read. */
				hunk[tail] = '\0';
				return hunk;
			}
			if (bufsize - 1 < tail + remain) {
				/* Buffer insuficiente */
				return NULL;
				/*bufsize = tail + remain + 1;
				hunk = xrealloc (hunk, bufsize);*/
			}
		} else {
			remain = pklen;
		}
	
		/* Now, read the data.  Note that we make no assumptions about
		     how much data we'll get.  (Some TCP stacks are notorious for
		     read returning less data than the previous MSG_PEEK.)  */
		rdlen = SSL_read (ssl, hunk + tail, remain);
		if (rdlen <= 0) {
			res = SSL_get_error (ssl, rdlen);
			
			if (res != SSL_ERROR_ZERO_RETURN) {
				return NULL;
			}
		}
	
		tail += rdlen;
		hunk[tail] = '\0';

		if (rdlen <= 0 && res == SSL_ERROR_ZERO_RETURN) {
			if (tail == 0) {
				/* EOF without anything having been read */
				return NULL;
			} else {
				/* EOF seen: return the data we've read. */
				return hunk;
			}
		}
		if (end && rdlen == remain) {
			/* The terminator was seen and the remaining data drained --
			 we got what we came for.  */
			return hunk;
		}

		/* Keep looping until all the data arrives. */

		if (tail == bufsize - 1) {
			/* No hay suficientes bytes para continuar las lecturas */
			return NULL;
		}
	}
}

int https_send_message (const char *user, const char *text) {
	int sock;
	int res;
	char buffer_url[128], buffer_val[128];
	char buffer_header[8192], buffer_body[8192];
	size_t len_body, len_header, chunk_size;
	char *p;
	HTTPResponse *response;
	SSL *ssl;
	int status;
	long int parsed_i, content_len;
	
	sock = https_connect_tcp ("api.telegram.org", 443);
	
	if (sock < 0) {
		return -1;
	}
	
	ssl = SSL_new (ssl_context);
	
	if (ssl == NULL) goto sd_close;
	
	SSL_set_fd (ssl, sock);
	
	res = SSL_connect (ssl);
	if (res != 1) goto sd_ssl_free;
	
	/* Construir el cuerpo del mensaje primero */
	len_body = https_build_post_send_message (user, text, buffer_body, sizeof (buffer_body));
	
	if (len_body == 0) goto sd_ssl_free; /* Falló */
	
	sprintf (buffer_url, "/bot%s/sendMessage", bot_key_api);
	
	len_header = https_build_http_header_send_message (buffer_url, "api.telegram.org", len_body, buffer_header, sizeof (buffer_header));
	
	/* No pude armar la petición */
	if (len_header == 0) goto sd_ssl_free;
	
	res = SSL_write (ssl, buffer_header, len_header);
	
	if (res <= 0) goto sd_shutdown;
	
	res = SSL_write (ssl, buffer_body, len_body);
	
	if (res <= 0) goto sd_shutdown;
	
	p = https_get_http_headers_response (ssl, buffer_header, sizeof (buffer_header));
	
	/* No tenemos header */
	if (p == NULL) goto sd_shutdown;
	
	response = parser_response_new (buffer_header);
	
	if (response == NULL) goto sd_shutdown;
	
	status = parser_resp_status (response, NULL);
	
	if (status == -1) {
		printf ("Malformed status line\n");
		
		goto sd_shutdown;
	}
	
	printf ("Status: %i\n", status);
	
	content_len = -1;
	if (parser_resp_header (response, "Content-Length", buffer_val, sizeof (buffer_val)) == 0) {
		errno = 0;
		parsed_i = strtol (buffer_val, NULL, 0);
		
		if (parsed_i == LONG_MAX && errno == ERANGE) {
			content_len = -1;
		} else if (parsed_i < 0) {
			content_len = -1;
		} else {
			content_len = parsed_i;
		}
	}
	
	if (content_len == -1 || content_len > sizeof (buffer_body)) {
		/* Leer máximo hasta el tamaño del buffer */
		content_len = sizeof (buffer_body);
	}
	
	len_body = 0;
	while (content_len > 0) {
		chunk_size = SSL_read (ssl, buffer_body + len_body, content_len);
		
		if (chunk_size <= 0) {
			res = SSL_get_error (ssl, chunk_size);
		
			if (res != SSL_ERROR_ZERO_RETURN) {
				break;
			}
		}
		
		content_len -= len_body;
		len_body += chunk_size;
	}
	
	if (len_body > 0) {
		printf ("Body: %s\n", buffer_body);
	}
	
	SSL_shutdown (ssl);
	SSL_free (ssl);
	close (sock);
	
	return parse_check (status, buffer_body, len_body);
	
sd_shutdown:
	SSL_shutdown (ssl);
sd_ssl_free:
	SSL_free (ssl);
sd_close:
	close (sock);
	
	return -1;
}

int parse_check (int status, char *body, int body_len) {
	int r;
	int g;
	jsmn_parser p;
	jsmntok_t t[128];
	char ok = 'n';
	char value[128];
	char error_desc[512];
	int error_code;
	
	error_desc[0] = 0;
	value[0] = 0;
	jsmn_init(&p);
	
	r = jsmn_parse(&p, body, body_len, t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return -1;
	}
	
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return -1;
	}
	
	for (g = 1; g < r; g++) {
		if (jsoneq (body, &t[g], "ok") == 0) {
			if (t[g + 1].type != JSMN_PRIMITIVE) {
				printf ("ok is expected to be a primitive\n");
				
				return -1;
			}
			
			ok = body[t[g + 1].start];
			
			if (ok != 't' && ok != 'f') {
				printf ("ok is expected to be a boolean\n");
				
				return -1;
			}
			
			if (ok == 't') {
				return 0;
			}
		} else if (jsoneq (body, &t[g], "error_code") == 0 && t[g + 1].type == JSMN_PRIMITIVE) {
			memcpy (value, body + t[g + 1].start, t[g + 1].end - t[g + 1].start);
			
			value [t[g + 1].end - t[g + 1].start] = '\0';
			
			error_code = 0;
			sscanf (value, "%d", &error_code);
		} else if (jsoneq (body, &t[g], "description") == 0) {
			if (t[g + 1].end - t[g + 1].start >= sizeof (error_desc)) {
				strncpy (error_desc, "Error description too long!", sizeof (value));
			} else {
				memcpy (error_desc, body + t[g + 1].start, t[g + 1].end - t[g + 1].start);
			
				error_desc [t[g + 1].end - t[g + 1].start] = '\0';
			}
		} else {
			if (t[g + 1].type == JSMN_ARRAY || t[g + 1].type == JSMN_OBJECT) {
				g = g + t[g + 1].size;
			}
		}
	}
	
	if (ok == 'f') {
		if (error_code == 403) {
			/* No pude enviar el mensaje, no reintentar */
			printf ("I couldn't send the message, error %d: %s\n", error_code, error_desc);
			
			return 0;
		}
		
		printf ("I couldn't send the message, error %d: %s\n", error_code, error_desc);
		
		return 0;
	}
	
}

