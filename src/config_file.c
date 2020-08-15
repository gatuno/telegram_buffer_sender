/*
 * config.c
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

#include <getopt.h>

#include "config_file.h"
#include "ini.h"

/* Las configuraciones globales */
#define DEFAULT_SOCKET_PATH "/var/run/telegrambf.socket"
#define DEFAULT_PID_PATH "/var/run/telegrambf.pid"

static char *config_file = NULL;
static char *config_pid_file = NULL;
static char *config_socket_path = NULL;
static char *config_bot_key = NULL;

static const struct option long_options[] = {
	//{ "help",     0, NULL, "h" },
	{ "config",		1, NULL, 'c' },
	{ "pid-file",	1, NULL, 'p' },
	//{ "verbose",  0, NULL, 'v' },
	{ NULL,       0, NULL, 0   }
};

static void parse_args (int argc, char *argv[]) {
	int next_option;
	
	do {
		next_option = getopt_long (argc, argv, "c:p:", long_options, NULL);
		
		switch (next_option) {
			case 'c':
				config_file = optarg;
				break;
			case 'p':
				config_pid_file = optarg;
				break;
			case -1:
				break;
		}
	} while (next_option != -1);
}

static int config_ini_parser_function (void* data, const char* section, const char* name, const char* value) {
	if (strcmp (section, "daemon") == 0 && strcmp (name, "socket") == 0) {
		config_socket_path = strdup (value);
	} else if (strcmp (section, "daemon") == 0 && strcmp (name, "bot-key") == 0) {
		config_bot_key = strdup (value);
	} else {
		return 0;
	}
	
	return -1;
}

char *config_get_string (int config) {
	switch (config) {
		case CONFIG_PID_FILE:
			return config_pid_file;
			break;
		case CONFIG_SOCKET_PATH:
			return config_socket_path;
			break;
		case CONFIG_BOT_KEY:
			return config_bot_key;
			break;
	}
	
	return NULL;
}

void config_init (int argc, char *argv[]) {
	/* Inicializar los valores por defecto */
	config_pid_file = DEFAULT_PID_PATH;
	
	config_socket_path = DEFAULT_SOCKET_PATH;
	
	/* Parsear los argumentos de la linea de comandos */
	parse_args (argc, argv);
	
	/* Tratar de cargar el fichero de configuración */
	if (config_file != NULL) {
		if (ini_parse (config_file, config_ini_parser_function, NULL) < 0) {
			fprintf (stderr, "Can't load config file %s\n", config_file);
			
			exit (1);
		}
	}
}
