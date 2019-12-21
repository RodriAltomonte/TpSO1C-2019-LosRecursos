#ifndef _CRITERIO_HH
#define _CRITERIO_HH

#include "memoria.h"
#include <commons/collections/list.h>

typedef enum {STRONG_CONSISTENCY=1,STRONG_HASH_CONSISTENCY,EVENTUAL_CONSISTENCY} CONSISTENCY_TYPE;

void CriterioAgregarMemoria(void *,CONSISTENCY_TYPE);
void CriteriosInicializar();
void CriteriosFinalizar();
CONSISTENCY_TYPE CriterioGetConsistency(char *);
void CriterioActualizar(CONSISTENCY_TYPE,char*);
t_list* CriterioGetTableConsistency(char *);
t_list* CriterioDameMemorias(char *);
unsigned short int MetricsOpsTotales();

#endif