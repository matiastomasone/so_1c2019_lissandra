#include "kernel.h"

int main(void) {
	configure_logger();
	configuracion_inicial();
	iniciarEstados();

	pthread_t consolaKernel;
	pthread_create(&consolaKernel, NULL, (void*) leer_por_consola,
			retorno_consola);
	for (;;) {
	} // Para que no muera
}

void configuracion_inicial(void) {
	t_config* CONFIG;
	CONFIG = config_create("config.cfg");
	if (!CONFIG) {
		log_error(LOGGER, "No encuentro el archivo config");
		exit_gracefully(EXIT_FAILURE);
	}
	PUERTO_DE_ESCUCHA = config_get_int_value(CONFIG, "PUERTO_DE_ESCUCHA");
	IP_MEMORIA_PPAL = config_get_string_value(CONFIG, "IP_MEMORIA_PPAL");
	PUERTO_MEMORIA_PPAL = config_get_int_value(CONFIG, "PUERTO_MEMORIA_PPAL");
	QUANTUM = config_get_int_value(CONFIG, "QUANTUM");
	config_destroy(CONFIG);
}

void retorno_consola(char* leido) {
	log_info(LOGGER, "Kernel. Se retorno a consola");
	log_info(LOGGER, leido);

	Proceso * proceso = malloc(sizeof(Proceso));
	proceso->instrucciones = queue_create();

	Instruccion instruccion_parseada = parser_lql(leido, KERNEL);

	if (instruccion_parseada.instruccion == RUN) {
			FILE* f;
			Run * run = instruccion_parseada.instruccion_a_realizar;
			f = fopen(run->path, "r");

			 char *line = string_new();

			if (f != NULL) {
				while(fgets(line, 1000, f) != NULL){
					if(string_length(line) > 0 && line[string_length(line)-1] == '\n'){
						line[string_length(line)-1] = '\0';
					}
					Instruccion instruc = parser_lql(line, KERNEL);
					queue_push(proceso->instrucciones, &instruc);
				}
			} else {
				log_error(LOGGER, "Kernel: Error al abrir el archivo");
				exit_gracefully(1);
			}
			free(line);

		} else {
			queue_push(proceso->instrucciones, &instruccion_parseada);
		}


//	switch(instruccion_parseada.instruccion){
//		case SELECT:;
//			Select * select = instruccion_parseada.instruccion_a_realizar;
//			printf("Tabla: %s Key: %i TS: %lu \n",select->nombre_tabla, select->key, select->timestamp);
//			break;
//		case INSERT:;
//			Insert * insert = instruccion_parseada.instruccion_a_realizar;
//			printf("Tabla: %s Key: %i Valor: %s TSins: %lu TS: %lu \n",insert->nombre_tabla,insert->key, insert->value, insert->timestamp_insert, insert->timestamp);
//			break;
//		case CREATE:;
//			Create * create = instruccion_parseada.instruccion_a_realizar;
//			printf("Tabla: %s Particiones: %i Compactacion: %lu Consistencia: %i TS: %lu \n",create->nombre_tabla,create->particiones, create->compactation_time, create->consistencia, create->timestamp);
//			break;
//		case DESCRIBE:;
//			Describe * describe = instruccion_parseada.instruccion_a_realizar;
//			printf("Tabla: %s TS: %lu\n",describe->nombre_tabla, describe->timestamp);
//			break;
//		case ADD:;
//			Add * add = instruccion_parseada.instruccion_a_realizar;
//			printf("Memoria: %i Consistencia: %i TS: %lu\n",add->memoria, add->consistencia, add->timestamp);
//			break;
//		case RUN:;
//			Run * run = instruccion_parseada.instruccion_a_realizar;
//			printf("Path: %s TS: %lu\n",run->path, run->timestamp);
//			break;
//		case DROP:;
//			Drop * drop = instruccion_parseada.instruccion_a_realizar;
//			printf("Tabla: %s TS: %lu\n",drop->nombre_tabla, drop->timestamp);
//			break;
//		case JOURNAL:;
//			Journal * journal = instruccion_parseada.instruccion_a_realizar;
//			printf("TS: %lu \n",journal->timestamp);
//			break;
//		case METRICS:;
//			Metrics * metrics = instruccion_parseada.instruccion_a_realizar;
//			printf("TS: %lu \n",metrics->timestamp);
//			break;
//		case ERROR:
//			printf("ERROR DE CONSULTA \n");
//	}

//	Proceso *proceso = crear_proceso(leido);
	printf("Llegue jaja");
}

void iniciarEstados() {
	log_info(LOGGER, "Kernel:Se inician estados");
	estadoReady = queue_create();
	estadoNew = queue_create();
	estadoExit = queue_create();
	estadoExec = queue_create();
	queue_clean(estadoReady);
	queue_clean(estadoNew);
	queue_clean(estadoExit);
	queue_clean(estadoExec);
}

void planificar_programas() {

}

Proceso * crear_proceso(char * leido) {

	Instruccion instruccion_parseada = parser_lql(leido, KERNEL);
	Proceso * proceso = malloc(sizeof(Proceso));
	proceso->instrucciones = queue_create();

	if (instruccion_parseada.instruccion == RUN) {
		FILE* f;
		Run * run = instruccion_parseada.instruccion_a_realizar;
		f = fopen("r", run->path);
		char * line = string_new();
		line = malloc(1024);

		if (f != NULL) {
			while(fgets(line, sizeof(line), f) != NULL){
				Instruccion instruc = parser_lql(line, KERNEL);
				queue_push(proceso->instrucciones, &instruc);
			}
		} else {
			log_error(LOGGER, "Kernel: Error al abrir el archivo");
			exit_gracefully(1);
		}
		free(line);

	} else {
		queue_push(proceso->instrucciones, &instruccion_parseada);
	}
	return proceso;
}



