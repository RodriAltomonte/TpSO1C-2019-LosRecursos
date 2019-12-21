#include "metrics.h"

int main(void)
{
	struct metrics_data_s data;
	data.type = STRONG_CONSISTENCY;
	data.command = SELECT;
	data.time_init = 2131231;
	data.time_end = 4354353;

	MetricsAddCommand(data);
	MetricsGetInfo();
	return 1;
}
