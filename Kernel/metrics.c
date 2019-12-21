#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include "metrics.h"
#include "kernel.h"


struct metrics_info_s *metricas=NULL;

struct metrics_info_s* MetricsGet();

unsigned short int GetCommandIn30SRange(COMMAND_TYPE,CONSISTENCY_TYPE);
double GetCommandExecutionTime(COMMAND_TYPE,CONSISTENCY_TYPE);
unsigned short int GetTimeInMs();

int MetricsMemoryGetCant(COMMAND_TYPE ct,int mem_id)
{
	struct metrics_info_s* aux = metricas;
	int cnt=0;
	while(aux)
	{
		if((aux->data.command == ct) && (aux->data.memoria_id == mem_id))
		{
				cnt++;
		}
		aux = aux->next;
	}
	return cnt;
}

unsigned short int MetricsOpsTotales()
{
	unsigned short int cant =0;
	struct metrics_info_s* aux = metricas;
	while(aux)
	{
		cant++;
	}
	return cant;

}

unsigned short int GetCommandIn30SRange(COMMAND_TYPE comm,CONSISTENCY_TYPE ct)
{
	double time_now = GetTimeInMs();
	unsigned short int cnt = 0;
	struct metrics_info_s* aux = metricas;
	while(aux)
	{
		if((aux->data.command == comm) && (time_now-aux->data.time_init < 30000) && (aux->data.type == ct))
		{
				cnt++;
		}
		aux = aux->next;
	}
	return cnt;
}
double GetCommandExecutionTime(COMMAND_TYPE comm,CONSISTENCY_TYPE ct)
{
	int time_now = GetTimeInMs();
	double ttotal = 0;
	struct metrics_info_s* aux = metricas;

	while(aux)
	{
		if((aux->data.command == comm) && (time_now-aux->data.time_init < 30000) && (aux->data.type == ct) )
		{
			ttotal+=(aux->data.time_end - aux->data.time_init);
		}
		aux = aux->next;
	}
	return ttotal;
}

void MetricsInit(struct metrics_data_s data)
{
	metricas = malloc(sizeof(struct metrics_info_s));

	(metricas)->data = data;
	(metricas)->next = NULL;
}

void MetricsAddCommand(struct metrics_data_s data)
{
	struct metrics_info_s *curr = metricas;
	struct metrics_info_s *tmp;

	if(metricas == NULL)
		MetricsInit(data);
		return;
	
	do
	{
		tmp = curr;
		curr = curr->next;
	
	}while(curr);
	
	struct metrics_info_s *new = malloc(sizeof(struct metrics_info_s));
	new->next=NULL;
	new->data = data;

	tmp->next = new;
	printf("Commando Agregado");
	return;
}

void  MetricsDelete()
{
	struct metrics_info_s *node = metricas;
	do 
	{
		struct metrics_info_s *tmp;
		tmp = node;
		node = node->next;
		free(tmp);
	}while (node);
}

void MetricsPrintReadLatency()
{
	double rl_sc = 0;
	double rl_shc = 0;
	double rl_ec = 0;

	if(metricas != NULL)
	{
		if(GetCommandIn30SRange(SELECT,STRONG_CONSISTENCY) == 0 )
		{
			rl_sc = 0;
		}else{
				rl_sc = GetCommandIn30SRange(SELECT,STRONG_CONSISTENCY) / GetCommandExecutionTime(SELECT,STRONG_CONSISTENCY);	
		}
		if(GetCommandIn30SRange(SELECT,STRONG_HASH_CONSISTENCY) == 0)
		{
			rl_shc = 0;
		}else{
				rl_shc =GetCommandIn30SRange(SELECT,STRONG_HASH_CONSISTENCY) / GetCommandExecutionTime(SELECT,STRONG_HASH_CONSISTENCY);	
		}	
		if(GetCommandIn30SRange(SELECT,EVENTUAL_CONSISTENCY) == 0)
		{
			rl_ec = 0;
		}else{
				rl_ec=GetCommandIn30SRange(SELECT,EVENTUAL_CONSISTENCY) / GetCommandExecutionTime(SELECT,EVENTUAL_CONSISTENCY);	
		}

		printf("\n* STRONG CONSISTENCY READ LATENCY -> %f*",rl_sc);
		printf("\n* STRONG HASH CONSISTENCY READ LATENCY -> %f*",rl_shc);
		printf("\n* EVENTUAL CONSISTENCY READ LATENCY -> %f*",rl_ec);
		return;
	}
	printf ("\n*READ LATENCY -> 0*");
}

void MetricsPrintWriteLatency()
{
	double wl_sc = 0;
	double wl_shc = 0;
	double wl_ec = 0;

	if(metricas != NULL)
	{
		if(GetCommandIn30SRange(INSERT,STRONG_CONSISTENCY) == 0)
		{
			wl_sc = 0;
		}else{
				wl_sc = GetCommandIn30SRange(INSERT,STRONG_CONSISTENCY) / GetCommandExecutionTime(INSERT,STRONG_CONSISTENCY);
		}
		if(GetCommandIn30SRange(INSERT,STRONG_HASH_CONSISTENCY) == 0)
		{
			wl_shc = 0;
		}else{
				wl_shc = GetCommandIn30SRange(INSERT,STRONG_HASH_CONSISTENCY) / GetCommandExecutionTime(INSERT,STRONG_HASH_CONSISTENCY);
		}
		if(GetCommandIn30SRange(INSERT,EVENTUAL_CONSISTENCY) == 0)
		{
			wl_ec = 0;
		}else{
				wl_ec = GetCommandIn30SRange(INSERT,EVENTUAL_CONSISTENCY) / GetCommandExecutionTime(INSERT,EVENTUAL_CONSISTENCY);
		}
		
		printf("\n* STRONG CONSISTENCY WRITE LATENCY -> %f*",&wl_sc);
		printf("\n* STRONG HASH CONSISTENCY WRITE LATENCY -> %f*",&wl_shc);
		printf("\n* EVENTUAL CONSISTENCY WRITE LATENCY -> %f*",&wl_ec);
		return;
	}
	printf("\n*WRITE LATENCY -> 0*");
}
void MetricsPrintReads()
{
	if(metricas != NULL)
	{
		unsigned short int r_sc = GetCommandIn30SRange(SELECT,STRONG_CONSISTENCY);
		unsigned short int r_shc = GetCommandIn30SRange(SELECT,STRONG_HASH_CONSISTENCY);
		unsigned short int r_ec = GetCommandIn30SRange(SELECT,EVENTUAL_CONSISTENCY);

		printf("\n*STRONG CONSISTENCY READS -> %d*",r_sc);
		printf("\n*STRONG HASH CONSISTENCY READS -> %d*",r_shc);
		printf("\n*EVENTUAL CONSISTENCY READS -> %d*",r_ec);
		return;
	}
	printf("\n*READS -> 0*");
}
void MetricsPrintWrites()
{
	if(metricas != NULL)
	{
		unsigned short int w_sc = GetCommandIn30SRange(INSERT,STRONG_CONSISTENCY);
		unsigned short int w_shc = GetCommandIn30SRange(INSERT,STRONG_HASH_CONSISTENCY);
		unsigned short int w_ec = GetCommandIn30SRange(INSERT,EVENTUAL_CONSISTENCY);

		printf("\n*STRONG CONSISTENCY WRITES -> %d*",w_sc);
		printf("\n*STRONG HASH CONSISTENCY WRITES -> %d*",w_shc);
		printf("\n*STRONG CONSISTENCY WRITES -> %d*",w_ec);
		return;
	}
	printf("\n*WRITES -> 0*");

}


void MetricsPrintMemoryLoad()
{
	int i=0;
	printf("\n Memory Load ");
	t_gossip* tg = list_get(lista_memorias,i);
	while(i<list_size(lista_memorias)-1)
	{
		double c = MetricsMemoryGetCant(INSERT,tg->id);
		double d = MetricsMemoryGetCant(SELECT,tg->id);
		int t = MetricsOpsTotales();
		printf("Memoria %d INSERTS -> %f/%d",c,t);
		printf("Memoria %d SELECTS -> %f/%d",d,t);

	}
}


void MetricsGetInfo()
{
	MetricsPrintWriteLatency();
	MetricsPrintReadLatency();
	MetricsPrintReads();
	MetricsPrintWrites();
}

struct metrics_info_s* MetricsGet()
{
	return metricas;
}

unsigned short int GetTimeInMs()
{

	long ms; // Milliseconds
	time_t s;  // Seconds
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);

	s  = spec.tv_sec;
	ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
	if (ms > 999)
   	{
		s++;
		ms = 0;
	}

	//printf("Current time: %"PRIdMAX".%03ld seconds since the Epoch\n",(intmax_t)s, ms);
	return s;
}
