#include "planificador.h"
#include "thread_pool.h"
#include <pthread.h>
#include "parser.h"

#define PL_DEBUG

thpool *ThreadPool;
char **Requests = NULL;

void round_robin(char**,job_t*);

void PlanificadorRestart(int qq,int mm,int s)
{
	ThreadPoolRestart(qq,mm,s,ThreadPool);
}

/*
 * @brief: Ejecuta n instrucciones y sale
 * @return: Numero de instrucciones ejecutadas
 */
void EjecutarNYSalir(char**,job_t*);

void PlInicializar(int QQ,int MM,int SLEEP)
{
	ThreadPoolSetQuantum(QQ);
	ThreadPoolSetMultiproc(MM);
	ThreadPoolSetSleepTime(SLEEP);
	ThreadPool = ThreadPoolInicializar((USHORT)MM);
}
void PlFinalizar()
{
	ThreadPoolDestruir(ThreadPool);	
}

void PlCrearTrabajos(char **script)
{
	ThreadPoolAgregarTrabajo(ThreadPool,round_robin,script);
	return;
}

void round_robin(char **args,job_t *job_p)
{
	EjecutarNYSalir(args,job_p);
	//usleep(ThreadPoolGetSleepTime());
}

void EjecutarNYSalir(char **script,job_t *job_p)
{
	USHORT qq = 0;
	USHORT QUANTUM = ThreadPoolGetQuantum();
	while( script[job_p->pc + qq] != NULL && qq <= QUANTUM-1) 
	{
		if(!PaParseLineaYEjecutar(script[job_p->pc +qq]))	 
		{
			printf("\nError al ejecutar archivo LQL %d!",job_p->pc);
			job_p->st = FINISHED;
			return;
		}
		//PaParseLineaYEjecutar(script[job_p->pc + qq]);
		qq+=1;
	}
	if(qq < QUANTUM && script[job_p->pc + qq] == NULL)
	{
		job_p->st = FINISHED;
	}
	if (qq < QUANTUM && script[job_p->pc + qq ] != NULL)
	{
		job_p->st = READY;
		job_p->pc+=qq;
	}
	if(qq == QUANTUM && script[job_p->pc + qq] == NULL)
	{
			job_p->st = FINISHED;
	}
	if(qq == QUANTUM && script[job_p->pc + qq] != NULL)
	{
		job_p->st = READY;
		job_p->pc+=qq;
	}
}

