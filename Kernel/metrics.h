#ifndef _METRICS_HH
#define _METRICS_HH

#include "criterio.h"

typedef enum COMMAND_TYPE{SEL=1,INS} COMMAND_TYPE;

struct metrics_data_s
{
	CONSISTENCY_TYPE type;
	COMMAND_TYPE command;
	int memoria_id;
	int time_end;
	int time_init;
};

struct metrics_info_s
{
	struct metrics_info_s *next;
	struct metrics_data_s data;
};


void MetricsGetInfo();
//ACA habria que poner un mutex porque sino hay colisiones con los threads
void MetricsAddCommand(struct metrics_data_s );
void MetricsPrintReadLatency();
void MetricsPrintWriteLatency();
void MetricsPrintReads();
void MetricsPrintWrites();
void MetricsPrintMemoryLoad();
void MetricsInit(struct metrics_data_s );
void MetricsDelete();

#endif
