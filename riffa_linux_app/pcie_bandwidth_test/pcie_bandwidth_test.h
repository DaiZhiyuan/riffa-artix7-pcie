#ifndef ARTIX7_TEST
#define ARTIX7_TEST

#include <time.h>
#define GET_TIME_INIT(num)  struct timespec _timers[num]
#define GET_TIME_VAL(num)   clock_gettime(CLOCK_MONOTONIC, &_timers[num])
#define TIME_VAL_TO_MS(num) ((double)_timers[num].tv_sec * 1000.0 + (double)_timers[num].tv_nsec / 1000000.0)

int fpga_device_detec();
int riffa_data_loopback_test(int data_length_in_byte);

#endif
