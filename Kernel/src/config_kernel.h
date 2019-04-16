#ifndef CONFIG_KERNEL_H_
#define CONFIG_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <math.h>
#include <time.h>
#include "parser.h"

t_log * LOGGER;
int PUERTO_ESCUCHA_CONEXION;
char* IP_CONFIG_MIO;

//Cargo los parametros desde el archivo config y los libero conforme deje de usarlos
void get_parametros_config();
void configure_logger();
//libera todos los parametros que tenga
void free_parametros_config();
void exit_gracefully(int exit_code);
char* leer_consola();

#endif
