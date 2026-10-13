#ifndef RIFFA_H
#define RIFFA_H

#include "riffa_driver.h"

struct fpga_t;
typedef struct fpga_t fpga_t;

/**
 * Populates the fpga_info_list pointer with all FPGAs registered in the system.
 * Returns 0 on success, a negative value on error.
 */
int fpga_list(fpga_info_list * list);

/**
 * Initializes the FPGA specified by id. On success, returns a pointer to a 
 * fpga_t struct. On error, returns NULL. Each FPGA must be opened before any 
 * channels can be accessed. Once opened, any number of threads can use the 
 * fpga_t struct.
 */
fpga_t * fpga_open(int id);

/**
 * Cleans up memory/resources for the FPGA specified by the fd descriptor.
 */
void fpga_close(fpga_t * fpga);

/**
 * Sends len words (4 byte words) from data to FPGA channel chnl using the 
 * fpga_t struct. The FPGA channel will be sent len, destoff, and last. If last
 * is 1, the channel should interpret the end of this send as the end of a
 * transaction. If last is 0, the channel should wait for additional sends 
 * before the end of the transaction. If timeout is non-zero, this call will 
 * send data and wait up to timeout ms for the FPGA to respond (between
 * packets) before timing out. If timeout is zero, this call may block 
 * indefinitely. Multiple threads sending on the same channel may result in 
 * corrupt data or error. This function is thread safe across channels.
 * On success, returns the number of words sent. On error returns a negative 
 * value. 
 */
int fpga_send(fpga_t * fpga, int chnl, void * data, int len, int destoff, 
	int last, long long timeout);

/**
 * Receives data from the FPGA channel chnl to the data pointer, using the 
 * fpga_t struct. The FPGA channel can send any amount of data, so the data 
 * array should be large enough to accommodate. The len parameter specifies the
 * actual size of the data buffer in words (4 byte words). The FPGA channel will
 * specify an offset which will determine where in the data array the data will
 * start being written. If the amount of data (plus offset) exceed the size of
 * the data array (len), then that data will be discarded. If timeout is 
 * non-zero, this call will wait up to timeout ms for the FPGA to respond 
 * (between packets) before timing out. If timeout is zero, this call may block 
 * indefinitely. Multiple threads receiving on the same channel may result in 
 * corrupt data or error. This function is thread safe across channels.
 * On success, returns the number of words written to the data array. On error 
 * returns a negative value. 
 */
int fpga_recv(fpga_t * fpga, int chnl, void * data, int len, long long timeout);

/**
 * Resets the state of the FPGA and all transfers across all channels. This is
 * meant to be used as an alternative to rebooting if an error occurs while 
 * sending/receiving. Calling this function while other threads are sending or
 * receiving will result in unexpected behavior.
 */
void fpga_reset(fpga_t * fpga);

#endif
