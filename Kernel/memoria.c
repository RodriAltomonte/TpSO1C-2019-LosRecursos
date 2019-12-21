#include "memoria.h"

unsigned short int cant_mems = 0;
memoria_info_t pool_memorias[MAX_POOL];

void MemAgregar(memoria_info_t m)
{
        pool_memorias[cant_mems] = m;
        cant_mems++;
}

