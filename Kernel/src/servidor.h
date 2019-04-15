#ifndef SERVIDORH
#define SERVIDORH

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include "config_kernel.h"

#define BACKLOG 10     // Cuántas conexiones pendientes se mantienen en cola

#define PUERTO_KERNELL 8080
#define PUERTO_POOL_MEM 8081
#define PUERTO_FS 8082
#define IP "127.0.0.1"

/* id = 1 responder */
/* id = 2 recibir */

typedef struct{
    int id;
    int value;
}  __attribute__((packed)) TypeOfCommunication;

typedef struct{
    int socketServidor;
    struct sockaddr_in cliente;
    unsigned int tamanoDireccion;
}  __attribute__((packed)) DatosCliente;


/*funciones que DEBERIAN utilizar los procesos*/
void levantar_servidor(void (*f) (char*));
void enviarMensaje(char*);

/*funciones hilo handler*/
void atender_cliente(void (*f) (char*));


/*funciones abstraccion comportamiento*/
int iniciar_socket();
void cargar_valores_address(struct sockaddr_in *fileSystemAddres, char* ip, int port);
void evitar_bloqueo_puerto(int socket);
void realizar_bind(int socket, struct sockaddr_in *fileSystemAddres);
void ponerse_a_escuchar(int socket, int cantidadConexiones);
void retornarControl();



#endif
