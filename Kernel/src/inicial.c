#include "kernel.h"

void configuracion_inicial(void) {
	t_config* CONFIG = config_create("config.cfg");
	if (!CONFIG) {
		log_error(LOGGER, "No encuentro el archivo config");
		exit_gracefully(EXIT_FAILURE);
	}

	LOGGER_METRICS = log_create("logger_metrics.log", "log_metrics", 0, LOG_LEVEL_INFO);

	PUERTO_DE_ESCUCHA = config_get_string_value(CONFIG,"PUERTO_DE_ESCUCHA");
	IP_MEMORIA_PPAL = config_get_string_value(CONFIG,"IP_MEMORIA_PPAL");
	PUERTO_MEMORIA_PPAL = config_get_string_value(CONFIG,"PUERTO_MEMORIA_PPAL");

	QUANTUM = config_get_int_value(CONFIG, "QUANTUM");
	TAMANO_MAXIMO_LECTURA_ARCHIVO = config_get_int_value(CONFIG,
			"TAMANO_MAXIMO_LECTURA_ARCHIVO");
	HILOS_KERNEL = config_get_int_value(CONFIG,
			"HILOS_KERNEL");

	SEGUNDOS_METRICS = config_get_int_value(CONFIG, "TIEMPO_METRICS");
	PREGUNTAR_POR_MEMORIAS = config_get_int_value(CONFIG, "TIEMPO_PREGUNTAR_MEMORIA");
	TIEMPO_DESCRIBE = config_get_int_value(CONFIG, "TIEMPO_DESCRIBE");
}

void actualizar_configuracion(t_config* conf) {
	SEGUNDOS_METRICS = config_get_int_value(conf, "TIEMPO_METRICS");
	PREGUNTAR_POR_MEMORIAS = config_get_int_value(conf, "TIEMPO_PREGUNTAR_MEMORIA");
	TIEMPO_DESCRIBE = config_get_int_value(conf, "TIEMPO_DESCRIBE");
}

void iniciarEstados() {
	estadoReady = list_create();
	estadoNew = list_create();
	estadoExit = list_create();
	list_clean(estadoReady);
	list_clean(estadoNew);
	list_clean(estadoExit);
}

void iniciarEstructurasAsociadas(){
	memorias = list_create();
	t_list * listaEC = list_create();
	t_list * listaSC = list_create();
	t_list * listaSHC = list_create();
	t_list * listaDISP = list_create();
	list_add_in_index(memorias, EC, listaEC);
	list_add_in_index(memorias, SC, listaSC);
	list_add_in_index(memorias, SHC, listaSHC);
	list_add_in_index(memorias, DISP, listaDISP);
	metrics = dictionary_create();

}
