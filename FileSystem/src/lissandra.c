#include "lissandra.h"

void *TH_confMonitor(void * p);
void *TH_compactacion(void* p);
void *TH_dump(void* p);
void *TH_server(void * p);
void *TH_compacItem(void* p);
void TH_consola(char* leido);

int main(void) {
	/*
		** Inicializaciones **
	*/
	pthread_t T_consola,T_server,T_confMonitor,T_dump,T_compactacion;
	void *TR_consola,*TR_server,*TR_confMonitor,*TR_dump,*TR_compactacion;

	configure_logger();

	pthread_mutex_init(&memtableMutex,NULL);
	pthread_mutex_init(&tableMetadataMutex,NULL);

	// Cargo la config inicial
	t_config* conf = config_create("config.cfg");
	if(conf == NULL) {
		log_error(LOGGER,"Archivo de configuracion: config.cfg no encontrado");
		pthread_mutex_destroy(&memtableMutex);
		pthread_mutex_destroy(&tableMetadataMutex);
		log_destroy(LOGGER);
		return 1;
	}
	global_conf_load(conf);
	config_destroy(conf);

	// Inicio FS
	if(fs_init()!=0){
		pthread_mutex_destroy(&memtableMutex);
		pthread_mutex_destroy(&tableMetadataMutex);
		log_destroy(LOGGER);
		global_conf_destroy();
		return 1;
	}

	// Memtable & Table Metadata
	initMemtable();
	initTableMetadata();

	/*
		** Creacion de Hilos **
	*/
	// Creo thread que va a ejecutar el monitoreo del archivo de configuracion
	pthread_create(&T_confMonitor,NULL,TH_confMonitor,NULL);
	// Creo thread de compactacion
	pthread_create(&T_compactacion,NULL,TH_compactacion,NULL);
	// Creo thread de dump
	pthread_create(&T_dump,NULL,TH_dump,NULL);
	// Creo thread que va a ejecutar la escucha por consola
	pthread_create(&T_consola,NULL,(void*)leer_por_consola,TH_consola);
	// Creo thread que va a ejecutar la escucha por socket
	pthread_create(&T_server,NULL,TH_server,NULL);

	/*
		** Join de Hilos **
	*/
	// Espero el fin del thread monitor
	pthread_join(T_confMonitor,&TR_confMonitor);
	if((int)TR_confMonitor != 0) {
		log_error(LOGGER,"Error con el thread de monitoreo de configuracion: %d",(int)TR_confMonitor);
	}
	// Espero el fin del thread consola
	pthread_join(T_consola,&TR_consola);
	if((int)TR_consola != 0) {
		log_error(LOGGER,"Error con el thread consola: %d",(int)TR_consola);
	}
	// Espero el fin del thread server
	pthread_join(T_server,&TR_server);
	if((int)TR_server != 0) {
		log_error(LOGGER,"Error con el thread server: %d",(int)TR_server);
	}
	// Espero el fin del thread compactacion
	pthread_join(T_compactacion,&TR_compactacion);
	if((int)TR_compactacion != 0) {
		log_error(LOGGER,"Error con el thread de compactacion: %d",(int)TR_compactacion);
	}
	// Espero el fin del thread dump
	pthread_join(T_dump,&TR_dump);
	if((int)TR_dump != 0) {
		log_error(LOGGER,"Error con el thread de dump: %d",(int)TR_dump);
	}


	/*
		Fin Lissandra
	*/
	destroyTableMetadata();
	destroyMemtable();
	pthread_mutex_destroy(&memtableMutex);
	pthread_mutex_destroy(&tableMetadataMutex);
	log_destroy(LOGGER);
	global_conf_destroy();
	return 0;	
}


/*
	Manejo Consola
*/
void TH_consola(char* leido){
	Instruccion* instruccion_parseada = parser_lql(leido, FILESYSTEM);
	// Validacion de Consultas API para Lissandra
	if(	instruccion_parseada->instruccion != ERROR &&
		instruccion_parseada->instruccion != METRICS &&
		instruccion_parseada->instruccion != ADD &&
		instruccion_parseada->instruccion != RUN &&
		instruccion_parseada->instruccion != JOURNAL){
		
		controller(instruccion_parseada);
	}
	free_consulta(instruccion_parseada);
}

/*
	Manejo Server
*/
void *TH_server(void * p){
	void manejo_instruccion(Instruccion* instruccion, int fd_cliente) {
		controller(instruccion);
	}
	servidor_comunicacion(&manejo_instruccion,global_conf.puerto);
	return (void *)0;
}

/*
	Manejo Monitoreo
*/
void *TH_confMonitor(void * p){

	int confMonitor_cb(void){
		t_config* conf = config_create("config.cfg");
		if(conf == NULL) {
			log_error(LOGGER,"Archivo de configuracion: config.cfg no encontrado");
			return 1;
		}
		global_conf_update(conf);
		log_info(LOGGER,"Se ha actualizado el archivo de configuracion: retardo: %d, tiempo_dump: %d",global_conf.retardo,global_conf.tiempo_dump);
		config_destroy(conf);
		return 0;
	}

	int retMon = monitorNode("config.cfg",IN_MODIFY,&confMonitor_cb);
	if(retMon!=0){
		return (void*)1;
	}
	return (void*)0;
}

/*
	Manejo Dump
*/
void cleanRegistros(void*elem) {
	if(elem == NULL){
		return;
	}
	struct dumpTableList* dump_table_item = (struct dumpTableList*)elem;
	void cleanValue(struct tableRegister* reg){
		free(reg->value);
	}
	list_iterate(dump_table_item->registros,(void*)cleanValue);
	list_destroy(dump_table_item->registros);
}
void *TH_dump(void* p){
	t_list* table_list = list_create();

	void loadDumpStructure(void*elem){
		if(elem == NULL){
			return;
		}
		struct memtableItem* item=(struct memtableItem*)elem;

		/* Defino la funcion de criterio para la busqueda de la tabla */
		bool igualCurrTableName(void * param) {
			struct dumpTableList* dump_table_item = (struct dumpTableList*)param;
			return !strcmp(dump_table_item->tableName,item->tableName);
		}

		/* Busco si ya existe la tabla en la lista de tablas*/
		struct dumpTableList* tableFound = list_find(table_list,&igualCurrTableName);

		if(tableFound == NULL) {  /*Si no la encontre, la creo*/
			/* Reservo memoria para la nueva tabla*/
			struct dumpTableList* newTable = malloc(sizeof(struct dumpTableList));
			/* Cargo el nombre del item que estoy recorriendo en la nueva tabla*/ 
			strcpy(newTable->tableName,item->tableName);
			/* Creo una lista de registros para la nueva tabla */
			newTable->registros = list_create();
			/* Agrego el item que estoy recorriendo a la lista de registros de la tabla */
			struct tableRegister* nuevoRegistro = malloc(sizeof(struct tableRegister));
			*nuevoRegistro=createTableRegister(item->reg.key,item->reg.value,item->reg.timestamp);
			list_add(newTable->registros, (void *)nuevoRegistro);
			/* Agrego la nueva tabla a la lista de tablas */
			list_add(table_list,(void *)newTable);
		} else { /* Si la encontre, cargo el registro */
			struct tableRegister* nuevoRegistro = malloc(sizeof(struct tableRegister));
			*nuevoRegistro=createTableRegister(item->reg.key,item->reg.value,item->reg.timestamp);
			list_add(tableFound->registros, (void *)nuevoRegistro);
		}
	}

	void performDump(void* elem) {
		if(elem == NULL){
			return;
		}
		struct dumpTableList* dump_table_item = (struct dumpTableList*)elem;

		// Obtengo el nombre del archivo
		struct tableMetadataItem* found = get_table_metadata(dump_table_item->tableName);
		if(found == NULL) {
			log_error(LOGGER,"DUMP: No se encontro la metadata de la tabla '%s'",dump_table_item->tableName);
			return;
		}
		char*path=getTablePath(dump_table_item->tableName);

		pthread_rwlock_rdlock(&found->lock);
		int lastDump = getNumLastFile("dump_","tmp",path);
		pthread_rwlock_unlock(&found->lock);

		char*newfilename=malloc(strlen(path)+6+digitos(lastDump)+5);
		sprintf(newfilename,"%s/dump_%d.tmp",path,lastDump+1);

		pthread_rwlock_wrlock(&found->lock);
		if(fs_write(newfilename,dump_table_item->registros)!=0){
			pthread_rwlock_unlock(&found->lock);
			free(path);
			free(newfilename);
			return;
		}
		pthread_rwlock_unlock(&found->lock);
		free(path);
		free(newfilename);
	}
	while(true) {
		usleep(global_conf.tiempo_dump*1000);
		/* Bloqueo Memtable mietras hago el dump*/
		pthread_mutex_lock(&memtableMutex);

		/* Cargo estructura de tablas desde la memtable */
		list_iterate(global_memtable, &loadDumpStructure);

		/* Realizo el dump con los datos en la table_list */
		list_iterate(table_list,&performDump);

		/* Limpio los registros de cada tabla */
		list_iterate(table_list,&cleanRegistros);

		/* Limpio la table_list */
		list_clean(table_list);

		/* Limpio antes de desbloquear*/
		cleanMemtable();

		/* Ya dumpie, desbloqueo*/
		pthread_mutex_unlock(&memtableMutex);

		//log_info(LOGGER,"Dump de Memtable Realizado");
	}
	list_destroy(table_list);
	return (void*)0;
}

/*
	Manejo Compactacion
*/
void *TH_compactacion(void* p){
	loadCurrentTableMetadata();
	t_list* local_table_metadata = list_create();

	void agregoNuevasTablas(void* elem){
		if(elem == NULL){
			return;
		}
		struct tableMetadataItem* item = (struct tableMetadataItem*)elem;

		/*Declaro funcion compare para el list_find */
		bool matchTablename(void* element){
			struct compactionItem* itemNested = (struct compactionItem*)element;
			return !strcmp(itemNested->tableName,item->tableName);
		}
		/* Busco si en la referencia local tengo la tabla */
		struct compactionItem* found = list_find(local_table_metadata,&matchTablename);
		if(found == NULL) { //Si no la tengo, creo el item y levanto el thread
			struct compactionItem *newItem = malloc(sizeof(struct compactionItem));
			strcpy(newItem->tableName,item->tableName);
			newItem->metadata=item->metadata;
			newItem->endFlag=0;
			pthread_create(&newItem->thread,NULL,TH_compacItem,(void*)newItem);
			list_add(local_table_metadata,(void*)newItem);
		}
	}
	void sacoViejasTablas(void* elem){
		if(elem == NULL){
			return;
		}
		struct compactionItem *item = (struct compactionItem *)elem;

		/*Declaro funcion compare para el list_find */
		bool matchTablename(void* element){
			struct tableMetadataItem* itemNested = (struct tableMetadataItem*)element;
			return !strcmp(itemNested->tableName,item->tableName);
		}
		/* Declaro funcion condicion para list_remove_by_condition */
		bool condicionFinHilo(void* element){
			struct compactionItem *itemNested = (struct compactionItem *)element;
			return !strcmp(itemNested->tableName,item->tableName);
		}
		/* Busco si en la referencia global tengo la tabla */
		pthread_mutex_lock(&tableMetadataMutex);
		struct tableMetadataItem* found = list_find(global_table_metadata,&matchTablename);
		pthread_mutex_unlock(&tableMetadataMutex);
		if(found == NULL) { //Si no la tengo, la dropearon, termino el thread correspondiente y elimino el nodo
			item->endFlag=1; // Le aviso que termine
			pthread_join(item->thread,NULL);
			list_remove_by_condition(local_table_metadata,&condicionFinHilo);
		}
	}
	
	while(1) {
		list_iterate(local_table_metadata,&sacoViejasTablas);
		pthread_mutex_lock(&tableMetadataMutex);
		list_iterate(global_table_metadata,&agregoNuevasTablas);
		pthread_mutex_unlock(&tableMetadataMutex);
		usleep(1*1000*1000); //1 seg
	}
	list_destroy(local_table_metadata);
	return (void*)0;
}

void* TH_compacItem(void* p){
	struct compactionItem *item = (struct compactionItem *)p;
	while(item->endFlag == 0){
		usleep(item->metadata.compaction_time*1000);
		if(item->endFlag != 0)
			break;

		struct tableMetadataItem* fnd = get_table_metadata(item->tableName);
		if(fnd == NULL) {
			log_error(LOGGER,"Compaction: No se encontro la metadata de la tabla %s",item->tableName);
			continue;
		}

		t_list* tmpc_files = list_create();
		
		if(compac_search_tmp_files(tmpc_files,item->tableName)!=0) {
			list_destroy(tmpc_files);
			continue;
		}

		// Si no hay archivos temporales
		if(list_size(tmpc_files)==0){
			list_destroy(tmpc_files);
			continue;
		}

		// Obtener registros de los tmpc
		t_list* registrosTemporales = list_create();
		if(compac_get_tmp_registers(registrosTemporales,tmpc_files)!=0){
			clean_registers_list(registrosTemporales);
			list_destroy(registrosTemporales);
			list_destroy(tmpc_files);
			continue;
		}

		// Obtener registros de las particiones
		t_list* registrosParticiones = list_create();
		if(compac_get_partition_registers(registrosParticiones,item->tableName,item->metadata.numero_particiones)!=0) {
			compac_clean_partition_registers(registrosParticiones);
			list_destroy(registrosParticiones);
			clean_registers_list(registrosTemporales);
			list_destroy(registrosTemporales);
			list_destroy(tmpc_files);
			continue;
		}

		// Comparar uno por uno, modificando la estructura en memoria de cada particion
		compac_match_registers(registrosParticiones,registrosTemporales);
		
		long startBlocking = getTimestamp();
		pthread_rwlock_wrlock(&fnd->lock);

		// borrar archivos tmpc
		if(compac_delete_tmpc_files(tmpc_files)!=0) {
			compac_clean_partition_registers(registrosParticiones);
			list_destroy(registrosParticiones);
			clean_registers_list(registrosTemporales);
			list_destroy(registrosTemporales);
			list_destroy(tmpc_files);
			pthread_rwlock_unlock(&fnd->lock);
			long endBlocking = getTimestamp();
			log_info(LOGGER,"Compactacion: Error al borrar los archivos de la tabla %s, se bloqueo por %lu milisegundos",item->tableName,endBlocking-startBlocking);
			continue;
		}

		// guardar las particiones modificadas
		if(compac_save_partition_registers(registrosParticiones,item->tableName)!=0) {
			compac_clean_partition_registers(registrosParticiones);
			list_destroy(registrosParticiones);
			clean_registers_list(registrosTemporales);
			list_destroy(registrosTemporales);
			list_destroy(tmpc_files);
			pthread_rwlock_unlock(&fnd->lock);
			long endBlocking = getTimestamp();
			log_info(LOGGER,"Compactacion: Error al crear las nuevas particiones de la tabla %s, se bloqueo por %lu milisegundos",item->tableName,endBlocking-startBlocking);
			continue;
		}

		pthread_rwlock_unlock(&fnd->lock);
		long endBlocking = getTimestamp();
		log_info(LOGGER,"Se ha compactado la tabla %s, se bloqueo por %lu milisegundos",item->tableName,endBlocking-startBlocking);

		compac_clean_partition_registers(registrosParticiones);
		list_destroy(registrosParticiones);
		clean_registers_list(registrosTemporales);
		list_destroy(registrosTemporales);
		list_destroy(tmpc_files);
	}
	return (void*)0;
}