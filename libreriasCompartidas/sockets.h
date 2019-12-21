#ifndef SOCKETS_H_

#define SOCKETS_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "serializacion.h"
#include "../FileSystem/fileSystem.h"
#include "../Kernel/kernel.h"
#include "../Memoria/memoria.h"

#define FAIL -1

int32_t escucharSocket(int32_t puertoObjetivo, t_log *log);
int32_t aceptarConexion(int32_t socketEscucha, t_log *log);
int32_t conectarseA(char *ip, int32_t puertoObjetivo, t_log *log);
int32_t enviarCadena(int32_t socketDestino, char *mensaje);
int32_t recibirCadena(int32_t socketOrigen, char *mensaje);
int32_t enviarInt(int32_t socketDestino, int32_t num);
int32_t recibirInt(int32_t socketDestino, int32_t *i);
int32_t cliente(char *ipCliente, int32_t puertoCliente, int32_t idCliente, t_log *log);
char *devuelveNombreProceso(int32_t idProceso);
int32_t enviarPorSocket(int32_t fdCliente, const void *mensaje, int32_t totalAEnviar, t_log *log);
int32_t recibirPorSocket(int32_t fdCliente, void *buffer, int32_t totalARecibir, t_log *log);
void selectRead(int32_t descriptor, fd_set *bag, struct timeval *timeout, t_log *log);
void selectWrite(int32_t descriptor, fd_set *bag, struct timeval *timeout, t_log *log);
void selectException(int32_t descriptor, fd_set *bag, struct timeval *timeout, t_log *log);

void servidor_inicializar(void *);
#endif
