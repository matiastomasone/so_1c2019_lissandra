#include "config_kernel.h"


void get_parametros_config(){

	t_config* config = config_create("config.cfg");
	if (!config) {
		printf("No encuentro el archivo config\n");
		//MUERO
		exit_gracefully(EXIT_FAILURE);
	}

	PUERTO_ESCUCHA_CONEXION = config_get_int_value(config,"PUERTO_ESCUCHA_CONEXION");
	IP_CONFIG_MIO = malloc(sizeof(char) * 100);
	strcpy(IP_CONFIG_MIO,config_get_string_value(config, "IP_CONFIG_MIO"));

	config_destroy(config);
}

void configure_logger() {

	LOGGER = log_create("kernel.log","tp-lissandra",1,LOG_LEVEL_DEBUG);
	log_info(LOGGER, "Inicia Kernel");
}

void exit_gracefully(int exit_code){
	if(exit_code == EXIT_FAILURE){
		log_error(LOGGER,"Proceso termino en error");
	}
	else{
		log_info(LOGGER,"Proceso termino correctamente");
	}
	log_destroy(LOGGER);
	exit(exit_code);
}
