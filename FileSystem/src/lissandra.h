#ifndef LISSANDRA_H
#define LISSANDRA_H
/*
	UtilGueguencha
*/
#include "../../utilguenguencha/src/comunicacion.h"
#include "../../utilguenguencha/src/parser.h"
#include "../../utilguenguencha/src/utils.h"
/*
	Commons
*/
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/txt.h>
/*
	Unix
*/
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN (1024*(EVENT_SIZE + 16))


/*
	Global Config
*/
struct lfsConfig{
	char*puerto;
	int retardo;
	int max_value_size;
	int tiempo_dump;
	char*directorio_tablas;
	char*directorio_bloques;
	char*directorio_metadata;
};
struct lfsConfig global_conf;
void global_conf_load(t_config* conf);
void global_conf_update(t_config* conf);
void global_conf_destroy(void);


/*
	Table Metadata
*/
struct TableMetadata{
	Consistencias consistencia;
	int numero_particiones;
	int compaction_time;
};

struct tableMetadataItem {
	struct TableMetadata metadata;
	char tableName[100];
	pthread_rwlock_t lock;
};

struct FileSystemMetadata{
	int block_size;
	int blocks;
	int magic_number;
};

/* Tables */
struct tableRegister{
	uint16_t key;
	char* value;
	long timestamp;
};
struct memtableItem{
	struct tableRegister reg;
	char tableName[100];
};
/* Dump Memtable Struct*/
struct dumpTableList{
	char tableName[100];
	t_list* registros;
};

/* Compaction Item Struct*/
struct compactionItem {
	char tableName[100];
	pthread_t thread;
	int endFlag;
	struct TableMetadata metadata;
};
/* Global Variables*/
t_list* global_memtable;
t_list* global_table_metadata;
pthread_mutex_t memtableMutex;
pthread_mutex_t tableMetadataMutex;


/* API */
int controller(Instruccion* instruccion);

/* Memtable */
void initMemtable(void);
void cleanMemtable(void);
void destroyMemtable(void);
void insertInMemtable(struct memtableItem* item);
struct memtableItem* createMemtableItem(char* tableName, struct tableRegister reg);
void findInMemtable(char* tablename, uint16_t key, t_list* registers);

/* Global Table metadata*/
void initTableMetadata(void);
void insertInTableMetadata(char*tableName,struct TableMetadata tMetadata);
void deleteInTableMetadata(char*tableName);
void destroyTableMetadata(void);
void loadCurrentTableMetadata(void);

/* Registros */
long getTimestamp(void);
struct tableRegister createTableRegister(uint16_t key,char* value,long timestamp);

/* Utils */
int tableExists(char * table);
int getNumLastFile(char* prefix,char* extension,char* path);
int monitorNode(char * node,int mode, int(*callback)(void));
struct tableMetadataItem* get_table_metadata(char* tabla);
int deleteTable(char* tabla);
void clean_registers_list(t_list*registers);
int read_temp_files(char* tabla,t_list* listaRegistros);
char *consistencia2string(Consistencias consistencia);
int string2consistencia(char* consistencia);
char* getTablePath(char*tabla);
int digitos(int num);
int digitos_long(long num);

/* Fmanager */
int existeDirectorio(char path[]);
int crearDirectorio(char*source, char*name);
int crearMetadataTableFile(char*directorio,struct TableMetadata tableMeta);
int crearMetadataFile(char*directorio,struct FileSystemMetadata systemMeta);
int crearBinarios(char*directorio,int numero_particiones);
int mkdirRecursivo(char*path);
struct TableMetadata setTableMetadata(Consistencias consistencia, int numero_particiones,double compaction_time);

/* Filesystem */
int fs_init(void);
/*
	fs_read() : implementacion FS de lectura de archivo
		char* filename : nombre de archivo a abrir
		t_list* registros : lista donde va a escribir los registros leidos, items en formato "struct tableRegister"
	return value: 0 en caso de exito, de lo contrario 1
*/
int fs_read(char* filename, t_list* registros);
/*
	fs_write() : implementacion FS de lectura de archivo
		char* filename : nombre de archivo a escribir
		t_list* registros : lista con items en formato "struct tableRegister" a escribir en el archivo
	return value: 0 en caso de exito, de lo contrario 1
*/
int fs_write(char* filename, t_list* registros);
/*
	fs_create() : implementacion FS de creacion de archivo
		char* filename : nombre de archivo a crear
	return value: 0 en caso de exito, de lo contrario 1
*/
int fs_create(char* filename);
/*
	fs_delete() : implementacion FS de borrado de archivo
		char* filename : nombre de archivo a borrar
	return value: 0 en caso de exito, de lo contrario 1
*/
int fs_delete(char* filename);

/* Compactacion */
int compac_search_tmp_files(t_list* tmpc_files, char* tableName);
int compac_get_tmp_registers(t_list*registrosTemporales,t_list*tmpc_files);
int compac_get_partition_registers(t_list*registrosParticiones,char*tableName,int numero_particiones);
void compac_match_registers(t_list*registrosParticiones,t_list*registrosTemporales);
int compac_save_partition_registers(t_list*registrosParticiones,char* tablename);
int compac_delete_tmpc_files(t_list*tmpc_files);
void compac_clean_partition_registers(t_list*registrosParticiones);

#endif /*LISSANDRA_H*/