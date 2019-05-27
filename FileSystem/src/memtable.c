#include "lissandra.h"
struct memtableItem* createMemtableItem(char* tableName, struct tableRegister reg) {
	struct memtableItem item;
	strcpy(item.tableName,tableName);
	item.reg = reg;
	struct memtableItem * ret = malloc(sizeof(struct memtableItem));
	*ret = item;
	return ret;
}
void initMemtable(void){
	global_memtable = list_create();
}
void insertInMemtable(struct memtableItem* item) {
	list_add(global_memtable, (void *)item);
}
void cleanMemtable(void){
	void freeValues(void* elem) {
		struct memtableItem* item = (struct memtableItem*)elem;
		free(item->reg.value);
	}
	list_iterate(global_memtable, &freeValues);
	list_clean(global_memtable);
}
void destroyMemtable(void){
	cleanMemtable();
	list_destroy(global_memtable);
}
void findInMemtable(char* tablename, uint16_t key, t_list* registers){
	void matchReg(struct memtableItem * item){
		if (strcmp(item->tableName,tablename)==0 && item->reg.key == key) {
			struct tableRegister* newreg = malloc(sizeof(struct tableRegister));
			*newreg = createTableRegister(item->reg.key,item->reg.value,item->reg.timestamp);
			list_add(registers,(void*)newreg);
		}
	}
	pthread_mutex_lock(&memtableMutex);
	list_iterate(global_memtable,(void*)matchReg);
	pthread_mutex_unlock(&memtableMutex);
}

/* Global Table metadata*/
void initTableMetadata(void){
	global_table_metadata = list_create();
}
void insertInTableMetadata(char*tableName,struct TableMetadata tMetadata) {
	struct tableMetadataItem* item = malloc(sizeof(struct tableMetadataItem));
	item->metadata=tMetadata;
	strcpy(item->tableName,tableName);
	pthread_rwlock_init(&item->lock,NULL);
	list_add(global_table_metadata, (void *)item);
}
void deleteInTableMetadata(char*tableName) {
	bool condition(struct tableMetadataItem* item){
		return !strcmp(tableName,item->tableName);
	}
	struct tableMetadataItem* found = list_find(global_table_metadata,(void*)condition);
	if (found != NULL) {
		pthread_rwlock_destroy(&found->lock);
	}
	struct tableMetadataItem* removido = list_remove_by_condition(global_table_metadata,(void*)condition);
	free(removido);
}
void destroyTableMetadata(void){
	void destroyMutex(struct tableMetadataItem * item){
		pthread_rwlock_destroy(&item->lock);
	}
	list_iterate(global_table_metadata,(void*)destroyMutex);
	list_destroy(global_table_metadata);
}
void loadCurrentTableMetadata(void){
	struct dirent *dir;
	DIR *d = opendir(global_conf.directorio_tablas);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0){
				char*filename=malloc(strlen(global_conf.directorio_tablas)+strlen(dir->d_name)+1+12+1);
				sprintf(filename,"%s%s/Metadata.txt",global_conf.directorio_tablas,dir->d_name);
				t_config* conf = config_create(filename);
				struct tableMetadataItem *item = malloc(sizeof(struct tableMetadataItem));
				pthread_rwlock_init(&item->lock,NULL);
				strcpy(item->tableName,dir->d_name);
				item->metadata.consistencia=string2consistencia(config_get_string_value(conf,"CONSISTENCY"));
				item->metadata.numero_particiones=config_get_int_value(conf,"PARTITIONS");
				item->metadata.compaction_time=config_get_int_value(conf,"COMPACTION_TIME");
				config_destroy(conf);
				free(filename);
				pthread_mutex_lock(&tableMetadataMutex);
				list_add(global_table_metadata,(void *)item);
				pthread_mutex_unlock(&tableMetadataMutex);
			}
		}
		closedir(d);
	}
}