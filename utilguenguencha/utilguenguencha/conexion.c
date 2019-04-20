#include "conexion.h"

int socketServer, socketPoolMem;

/* funciones genericas para todos los modulos*/
void conectar_y_crear_hilo(void (*f) (Instruction_set, int),char* ip, int puerto) {
	log_info(LOGGER, "Se inicia servidor");
	listaConexiones = queue_create();
	listaConexionesRetorno = queue_create();
	struct sockaddr_in serverAddress, clientAddress;

	socketServer = iniciar_socket();
	cargar_valores_address(&serverAddress, ip, puerto);
	evitar_bloqueo_puerto(socketServer);
	realizar_bind(socketServer, &serverAddress);
	ponerse_a_escuchar(socketServer, BACKLOG);

	pthread_t hiloHandler;
	pthread_create(&hiloHandler, NULL, (void*) atender_cliente, f);

	while (1) {
		unsigned int tamanoDireccion = sizeof(struct sockaddr_in);
		int socketCliente = accept(socketServer,
				(struct sockaddr *) &clientAddress, &tamanoDireccion);
		if (socketCliente != -1) {
			fcntl(socketCliente, F_SETFL, O_NONBLOCK);
			queue_push(listaConexiones, (int *) socketCliente);
		}
	}
}


char* recibir(char*ip, int puerto){
	struct sockaddr_in serverAddress;
	int socketCliente = iniciar_socket();
	cargar_valores_address(&serverAddress,ip , puerto);
	evitar_bloqueo_puerto(socketServer);
	realizar_bind(socketServer, &serverAddress);
	ponerse_a_escuchar(socketServer, BACKLOG);
	char *buffer = malloc(100);
	int bytesRecibidos = recv(socketCliente, buffer, 99, 0);
	if (bytesRecibidos == 0){
		char * error = string_new();
		string_append(&error, "Cliente se desconecto: ");
		string_append(&error, strerror(errno));
		close(socketCliente);
		log_error(LOGGER, error);
	}
	return buffer;
}

void enviar(Instruction_set instruccion, char * ip, int puerto){
	struct sockaddr_in addressToConect;
	int socketServidor = iniciar_socket();
	cargar_valores_address(&addressToConect, ip, puerto);
	realizar_conexion(socketServidor, &addressToConect);
	/***************************************************************/
	// Hay que arreglar el envio para que envie toda la estructura //
	/***************************************************************/
	if(send(socketServidor, &instruccion, 99, 0) <= 0){
		perror("Error al enviar querys de la consola");
		exit_gracefully(EXIT_FAILURE);
	}
}

/* funcion hilo handler*/
void atender_cliente(void (*f) (Instruction_set, int)) {
	while (1) {
		if (queue_size(listaConexiones) > 0) {
			char * buffer = malloc(100);
			int socketCliente = (int)queue_peek(listaConexiones);
			queue_pop(listaConexiones);
			int bytesRecibidos = recv(socketCliente, buffer, 99, 0);
			if (bytesRecibidos == 0){
				char * error = string_new();
				string_append(&error, "Cliente se desconecto: ");
				string_append(&error, strerror(errno));
				close(socketCliente);
				log_error(LOGGER, error);
			}else if ((errno == EAGAIN || errno == EWOULDBLOCK) && bytesRecibidos == -1) {
				queue_push(listaConexiones, (int *)socketCliente);
			}else{
				/* Vuelvo a mandar el cliente a la lista para seguir recibiendo */
				queue_push(listaConexiones, (int *) socketCliente);

				/*Tenemos que levantar la instruccion y enviarla a f (retornaControl) desde buffer*/

				Instruction_set instruccionRecibida;
				f(instruccionRecibida, socketCliente);
			}
			free(buffer);
		}
	}
}

void atender_respuesta() {
	while (1) {
		char * buffer = malloc(100);
		if (queue_size(listaConexionesRetorno) > 0) {
			int cliente = (int)queue_peek(listaConexionesRetorno);
			queue_pop(listaConexionesRetorno);
			int bytesRecibidos = recv(cliente, buffer, 99, 0);
			if (bytesRecibidos == 0) {
				char * error = string_new();
				string_append(&error, "Cliente se desconecto: ");
				string_append(&error, strerror(errno));
				close(cliente);
				log_error(LOGGER, error);
			} else if ((errno == EAGAIN || errno == EWOULDBLOCK)
					&& bytesRecibidos == -1) {
				queue_push(listaConexiones, (int *) cliente);
			} else {
				log_info(LOGGER, "informacion mandada por el cliente correcta");
			}
		}
	}
}

/* Funciones abtraccion comportamiento */
int iniciar_socket() {
	log_info(LOGGER, "Se crea socket");
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock <= -1) {
		char * error = string_new();
		string_append(&error, "Error al iniciar socket:");
		string_append(&error, strerror(errno));
		close(sock);
		log_error(LOGGER, error);
		exit_gracefully(EXIT_FAILURE);
	}
	return sock;
}

void cargar_valores_address(struct sockaddr_in *fileSystemAddres,char* ip, int port) {
	log_info(LOGGER, "Se carga valores a direccion");
	fileSystemAddres->sin_family = AF_INET;
	fileSystemAddres->sin_addr.s_addr = inet_addr(ip);
	fileSystemAddres->sin_port = htons(port);
	memset(&(fileSystemAddres->sin_zero), '\0', 8);
}

void evitar_bloqueo_puerto(int socket) {
	log_info(LOGGER, "Se evita bloqueo de puerto");
	int activado = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)) < 0){
		perror("Fallo seteo de socket: ");
		log_error(LOGGER,"Fallo seteo de socket");
		exit_gracefully(EXIT_FAILURE);
	};
}

void realizar_bind(int socket, struct sockaddr_in * address) {
	log_info(LOGGER, "Se realiza bind");
	if (bind(socket, (struct sockaddr *) address, sizeof(struct sockaddr)) < 0){
		perror("Fallo el bind: ");
		log_error(LOGGER, "Fallo en bind");
		exit_gracefully(EXIT_FAILURE);
	}
}

void ponerse_a_escuchar(int socket, int cantidadConexiones) {
	log_info(LOGGER, "Estoy escuchado");
	if (listen(socket, cantidadConexiones) < 0) {
		perror("Fallo en el listen: ");
		log_error(LOGGER, "Fallo el listen");
		exit_gracefully(EXIT_FAILURE);
	}
}

void realizar_conexion(int socketServer, struct sockaddr_in * address){
	if(connect(socketServer, (void *) address, sizeof(*address)) < 0){
		perror("Error al realizar connect: ");
		log_error(LOGGER, "Error al realizar connect");
		exit_gracefully(EXIT_FAILURE);
	}
}