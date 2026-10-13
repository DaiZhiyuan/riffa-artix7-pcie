#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "riffa.h"

struct fpga_t
{
	int fd;
	int id;
};

fpga_t * fpga_open(int id) 
{
	fpga_t * fpga;

	// Allocate space for the fpga_dev
	fpga = (fpga_t *)malloc(sizeof(fpga_t));
	if (fpga == NULL)
		return NULL;
	fpga->id = id;	

	// Open the device file.
	fpga->fd = open("/dev/" DEVICE_NAME, O_RDWR | O_SYNC);
	if (fpga->fd < 0) {
		free(fpga); 
		return NULL;
	}
	
	return fpga;
}

void fpga_close(fpga_t * fpga) 
{
	// Close the device file.
	close(fpga->fd);
	free(fpga);
}

int fpga_send(fpga_t * fpga, int chnl, void * data, int len, int destoff, 
	int last, long long timeout)
{
	fpga_chnl_io io;

	io.id = fpga->id;
	io.chnl = chnl;
	io.len = len;
	io.offset = destoff;
	io.last = last;
	io.timeout = timeout;
	io.data = (char *)data;

	return ioctl(fpga->fd, IOCTL_SEND, &io);
}

int fpga_recv(fpga_t * fpga, int chnl, void * data, int len, long long timeout)
{
	fpga_chnl_io io;

	io.id = fpga->id;
	io.chnl = chnl;
	io.len = len;
	io.timeout = timeout;
	io.data = (char *)data;

	return ioctl(fpga->fd, IOCTL_RECV, &io);
}

void fpga_reset(fpga_t * fpga)
{
	ioctl(fpga->fd, IOCTL_RESET, fpga->id);
}

int fpga_list(fpga_info_list * list) {
	int fd;
	int rc;

	fd = open("/dev/" DEVICE_NAME, O_RDWR | O_SYNC);
	if (fd < 0)
		return fd;
	rc = ioctl(fd, IOCTL_LIST, list);
	close(fd);
	return rc;
}



