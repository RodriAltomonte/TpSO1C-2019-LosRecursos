#include "thread_pool.h"
#include "signal.h"

int threads_keepalive;
int threads_on_hold;
unsigned short int num_tasks = 0;
threadpool_parameters_t params;

void ThreadPoolSetQuantum(int);
void ThreadPoolSetMultiproc(int);
void ThreadPoolSetSleepTime(int);
unsigned short int ThreadPoolGetQuantum();
unsigned short int ThreadPoolGetMultiproc();
unsigned short int ThreadPoolGetSleepTime();
void ThreadPoolRestart(int,int,int,thpool*);

void ThreadPoolRestart(int qq,int mm,int s,thpool* thp)
{
	ThreadPoolPause(thp);
	ThreadPoolSetQuantum(qq);
	ThreadPoolSetMultiproc(mm);
	ThreadPoolSetSleepTime(s);
	ThreadPoolResume(thp);
}
void ThreadPoolSetQuantum(int qq)
{
	params.QUANTUM = (unsigned short int)qq;
}
void ThreadPoolSetMultiproc(int mm)
{
	params.MULTIPROCESAMIENTO = (unsigned short int)mm;
}
void ThreadPoolSetSleepTime(int s)
{
	params.EXEC_SLEEP = (unsigned short int)s;
}
unsigned short int ThreadPoolGetQuantum()
{
	return params.QUANTUM;
}
unsigned short int ThreadPoolGetMultiProc()
{
	return params.MULTIPROCESAMIENTO;
}
unsigned short int ThreadPoolGetSleepTime()
{
	return params.EXEC_SLEEP;
}

thpool* ThreadPoolInicializar(int cant_threads)
{
	threads_keepalive = 1;
	threads_on_hold = 0;

	thpool* thpool_ptr;
	thpool_ptr = (struct thpool_s*)malloc(sizeof(struct thpool_s));

	if(thpool_ptr == NULL)
	{
		printf("No se pudo reservar memoria para el thread pool");
		return NULL;
	}

	thpool_ptr->num_threads_vivos = 0;
	thpool_ptr->num_threads_trabajando = 0;

	/*Inicializar el jobqueue */
	if(JobQueueInicializar(&thpool_ptr->jobqueue) == -1)
	{
		printf("ThreadPoolInit(): No se pudo inicializar el jobqueue\n");
		free(thpool_ptr);
		return NULL;
	}
	
	thpool_ptr->threads = (struct thread_s**)malloc(cant_threads * sizeof(struct thread_s));
	if(thpool_ptr->threads == NULL)
	{
		printf("ThreadPoolInicializar: No se pudo reservar memoria para los threads\n");
		JobQueueDestruir(&thpool_ptr->jobqueue);
		free(thpool_ptr);
		return NULL;
	}

	pthread_mutex_init(&(thpool_ptr->thcount_lock),NULL);
	pthread_cond_init(&(thpool_ptr->threads_all_idle),NULL);

	int m;
	for(m=0;m<cant_threads;m++)
	{
		thread_init(thpool_ptr,&thpool_ptr->threads[m],m);
	}

	/* Esperar a los threads que inicializen*/
	while(thpool_ptr->num_threads_vivos != cant_threads) {}

	return thpool_ptr;
}
void ThreadPoolPause(thpool* thp)
{
/*
    int n;
	for(n=0;n<thp->num_threads_vivos;n++)
	{
		pthread_kill(thp->threads[n]->pthread,SIGUSR1);
	}
*/
    threads_on_hold = 1;	
}
void ThreadPoolResume(thpool* thp)
{
	threads_on_hold = 0;
}
int ThreadPoolNumeroTrabajosActivos(thpool *thp)
{
	return thp->num_threads_trabajando;
}

int ThreadPoolAgregarTrabajo(thpool* thp,void (*func_p)(void**),void** args)
{
	job_t* njob;
	num_tasks++;
	njob =(struct job_s*)malloc(sizeof(struct job_s));
	if(njob == NULL)
	{
	 printf("ThreadPoolAgregarTrabajo(): No se pudo asignar memoria para nuevo trabajo\n");
	 return -1;
	}
		njob->func = func_p;
		njob->args = args;
		njob->pc = 0;
		njob->st = NEW;
		njob->pid = num_tasks;

		JobQueuePush(&thp->jobqueue,njob);
		return 0;
}


void ThreadPoolDestruir(thpool* thp)
{
	if(thp == NULL) return ;

	int cnt_threads = thp->num_threads_vivos;
	
	threads_keepalive=0;
	
	double TIMEOUT = 1.0;
	time_t start,end;
	double tpasado=0.0;

	time(&start);
	while(tpasado < TIMEOUT && thp->num_threads_vivos)
	{
		bsem_post_all(thp->jobqueue.tiene_trabajos);
		time(&end);
		tpasado = difftime(end,start);
	}
	
	while(thp->num_threads_vivos)
	{
		
		bsem_post_all(thp->jobqueue.tiene_trabajos);
		sleep(1);
	}

	JobQueueDestruir(&thp->jobqueue);
	
	int n;
	for(n=0;n< cnt_threads;n++)
	{
		thread_destroy(thp->threads[n]);
	}
	free(thp->threads);
	free(thp);
}

int thread_init(thpool* thp,thread_t** thread_p,int id)
{
	*thread_p = (struct thread_s*)malloc(sizeof(struct thread_s));
	if(thread_p==NULL)
	{
		printf("thread_init(): No se pudo reservar memoria para el thread");
	}

	(*thread_p)->thpool_ptr = thp;
	(*thread_p)->id = id;

	pthread_create(&(*thread_p)->pthread,NULL,(void*)thread_do,(*thread_p));
	pthread_detach((*thread_p)->pthread);
	return 0;	
}

void thread_destroy(thread_t* thread_p)
{
	free(thread_p);
}

void thread_hold(int sigid)
{
	(void)sigid;
	threads_on_hold = 1; 
	while(threads_on_hold)
	{
		sleep(1);
	}
}
void* thread_do(thread_t* thread_p)
{
	thpool* thpool_p = thread_p->thpool_ptr;
/*
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags=0;
	action.sa_handler = thread_hold;
	if(sigaction(SIGUSR1,&action,NULL) == -1){
		perror("Error: No se pudo atrapar SIGUSR1");
	}
*/
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_vivos+=1;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	while(threads_keepalive && !threads_on_hold)
	{
		bsem_wait(thpool_p->jobqueue.tiene_trabajos);
		if(threads_keepalive && !threads_on_hold)
		{
			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_trabajando++;
			pthread_mutex_unlock(&thpool_p->thcount_lock);

			void (*func_buff)(void **,job_t *);
			void* args_buff;

			job_t* job_p=JobQueuePull(&thpool_p->jobqueue);

			if(job_p)
			{
				func_buff = job_p->func;
				args_buff = job_p->args;

				func_buff(args_buff,job_p);

				if(job_p->st == FINISHED)
				{
					free(job_p->args);
					free(job_p);	
				}else if(job_p->st == READY)
				{
					JobQueuePush(&thpool_p->jobqueue,job_p);
				}
			
			}
			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_trabajando--;

			if(!thpool_p->num_threads_trabajando)
			{
				pthread_cond_signal(&thpool_p->threads_all_idle);
			}
			pthread_mutex_unlock(&thpool_p->thcount_lock);

		}

	}
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_vivos--;
	pthread_mutex_unlock(&thpool_p->thcount_lock);
	return NULL;
}

int JobQueueInicializar(jobqueue_t* jq_ptr)
{
	jq_ptr->len = 0;
	jq_ptr->frente=NULL;
	jq_ptr->posterior=NULL;

	jq_ptr->tiene_trabajos = (struct bsem_s*)malloc(sizeof(struct bsem_s));

	if(jq_ptr->tiene_trabajos == NULL)
	{
		puts("JobQueueInicializar():Error al asignar memoria!\n");
		return -1;
	}
	pthread_mutex_init(&(jq_ptr->rwmutex),NULL);
	bsem_init(jq_ptr->tiene_trabajos,0);

	return 0;
}

void JobQueuePush(jobqueue_t* jq_ptr,job_t* newjob)
{
	pthread_mutex_lock(&jq_ptr->rwmutex);
	newjob->prev = NULL;

	switch(jq_ptr->len)
	{
		case 0:
			jq_ptr->frente = newjob;
			jq_ptr->posterior = newjob;
			break;
		default:
			jq_ptr->posterior->prev = newjob;
			jq_ptr->posterior = newjob;
	}
	jq_ptr->len++;

	bsem_post(jq_ptr->tiene_trabajos);
	pthread_mutex_unlock(&jq_ptr->rwmutex);
}

job_t* JobQueuePull(jobqueue_t* jq_ptr)
{
	pthread_mutex_lock(&jq_ptr->rwmutex);
	job_t* job_p = jq_ptr->frente;

	switch(jq_ptr->len)
	{
		case 0:
			break;
		case 1:
			jq_ptr->frente=NULL;
			jq_ptr->posterior = NULL;
			jq_ptr->len = 0;
			break;
		default:
				jq_ptr->frente = job_p->prev;
				jq_ptr->len--;
				bsem_post(jq_ptr->tiene_trabajos);
	}

	pthread_mutex_unlock(&jq_ptr->rwmutex);
	return job_p;
}

void JobQueueClear(jobqueue_t* jobqueue_p)
{
	while(jobqueue_p->len)
	{
		free(JobQueuePull(jobqueue_p));
	}

	jobqueue_p->frente = NULL;
	jobqueue_p->posterior = NULL;
	bsem_reset(jobqueue_p->tiene_trabajos);
	jobqueue_p->len = 0;
}

void JobQueueDestruir(jobqueue_t* jq_ptr)
{
	JobQueueClear(jq_ptr);
	free(jq_ptr->tiene_trabajos);
}

void bsem_init(bsem* bsem_p,int value)
{
	if(value <0 || value > 1)
	{
		printf("bsem_init():Error Semaforo binario solo puede tener un valor entre 0 y 1\n");
		exit(1);
	}
	pthread_mutex_init(&(bsem_p->mutex),NULL);
	pthread_cond_init(&(bsem_p->cond),NULL);
	bsem_p->v = value;
}

void bsem_reset(bsem *bsem_p)
{
	bsem_init(bsem_p,0);
}

void bsem_post(bsem* bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v=1;
	pthread_cond_signal(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}

void bsem_post_all(bsem* bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v= 1;
	pthread_cond_broadcast(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}

void bsem_wait(bsem* bsem_p)
{
	pthread_mutex_lock(&bsem_p->mutex);
	while(bsem_p->v != 1)
	{
		pthread_cond_wait(&bsem_p->cond,&bsem_p->mutex);
	}
	bsem_p->v=0;
	pthread_mutex_unlock(&bsem_p->mutex);
}
