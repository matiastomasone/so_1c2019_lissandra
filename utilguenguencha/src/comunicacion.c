#include "comunicacion.h"

void servidor_comunicacion(Comunicacion *comunicacion){
	fd_set fd_set_master, fd_set_temporal;
	int aux1, bytes_recibidos, fd_max, server_socket;
	server_socket = iniciar_servidor(comunicacion->puerto_servidor);
	FD_ZERO(&fd_set_master);
	FD_ZERO(&fd_set_temporal);
	FD_SET(server_socket, &fd_set_master);
	fd_max = server_socket;
	for (;;) {
		fd_set_temporal = fd_set_master;
		if (select(fd_max + 1, &fd_set_temporal, NULL, NULL, NULL) == -1) {
			exit_gracefully(EXIT_FAILURE);
		}
		int fin = fd_max;
		for (aux1 = 0; aux1 <= fin; aux1++) {
			if (FD_ISSET(aux1, &fd_set_temporal)) {
				if (aux1 == server_socket) {
					struct sockaddr_in client_address;
					size_t tamanio_client_address = sizeof(client_address);
					int socket_cliente = accept(server_socket,
							(struct sockaddr *) &client_address,
							&tamanio_client_address);
					if (socket_cliente < 0) {
						exit_gracefully(EXIT_FAILURE);
					}
					if(!set_timeout(socket_cliente, 1)){
						liberar_conexion(socket_cliente);
						continue;
					}
					FD_SET(socket_cliente, &fd_set_master);
					if (socket_cliente > fd_max) {
						fd_max = socket_cliente;
					}
				} else {
					Tipo_Comunicacion tipo_comu;
					if ((bytes_recibidos = recv(aux1, &tipo_comu, sizeof(Tipo_Comunicacion), MSG_DONTWAIT)) <= 0) {
						liberar_conexion(aux1);
						FD_CLR(aux1, &fd_set_master);
					} else {
						Instruction_set inst_op;
						Instruccion *instruccion = malloc(sizeof(Instruccion));
						if(tipo_comu == comunicacion->tipo_comunicacion){
							Procesos proceso_que_envia;
							if ((bytes_recibidos = recv(aux1, &proceso_que_envia, sizeof(Procesos), MSG_DONTWAIT)) <= 0) {
								liberar_conexion(aux1);
								FD_CLR(aux1, &fd_set_master);
							}else if(validar_sender(proceso_que_envia, comunicacion->proceso, comunicacion->tipo_comunicacion)){
								if ((bytes_recibidos = recv(aux1, &inst_op,sizeof(Instruction_set), MSG_DONTWAIT))<= 0) {
									liberar_conexion(aux1);
									FD_CLR(aux1, &fd_set_master);
								}
								if (recibir_buffer(aux1, inst_op, instruccion, comunicacion->tipo_comunicacion)) {
									FD_CLR(aux1, &fd_set_master);
									retornarControl(instruccion, aux1);
								} else {
									FD_CLR(aux1, &fd_set_master);
									liberar_conexion(aux1);
								}
							}else{
								liberar_conexion(aux1);
								FD_CLR(aux1, &fd_set_master);
							}
						}else{
							liberar_conexion(aux1);
							FD_CLR(aux1, &fd_set_master);
						}
					}
				}
			}
		}
	}
}


int iniciar_servidor(char* puerto_servidor) {
	int socket_servidor;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int activado = 1;

	getaddrinfo("127.0.0.1", puerto_servidor, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((socket_servidor = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1)
			continue;
		if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado,
				sizeof(activado)) < 0) {
			close(socket_servidor);
			continue;
		}
		if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
			close(socket_servidor);
			continue;
		}
		break;
	}

	if (listen(socket_servidor, BACKLOG) < 0) {
		exit_gracefully(EXIT_FAILURE);
	}

	freeaddrinfo(servinfo);

	return socket_servidor;
}

bool recibir_buffer(int aux1, Instruction_set inst_op, Instruccion *instruccion, Tipo_Comunicacion tipo_comu) {
	size_t buffer_size;
	void* stream;
	int bytes_recibidos;
	switch (inst_op) {
	case SELECT:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Select *select;
			select = desempaquetar_select(stream);
			instruccion->instruccion = SELECT;
			instruccion->instruccion_a_realizar = select;
			free(stream);
			return true;
		}else{
			return false;
		}
	case INSERT:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Insert *insert;
			insert = desempaquetar_insert(stream);
			instruccion->instruccion = INSERT;
			instruccion->instruccion_a_realizar = insert;
			free(stream);
			return true;
		}else{
			return false;
		}
	case CREATE:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Create *create;
			create = desempaquetar_create(stream);
			instruccion->instruccion = CREATE;
			instruccion->instruccion_a_realizar = create;
			free(stream);
			return true;
		}else{
			return false;
		}
	case DESCRIBE:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Describe *describe;
			describe = desempaquetar_describe(stream);
			instruccion->instruccion = DESCRIBE;
			instruccion->instruccion_a_realizar = describe;
			free(stream);
			return true;
		}else{
			return false;
		}
	case DROP:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Drop *drop;
			drop = desempaquetar_drop(stream);
			instruccion->instruccion = DROP;
			instruccion->instruccion_a_realizar = drop;
			free(stream);
			return true;
		}else{
			return false;
		}
	case JOURNAL:
		if(tipo_comu == T_INSTRUCCION){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT))<= 0) {
				free(stream);
				return false;
			}
			Journal *journal;
			journal = desempaquetar_journal(stream);
			instruccion->instruccion = JOURNAL;
			instruccion->instruccion_a_realizar = journal;
			free(stream);
			return true;
		}else{
			return false;
		}
	case GOSSIP:
		if(tipo_comu == T_GOSSIPING){
			if ((bytes_recibidos = recv(aux1, &buffer_size, sizeof(size_t), MSG_DONTWAIT)) <= 0) {
				return false;
			}
			stream = malloc(buffer_size);
			if ((bytes_recibidos = recv(aux1, stream, buffer_size, MSG_DONTWAIT)) <= 0) {
				free(stream);
				return false;
			}
			Gossip *gossip;
			gossip = desempaquetar_gossip(stream);
			instruccion->instruccion = GOSSIP;
			instruccion->instruccion_a_realizar = gossip;
			free(stream);
			return true;
		}else{
			return false;
		}
	case MAX_VALUE:
		if(tipo_comu == T_VALUE){
			instruccion->instruccion = MAX_VALUE;
			return true;
		}else{
			return false;
		}
	default:
		return false;
	}
}

Instruccion *enviar_instruccion(char* ip, char* puerto, Instruccion *instruccion, Procesos proceso_del_que_envio, Tipo_Comunicacion tipo_comu) {
	int server_fd = crear_conexion(ip, puerto);
	if (server_fd == -1) {
		log_error(LOGGER, "No se puede establecer comunicacion con destino");
		return respuesta_error(CONNECTION_ERROR);
	} else {
		if(!set_timeout(server_fd, 1)){
			liberar_conexion(server_fd);
			return respuesta_error(CONNECTION_ERROR);
		}
		t_paquete * paquete = crear_paquete(tipo_comu, proceso_del_que_envio, instruccion);
		if (enviar_paquete(paquete, server_fd)) {
			eliminar_paquete(paquete);
			return recibir_respuesta(server_fd);
		}else{
			liberar_conexion(server_fd);
			log_error(LOGGER, "No se pudo enviar la instruccion");
			return respuesta_error(CONNECTION_ERROR);
		}
	}
}

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
		return -1;
	}
	freeaddrinfo(server_info);

	return socket_cliente;
}

bool enviar_paquete(t_paquete* paquete, int socket_cliente) {
	int bytes = paquete->buffer->size + 4 * sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);
	if ((send(socket_cliente, a_enviar, bytes, 0)) < 0) {
		return false;
	}
	free(a_enviar);
	return true;
}

bool enviar_paquete_retorno(t_paquete_retorno* paquete, int socket_cliente) {
	int bytes = paquete->buffer->size + 3 * sizeof(int);
	void* a_enviar = serializar_paquete_retorno(paquete, bytes);
	if ((send(socket_cliente, a_enviar, bytes, 0)) < 0) {
		return false;
	}
	free(a_enviar);
	return true;
}

void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}

void *serializar_paquete_retorno(t_paquete_retorno *paquete, int bytes){
	void *magic = malloc(bytes);
	int desplazamiento = 0;
	memcpy(magic + desplazamiento, &paquete->header, sizeof(paquete->header));
	desplazamiento += sizeof(paquete->header);
	memcpy(magic + desplazamiento, &paquete->retorno, sizeof(paquete->retorno));
	desplazamiento += sizeof(paquete->retorno);
	if(paquete->buffer->size > 0){
		memcpy(magic + desplazamiento, &paquete->buffer->size, sizeof(paquete->buffer->size));
		desplazamiento += sizeof(paquete->buffer->size);
		memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
		desplazamiento += paquete->buffer->size;
	}
	return magic;
}

void *serializar_paquete(t_paquete* paquete, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;
	memcpy(magic + desplazamiento, &(paquete->comunicacion), sizeof(paquete->comunicacion));
	desplazamiento += sizeof(paquete->comunicacion);
	memcpy(magic + desplazamiento, &(paquete->source), sizeof(paquete->source));
	desplazamiento += sizeof(paquete->source);
	memcpy(magic + desplazamiento, &(paquete->header), sizeof(paquete->header));
	desplazamiento += sizeof(paquete->header);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(paquete->buffer->size));
	desplazamiento += sizeof(paquete->buffer->size);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;
	return magic;
}

void crear_buffer(t_paquete* paquete) {
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void crear_buffer_retorno(t_paquete_retorno* paquete) {
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

t_paquete* crear_paquete(Tipo_Comunicacion tipo_comu, Procesos proceso_del_que_envio,
		Instruccion* instruccion) {
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->comunicacion = tipo_comu;
	paquete->source = proceso_del_que_envio;
	paquete->header = instruccion->instruccion;
	crear_buffer(paquete);
	switch (instruccion->instruccion) {
	case SELECT:
		empaquetar_select(paquete,
				(Select*) instruccion->instruccion_a_realizar);
		break;
	case INSERT:
		empaquetar_insert(paquete,
				(Insert*) instruccion->instruccion_a_realizar);
		break;
	case CREATE:
		empaquetar_create(paquete,
				(Create*) instruccion->instruccion_a_realizar);
		break;
	case DESCRIBE:
		empaquetar_describe(paquete,
				(Describe*) instruccion->instruccion_a_realizar);
		break;
	case DROP:
		empaquetar_drop(paquete, (Drop*) instruccion->instruccion_a_realizar);
		break;
	case JOURNAL:
		empaquetar_journal(paquete,
				(Journal*) instruccion->instruccion_a_realizar);
		break;
	case GOSSIP:
		empaquetar_gossip(paquete,
				(Gossip*) instruccion->instruccion_a_realizar);
		break;
	default:
		free(paquete->buffer);
		free(paquete);
		return (t_paquete*) NULL;
		break;
	}
	return paquete;
}

t_paquete_retorno *crear_paquete_retorno(Instruccion *instruccion){
	t_paquete_retorno *paquete = malloc(sizeof(t_paquete_retorno));
	paquete->header = instruccion->instruccion;
	crear_buffer_retorno(paquete);
	switch (instruccion->instruccion) {
		case RETORNO:{
			paquete->retorno = RETORNO;
			Retorno_Generico *retorno = instruccion->instruccion_a_realizar;
			switch(retorno->tipo_retorno){
				case VALOR:
					empaquetar_retorno_valor(paquete, (Retorno_Value*) retorno->retorno);
					break;
				case DATOS_DESCRIBE:
					empaquetar_retorno_describe(paquete, (t_list *)retorno->retorno);
					break;
				case TAMANIO_VALOR_MAXIMO:
					empaquetar_retorno_max_val(paquete, (Retorno_Max_Value *)retorno->retorno);
					break;
				case SUCCESS:
					empaquetar_retorno_success(paquete);
					break;
			}
			break;
		}
		case ERROR:{
			paquete->retorno = ERROR;
			Error *error = instruccion->instruccion_a_realizar;
			empaquetar_retorno_error(paquete, error);
			break;
		}
		default:
			break;
	}
	return paquete;
}

void empaquetar_select(t_paquete *paquete, Select *select) {
	size_t tamanio_nombre_tabla = (strlen(select->nombre_tabla) + 1);
	paquete->buffer->stream = malloc(
			sizeof(select->key) + sizeof(size_t) + tamanio_nombre_tabla
					+ sizeof(select->timestamp));
	memcpy(paquete->buffer->stream, &select->key, sizeof(select->key));
	paquete->buffer->size += sizeof(select->key);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&tamanio_nombre_tabla, sizeof(tamanio_nombre_tabla));
	paquete->buffer->size += sizeof(tamanio_nombre_tabla);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			select->nombre_tabla, tamanio_nombre_tabla);
	paquete->buffer->size += tamanio_nombre_tabla;
	memcpy(paquete->buffer->stream + paquete->buffer->size, &select->timestamp,
			sizeof(select->timestamp));
	paquete->buffer->size += sizeof(select->timestamp);
}

void empaquetar_insert(t_paquete *paquete, Insert *insert) {
	size_t tamanio_nombre_tabla = (strlen(insert->nombre_tabla) + 1);
	size_t tamanio_value = (strlen(insert->value) + 1);
	paquete->buffer->stream = malloc(
			sizeof(insert->key) + sizeof(size_t) + tamanio_nombre_tabla
					+ sizeof(insert->timestamp)+ sizeof(insert->timestamp_insert) + sizeof(size_t) + tamanio_value);
	memcpy(paquete->buffer->stream, &insert->key, sizeof(insert->key));
	paquete->buffer->size += sizeof(insert->key);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&tamanio_nombre_tabla, sizeof(tamanio_nombre_tabla));
	paquete->buffer->size += sizeof(tamanio_nombre_tabla);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			insert->nombre_tabla, tamanio_nombre_tabla);
	paquete->buffer->size += tamanio_nombre_tabla;
	memcpy(paquete->buffer->stream + paquete->buffer->size, &insert->timestamp,
			sizeof(insert->timestamp));
	paquete->buffer->size += sizeof(insert->timestamp);
	memcpy(paquete->buffer->stream + paquete->buffer->size, &insert->timestamp_insert,
			sizeof(insert->timestamp_insert));
	paquete->buffer->size += sizeof(insert->timestamp_insert);
	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_value,
			sizeof(tamanio_value));
	paquete->buffer->size += sizeof(tamanio_value);
	memcpy(paquete->buffer->stream + paquete->buffer->size, insert->value,
			tamanio_value);
	paquete->buffer->size += tamanio_value;
}

void empaquetar_create(t_paquete * paquete, Create *create) {
	size_t tamanio_nombre_tabla = (strlen(create->nombre_tabla) + 1);
	paquete->buffer->stream = malloc(
			sizeof(create->compactation_time) + sizeof(create->consistencia)
					+ sizeof(size_t) + tamanio_nombre_tabla + sizeof(create->particiones)
					+ sizeof(create->timestamp));
	memcpy(paquete->buffer->stream, &create->compactation_time,
			sizeof(create->compactation_time));
	paquete->buffer->size += sizeof(create->compactation_time);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&create->consistencia, sizeof(create->consistencia));
	paquete->buffer->size += sizeof(create->consistencia);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&tamanio_nombre_tabla, sizeof(tamanio_nombre_tabla));
	paquete->buffer->size += sizeof(tamanio_nombre_tabla);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			create->nombre_tabla, tamanio_nombre_tabla);
	paquete->buffer->size += tamanio_nombre_tabla;
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&create->particiones, sizeof(create->particiones));
	paquete->buffer->size += sizeof(create->particiones);
	memcpy(paquete->buffer->stream + paquete->buffer->size, &create->timestamp,
			sizeof(create->timestamp));
	paquete->buffer->size += sizeof(create->timestamp);
}

void empaquetar_describe(t_paquete * paquete, Describe *describe) {
	size_t tamanio_nombre_tabla = (strlen(describe->nombre_tabla) + 1);
	paquete->buffer->stream = malloc( sizeof(size_t) +
			tamanio_nombre_tabla + sizeof(describe->timestamp));
	memcpy(paquete->buffer->stream, &tamanio_nombre_tabla,
			sizeof(tamanio_nombre_tabla));
	paquete->buffer->size += sizeof(tamanio_nombre_tabla);
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			describe->nombre_tabla, tamanio_nombre_tabla);
	paquete->buffer->size += tamanio_nombre_tabla;
	memcpy(paquete->buffer->stream + paquete->buffer->size,
			&describe->timestamp, sizeof(describe->timestamp));
	paquete->buffer->size += sizeof(describe->timestamp);
}

void empaquetar_drop(t_paquete * paquete, Drop * drop) {
	size_t tamanio_nombre_tabla = (strlen(drop->nombre_tabla) + 1);
	paquete->buffer->stream = malloc( sizeof(size_t) +
			tamanio_nombre_tabla + sizeof(drop->timestamp));
	memcpy(paquete->buffer->stream, &tamanio_nombre_tabla,
			sizeof(tamanio_nombre_tabla));
	paquete->buffer->size += sizeof(tamanio_nombre_tabla);
	memcpy(paquete->buffer->stream + paquete->buffer->size, drop->nombre_tabla,
			tamanio_nombre_tabla);
	paquete->buffer->size += tamanio_nombre_tabla;
	memcpy(paquete->buffer->stream + paquete->buffer->size, &drop->timestamp,
			sizeof(drop->timestamp));
	paquete->buffer->size += sizeof(drop->timestamp);
}

void empaquetar_journal(t_paquete * paquete, Journal * journal) {
	paquete->buffer->stream = malloc(sizeof(journal->timestamp));
	memcpy(paquete->buffer->stream, &journal->timestamp,
			sizeof(journal->timestamp));
	paquete->buffer->size += sizeof(journal->timestamp);
}

void empaquetar_gossip(t_paquete * paquete, Gossip * gossip) {
	int cantidad_memorias = list_size(gossip->lista_memorias);
	paquete->buffer->stream = malloc(sizeof(int));
	memcpy(paquete->buffer->stream, &cantidad_memorias, sizeof(int));
	paquete->buffer->size += sizeof(int);
	while(cantidad_memorias > 0){
		Memoria *memoria = malloc(sizeof(Memoria));
		memoria = list_get(gossip->lista_memorias, cantidad_memorias - 1);
		size_t tamanio_ip = (strlen(memoria->ip) + 1 );
		size_t tamanio_puerto = (strlen(memoria->puerto) + 1 );
		size_t tamanio_id = sizeof(memoria->idMemoria);
		size_t tamanio = tamanio_ip + sizeof(size_t) + tamanio_puerto + sizeof(size_t) + tamanio_id;
		paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);
		memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_ip, sizeof(size_t));
		paquete->buffer->size += sizeof(size_t);
		memcpy(paquete->buffer->stream + paquete->buffer->size, memoria->ip, (strlen(memoria->ip)+1));
		paquete->buffer->size += strlen(memoria->ip) + 1;
		memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_puerto, sizeof(size_t));
		paquete->buffer->size += sizeof(size_t);
		memcpy(paquete->buffer->stream + paquete->buffer->size, memoria->puerto, (strlen(memoria->puerto)+1));
		paquete->buffer->size += strlen(memoria->puerto) + 1;
		memcpy(paquete->buffer->stream + paquete->buffer->size, &memoria->idMemoria, sizeof(memoria->idMemoria));
		paquete->buffer->size += sizeof(memoria->idMemoria);
		cantidad_memorias--;
		free(memoria);
	}
}

Select *desempaquetar_select(void* stream) {
	int desplazamiento = 0;
	Select *select = malloc(sizeof(Select));
	int tamanio;
	memcpy(&select->key, stream, sizeof(select->key));
	desplazamiento += sizeof(select->key);
	memcpy(&tamanio, stream + desplazamiento, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	select->nombre_tabla = malloc(tamanio);
	memcpy(select->nombre_tabla, stream + desplazamiento, tamanio);
	desplazamiento += tamanio;
	memcpy(&select->timestamp, stream + desplazamiento,
			sizeof(select->timestamp));
	return select;
}

Insert *desempaquetar_insert(void* stream) {
	int desplazamiento = 0;
	Insert *insert = malloc(sizeof(Insert));
	size_t tamanio;
	memcpy(&insert->key, stream, sizeof(insert->key));
	desplazamiento += sizeof(insert->key);
	memcpy(&tamanio, stream + desplazamiento, sizeof(tamanio));
	desplazamiento += sizeof(tamanio);
	insert->nombre_tabla = malloc(tamanio);
	memcpy(insert->nombre_tabla, stream + desplazamiento, tamanio);
	desplazamiento += tamanio;
	memcpy(&insert->timestamp, stream + desplazamiento,
			sizeof(insert->timestamp));
	desplazamiento += sizeof(insert->timestamp);
	memcpy(&insert->timestamp_insert, stream + desplazamiento,
			sizeof(insert->timestamp_insert));
	desplazamiento += sizeof(insert->timestamp_insert);
	memcpy(&tamanio, stream + desplazamiento, sizeof(tamanio));
	desplazamiento += sizeof(tamanio);
	insert->value = malloc(tamanio);
	memcpy(insert->value, stream + desplazamiento, tamanio);
	return insert;
}
Create *desempaquetar_create(void* stream) {
	int desplazamiento = 0;
	Create *create = malloc(sizeof(Create));
	size_t tamanio;
	memcpy(&create->compactation_time, stream,
			sizeof(create->compactation_time));
	desplazamiento += sizeof(create->compactation_time);
	memcpy(&create->consistencia, stream + desplazamiento,
			sizeof(create->consistencia));
	desplazamiento += sizeof(create->consistencia);
	memcpy(&tamanio, stream + desplazamiento, sizeof(tamanio));
	desplazamiento += sizeof(tamanio);
	create->nombre_tabla = malloc(tamanio);
	memcpy(create->nombre_tabla, stream + desplazamiento, tamanio);
	desplazamiento += tamanio;
	memcpy(&create->particiones, stream + desplazamiento,
			sizeof(create->particiones));
	desplazamiento += sizeof(create->particiones);
	memcpy(&create->timestamp, stream + desplazamiento,
			sizeof(create->timestamp));
	return create;
}
Describe *desempaquetar_describe(void* stream) {
	int desplazamiento = 0;
	Describe *describe = malloc(sizeof(Describe));
	size_t tamanio;
	memcpy(&tamanio, stream, sizeof(tamanio));
	desplazamiento += sizeof(tamanio);
	describe->nombre_tabla = malloc(tamanio);
	memcpy(describe->nombre_tabla, stream + desplazamiento, tamanio);
	desplazamiento += tamanio;
	memcpy(&describe->timestamp, stream + desplazamiento,
			sizeof(describe->timestamp));
	return describe;
}

Drop *desempaquetar_drop(void* stream) {
	int desplazamiento = 0;
	Drop *drop = malloc(sizeof(Drop));
	size_t tamanio;
	memcpy(&tamanio, stream, sizeof(tamanio));
	desplazamiento += sizeof(tamanio);
	drop->nombre_tabla = malloc(tamanio);
	memcpy(drop->nombre_tabla, stream + desplazamiento, tamanio);
	desplazamiento += tamanio;
	memcpy(&drop->timestamp, stream + desplazamiento, sizeof(drop->timestamp));
	return drop;
}
Journal *desempaquetar_journal(void* stream) {
	Journal *journal = malloc(sizeof(Journal));
	memcpy(&journal->timestamp, stream, sizeof(journal->timestamp));
	return journal;
}

Gossip *desempaquetar_gossip(void* stream){
	int desplazamiento = 0;
	Gossip *gossip = malloc(sizeof(Gossip));
	gossip->lista_memorias = list_create();
	size_t cantidad_memorias, tamanio;
	memcpy(&cantidad_memorias, stream, sizeof(cantidad_memorias));
	desplazamiento += sizeof(cantidad_memorias);
	while(cantidad_memorias != 0){
		Memoria *memoria = malloc(sizeof(Memoria));
		memcpy(&tamanio, stream + desplazamiento, sizeof(tamanio));
		desplazamiento += sizeof(tamanio);
		memoria->ip = malloc(tamanio);
		memcpy(memoria->ip, stream + desplazamiento, tamanio);
		desplazamiento += tamanio;
		memcpy(&tamanio, stream + desplazamiento, sizeof(tamanio));
		desplazamiento += sizeof(tamanio);
		memoria->puerto = malloc(tamanio);
		memcpy(memoria->puerto, stream + desplazamiento, tamanio);
		desplazamiento += tamanio;
		memcpy(&memoria->idMemoria, stream + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		list_add(gossip->lista_memorias, memoria);
		free(memoria->ip);
		free(memoria->puerto);
		free(memoria);
		cantidad_memorias--;
	}
	return gossip;
}

bool validar_sender(Procesos sender, Procesos receiver, Tipo_Comunicacion comunicacion){
	switch (comunicacion){
	case T_INSTRUCCION:
		switch (receiver){
		case KERNEL:
			return false;
			break;
		case POOLMEMORY:
			return sender == KERNEL;
			break;
		case FILESYSTEM:
			return sender == POOLMEMORY;
			break;
		default:
			return false;
			break;
		}
		break;
	case T_GOSSIPING:
		return receiver == POOLMEMORY && (sender == KERNEL || sender == POOLMEMORY);
		break;
	case T_VALUE:
		return receiver == FILESYSTEM && sender == POOLMEMORY;
		break;
	default:
		return false;
		break;
	}
}

Instruccion *responder(int fd_a_responder, Instruccion *instruccion){
	if(instruccion->instruccion == RETORNO || instruccion->instruccion == ERROR){
		t_paquete_retorno *paquete = crear_paquete_retorno(instruccion);
		if(enviar_paquete_retorno(paquete, fd_a_responder)){
			liberar_conexion(fd_a_responder);
			return respuesta_success();
		}else{
			liberar_conexion(fd_a_responder);
			log_error(LOGGER, "No se pudo enviar la respuesta");
			return respuesta_error(CONNECTION_ERROR);
		}
	}else{
		return respuesta_error(BAD_RESPONSE);
	}
}

Instruccion *recibir_respuesta(int fd_a_escuchar){
	int bytes_recibidos;
	Instruction_set retorno;
	if((bytes_recibidos = recv(fd_a_escuchar, &retorno, sizeof(Instruction_set), MSG_DONTWAIT)) <= 0){
		liberar_conexion(fd_a_escuchar);
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			return respuesta_error(TIMEOUT);
		}else{
			return respuesta_error(CONNECTION_ERROR);
		}
	}switch(retorno){
	case RETORNO:
		return recibir_retorno(fd_a_escuchar);
	case ERROR:
		return recibir_error(fd_a_escuchar);
	default:
		return respuesta_error(UNKNOWN);
	}
}

Instruccion *recibir_error(int fd_a_escuchar){
	Error_set tipo_error;
	int bytes_recibidos;
			if ((bytes_recibidos = recv(fd_a_escuchar, &tipo_error, sizeof(Error_set), MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				return respuesta_error(CONNECTION_ERROR);
			}else{
				switch(tipo_error){
				case BAD_KEY:
					return respuesta_error(BAD_KEY);
				case MISSING_TABLE:
					return respuesta_error(MISSING_TABLE);
				case BAD_REQUEST:
					return respuesta_error(BAD_REQUEST);
				case MISSING_FILE:
					return respuesta_error(MISSING_FILE);
				case CONNECTION_ERROR:
					return respuesta_error(CONNECTION_ERROR);
				case MEMORY_FULL:
					return respuesta_error(MEMORY_FULL);
				case LARGE_VALUE:
					return respuesta_error(LARGE_VALUE);
				default:
					return respuesta_error(UNKNOWN);
				}
			}
}

Instruccion *recibir_retorno(int fd_a_escuchar){
	Tipo_Retorno tipo_ret;
	int bytes_recibidos;
	if ((bytes_recibidos = recv(fd_a_escuchar, &tipo_ret, sizeof(Tipo_Retorno), MSG_DONTWAIT)) <= 0){
		liberar_conexion(fd_a_escuchar);
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			return respuesta_error(TIMEOUT);
		}else{
			return respuesta_error(CONNECTION_ERROR);
		}
	}
	switch(tipo_ret){
		case VALOR:{
			size_t size_val;
			if ((bytes_recibidos = recv(fd_a_escuchar, &size_val, sizeof(size_t), MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return respuesta_error(TIMEOUT);
				}else{
					return respuesta_error(CONNECTION_ERROR);
				}
			}
			char* value = malloc(size_val);
			if ((bytes_recibidos = recv(fd_a_escuchar, value, size_val, MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return respuesta_error(TIMEOUT);
				}else{
					return respuesta_error(CONNECTION_ERROR);
				}
			}
			t_timestamp timestamp;
			if ((bytes_recibidos = recv(fd_a_escuchar, &timestamp, sizeof(t_timestamp), MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return respuesta_error(TIMEOUT);
				}else{
					return respuesta_error(CONNECTION_ERROR);
				}
			}
			return armar_retorno_value(value, timestamp);
		}
		case DATOS_DESCRIBE:{
			t_list *lista_describes;
			lista_describes = list_create();
			size_t cantidad_describes, tamanio;
			if ((bytes_recibidos = recv(fd_a_escuchar, &cantidad_describes, sizeof(cantidad_describes), MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return respuesta_error(TIMEOUT);
				}else{
					return respuesta_error(CONNECTION_ERROR);
				}
			}
			while(cantidad_describes != 0){
				Retorno_Describe *ret_desc = malloc(sizeof(Retorno_Describe));
				if ((bytes_recibidos = recv(fd_a_escuchar, &ret_desc->compactation_time, sizeof(ret_desc->compactation_time), MSG_DONTWAIT)) <= 0){
					liberar_conexion(fd_a_escuchar);
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						return respuesta_error(TIMEOUT);
					}else{
						return respuesta_error(CONNECTION_ERROR);
					}
				}
				if ((bytes_recibidos = recv(fd_a_escuchar, &ret_desc->consistencia, sizeof(ret_desc->consistencia), MSG_DONTWAIT)) <= 0){
					liberar_conexion(fd_a_escuchar);
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						return respuesta_error(TIMEOUT);
					}else{
						return respuesta_error(CONNECTION_ERROR);
					}
				}
				if ((bytes_recibidos = recv(fd_a_escuchar, &ret_desc->particiones, sizeof(ret_desc->particiones), MSG_DONTWAIT)) <= 0){
					liberar_conexion(fd_a_escuchar);
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						return respuesta_error(TIMEOUT);
					}else{
						return respuesta_error(CONNECTION_ERROR);
					}
				}
				if ((bytes_recibidos = recv(fd_a_escuchar, &tamanio, sizeof(tamanio), MSG_DONTWAIT)) <= 0){
					liberar_conexion(fd_a_escuchar);
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						return respuesta_error(TIMEOUT);
					}else{
						return respuesta_error(CONNECTION_ERROR);
					}
				}
				ret_desc->nombre_tabla = malloc(tamanio);
				if ((bytes_recibidos = recv(fd_a_escuchar, ret_desc->nombre_tabla, tamanio, MSG_DONTWAIT)) <= 0){
					liberar_conexion(fd_a_escuchar);
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						return respuesta_error(TIMEOUT);
					}else{
						return respuesta_error(CONNECTION_ERROR);
					}
				}
				list_add(lista_describes, ret_desc);
				free(ret_desc->nombre_tabla);
				free(ret_desc);
				cantidad_describes --;
			}
			return armar_retorno_describe(lista_describes);
		}
		case TAMANIO_VALOR_MAXIMO:{
			size_t max_value;

			if ((bytes_recibidos = recv(fd_a_escuchar, &max_value, sizeof(size_t), MSG_DONTWAIT)) <= 0){
				liberar_conexion(fd_a_escuchar);
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return respuesta_error(TIMEOUT);
				}else{
					return respuesta_error(CONNECTION_ERROR);
					break;
				}
			}
			return armar_retorno_max_value(max_value);
		}
		case SUCCESS:
			return respuesta_success();
		default:
			return respuesta_error(UNKNOWN);
	}
}

Instruccion *respuesta_error(Error_set error){
	Instruccion *respuesta = malloc(sizeof(Instruccion));
	Error * error_a_responder = malloc(sizeof(Error));
	error_a_responder->error = error;
	respuesta->instruccion = ERROR;
	respuesta->instruccion_a_realizar = error_a_responder;
	return respuesta;
}

Instruccion *respuesta_success(void){
	Instruccion *respuesta_success = malloc(sizeof(Instruccion));
	Retorno_Generico *success = malloc(sizeof(Retorno_Generico));
	success->tipo_retorno = SUCCESS;
	respuesta_success->instruccion = RETORNO;
	respuesta_success->instruccion_a_realizar = success;
	return respuesta_success;
}


bool set_timeout(int fd, __time_t timeout){
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)){
		return true;
	}
	log_error(LOGGER, "No se pudo setear Timeout para el fd: %d", fd);
	return false;
}

Instruccion *armar_retorno_value(char *value, t_timestamp timestamp){
	Instruccion *instruccion = malloc(sizeof(Instruccion));
	instruccion->instruccion = RETORNO;
	Retorno_Generico *retorno = malloc(sizeof(Retorno_Generico));
	retorno->tipo_retorno = VALOR;
	Retorno_Value *ret_val = malloc(sizeof(Retorno_Value));
	ret_val->timestamp = timestamp;
	ret_val->value = value;
	retorno->retorno = ret_val;
	instruccion->instruccion_a_realizar = retorno;
	return instruccion;
}

Instruccion *armar_retorno_max_value(size_t max_value){
	Instruccion *instruccion = malloc(sizeof(Instruccion));
	instruccion->instruccion = RETORNO;
	Retorno_Generico *retorno = malloc(sizeof(Retorno_Generico));
	retorno->tipo_retorno = MAX_VALUE;
	Retorno_Max_Value *ret_max_value = malloc(sizeof(Retorno_Max_Value));
	ret_max_value->value_size = max_value;
	retorno->retorno = ret_max_value;
	instruccion->instruccion_a_realizar = retorno;
	return instruccion;
}

Instruccion *armar_retorno_describe(t_list *lista_describes){
	Instruccion *instruccion = malloc(sizeof(Instruccion));
	instruccion->instruccion = RETORNO;
	Retorno_Generico *retorno = malloc(sizeof(Retorno_Generico));
	retorno->tipo_retorno = DATOS_DESCRIBE;
	retorno->retorno = lista_describes;
	instruccion->instruccion_a_realizar = retorno;
	return instruccion;
}

void empaquetar_retorno_valor(t_paquete_retorno *paquete, Retorno_Value *ret_val){
	//Meter en paquete value_size, value y timestamp
}

void empaquetar_retorno_describe(t_paquete_retorno *paquete, t_list *list_of_describes){
	//Meter en paquete la info de todas las tablas que esten en la estructura de describe
}

void empaquetar_retorno_max_val(t_paquete_retorno *paquete, Retorno_Max_Value *max_val){
	//Meter la info del max_value que esta
}

void empaquetar_retorno_success(t_paquete_retorno *paquete){
	//Meter solo un succes y luego el buffer en nulo
}

void empaquetar_retorno_error(t_paquete_retorno *paquete, Error *error){
	//Meter la estructura de error
}
