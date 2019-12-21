#include "criterio.h"
#include <stdio.h>
#include "kernel.h"
#include <commons/collections/list.h>
#include <commons/string.h>

void CriteriosFinalizar()
{
             list_destroy(STRONGC);
             list_destroy(STRONGHC);
             list_destroy(EVENTUALC);   
}

void CriterioAgregarMemoria(void *mem,CONSISTENCY_TYPE ct) 
{
        if(STRONGC == NULL && STRONGHC == NULL && EVENTUALC == NULL)
        {
                CriteriosInicializar();
        }
        switch(ct)
        {
                case STRONG_CONSISTENCY:
                        if(list_size(STRONGC) > 0)
                        {
                                printf("Error no puede haber mas de una memoria en Strong Consistency");
                                break;
                        }
                        list_add(STRONGC,mem);
                        break;
                case STRONG_HASH_CONSISTENCY:
                        list_add(STRONGHC,mem);
                        break;
                case EVENTUAL_CONSISTENCY:
                        list_add(EVENTUALC,mem);
                        break;
                default:
                        break;
        }
}

CONSISTENCY_TYPE CriterioGetConsistency(char *ct)
{
        if((strcmp(ct,"SC") == 0) || (strcmp(ct,"SC") == 0))
        {
                return STRONG_CONSISTENCY;
        }else if((strcmp(ct,"shc") == 0) || (strcmp(ct,"SHC") == 0))
        {
                return STRONG_HASH_CONSISTENCY;                
        }else
        {
                return EVENTUAL_CONSISTENCY;
        }
}

t_list* CriterioGetTableConsistency(char *t)
{
        nodoMetadata_t* tabla;
        int i=0;

        if(!list_is_empty(tables_metadata)){
                tabla = list_get(tables_metadata,i);
                while(i < (list_size(tables_metadata)-1) && strcmp(tabla->tabla,t))
                {
                        printf("\n\n\nTAMANIO LISTA TABLES_METADATA: %d",list_size(tables_metadata));
                        printf("\nTABLA -> %s",tabla->tabla);
                        printf("\nTABLA RECIBIDO -> %s",t);
                        printf("\nRESULTADO DE STRCMP: %d",strcmp(tabla->tabla,t));
                        i++;
                        printf("%d",i);
                        tabla=list_get(tables_metadata,i);
                }
                
                if(tabla->metadata->consistencia == SC)
                {
                        return STRONGC;
                }else if(tabla->metadata->consistencia == SHC)
                {
                        return STRONGHC;
                }else{
                        return EVENTUALC;
                }
        } else{
                return NULL;
        }
}

void CriterioActualizar(CONSISTENCY_TYPE t ,char* tabla)
{
        nodoMetadata_t* nodo = malloc(sizeof(nodoMetadata_t));
        nodo->tabla = string_new();
        string_append(&nodo->tabla,tabla);
        switch(t)
        {
                case SC:
                        nodo->metadata->consistencia = SC;
                        list_add(STRONGC,nodo);
                case SHC:
                        nodo->metadata->consistencia = SHC;
                        list_add(STRONGHC,nodo);
                case EC:
                        nodo->metadata->consistencia = EC;
                        list_add(EVENTUALC,nodo);

        }
}

t_list* CriterioDameMemorias(char *criterio)
{
        if(strcmp(criterio,"SC") == 0)
        {
                return STRONGC;
        }else if(strcmp(criterio,"SHC") == 0)
        {
                return STRONGHC;
        }else{
                return EVENTUALC;
        }

}