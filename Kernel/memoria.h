#ifndef _MEM_HH

#include <commons/config.h>

#define _MEM_HH

#define MAX_POOL 1024

typedef struct memoria_info_s
{
        char *ip;
        unsigned short int cantidad_pedidos;
        int puerto;
}memoria_info_t;

#endif
