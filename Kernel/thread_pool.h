#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H

#include <pthread.h>
typedef enum {READY=1,FINISHED,NEW} STATUS;

typedef struct job_s
{
	struct job* prev;
	void (*func)(void **args,struct job_s *job);
	void **args;
	unsigned short int pc;   //Program counter
	STATUS st;
	unsigned short int pid;
	//int failed;
}job_t;

/*Semaforo binario */
typedef struct bsem_s
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int v;
}bsem;

typedef struct jobqueue_s
{
	pthread_mutex_t rwmutex; // R/W Queue Mutex
	bsem *tiene_trabajos;
	job_t *frente;
	job_t *posterior;
	int len;
}jobqueue_t;

typedef struct thread_s
{
	int id;
	pthread_t pthread;
	struct thpool* thpool_ptr;
}thread_t;

typedef struct thpool_s
{
	thread_t** threads;
	jobqueue_t jobqueue;
	int num_threads_vivos;
	int num_threads_trabajando;
	pthread_mutex_t thcount_lock;
	pthread_cond_t threads_all_idle;
}thpool;

//=====================THREAD POOL=====================

typedef struct threadpool_parameters_s
{
	unsigned short int QUANTUM;
	unsigned short int MULTIPROCESAMIENTO;
	unsigned short int METADATA_REFRESH;
	unsigned short int EXEC_SLEEP;
}threadpool_parameters_t;

/*
 * @brief: Inicializa el thread pool
 * @params cant_threads: Cantidad de threads que va a tener el pool
 * @return: Puntero al thread pool creado
 */
thpool* ThreadPoolInicializar(int cant_threads);

/*
 * @brief: Agrega un nuevo thread para ejecutar requests
 * @params thp: Puntero al thread pool
 * @params func_p: Puntero a la funcion que va a realizar el trabajo
 * @params args: Lista que contiene los requests para ser parseados y ejecutados
 */
int ThreadPoolAgregarTrabajo(thpool* thp,void (*func_p)(void**),void** args);

/*
 * @brief: Libera la memoria utilizada por el thread pool
 * @params thp : Puntero al thp
 * @return: Nada
 */
void ThreadPoolDestruir(thpool* thp);

/*
 * @brief: Pausa la ejecucion de los threads
 * @params thp: Puntero al thread pool
 * @return: Nada
 */
void ThreadPoolPause(thpool* thp);

/*
 * @brief: Resume la ejecucuion de los threads
 * @params thp: Puntero al thread pool 
 * @return: Nada
 */
void ThreadPoolResume(thpool* thp);

int ThreadPoolNumeroTrabajosActivos(thpool* thp);
//=====================THREAD POOL=====================



//=====================THREAD=====================
/*
 * @brief: Inicializa un thread dentro del thread pool
 * @params thp_ptr: puntero al thread pool
 * @params thread_ptr: Puntero al thread a inicializar
 * @params id: Id del thread
 */
int thread_init(thpool* thp_ptr,thread_t** thread_ptr,int id);


void *thread_do(thread_t* thread_ptr);

void thread_destroy(thread_t* thread_ptr);
void thread_hold();

//=====================THREAD=====================

//=====================JOBQUEUE=====================
int JobQueueInicializar(jobqueue_t* jq_ptr);
void JobQueuePush(jobqueue_t* jq_ptr,job_t* newjob);
job_t* JobQueuePull(jobqueue_t* jq_ptr);
void JobQueueDestruir(jobqueue_t* jq_ptr);
//=====================JOBQUEUE=====================

//====================SINCRONIZACION================
void bsem_init(bsem*,int);
void bsem_reset(bsem*);
void bsem_post(bsem*);
void bsem_post_all(bsem*);
void bsem_wait(bsem*);
//====================SINCRONIZACION================


#endif
