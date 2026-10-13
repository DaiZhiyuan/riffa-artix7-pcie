#include <stdio.h>
#include <stdlib.h>
#include "riffa.h"
#include "pcie_bandwidth_test.h"

#define GREEN_TEXT "\033[1;32m"
#define RED_TEXT "\033[1;31m"
#define RESET_TEXT "\033[0m"

fpga_t * fpga;
fpga_info_list info;
unsigned int * send_buffer;
unsigned int * recv_buffer;

int fpga_id;
int seq;
int send_num;
int recv_num;
float tx_bandwidth;
float rx_bandwidth;

int test_status;

int main() 
{
    // Detect of FPGA Devices, exit program if none 
    test_status += fpga_device_detec();
    if(test_status < 0)
    {
        printf("Riffa FPGA Device detect fail, exit...\n");
        printf(RED_TEXT);
        printf("\n");
        printf("#######    #      ###   #\n");
        printf("#         # #      #    #\n");
        printf("#        #   #     #    #\n");
        printf("#####   #     #    #    #\n");
        printf("#       #######    #    #\n");
        printf("#       #     #    #    #\n");
        printf("#       #     #   ###   #######\n");
        printf("\n");
        printf(RESET_TEXT);
        system("pause");
        return -1;
    }

    // test all FPGA one by one
    for(fpga_id = 0; fpga_id < info.num_fpgas; fpga_id++)
    {
        // print FPGA infomations
        printf("========== FPGA Device %d =========\n", fpga_id);
        printf("ID: %d \n", info.id[fpga_id]);
        printf("Number of Channels: %d \n", info.num_chnls[fpga_id]);
        printf("Name: %s \n", info.name[fpga_id]);
        printf("Vendor ID: %04X \n", info.vendor_id[fpga_id]);
        printf("Device ID: %04X \n", info.device_id[fpga_id]);

        // open FPGA and test
        fpga = fpga_open(fpga_id);
        if(fpga == NULL)
        {
            printf("Could not open FPGA ID %d, skip this device \n", fpga_id);
        }
        else
        {
            printf("\n------------------- PCIE RIFFA DATA LOOPBACK TEST ----------------\n");
            printf("===============================================================================================\n");
            printf("| %-8s | %-10s | %-15s | %-8s | %-15s | %-8s |\n", 
            "CH", "SIZE(B)", "TX_BANDWIDTH(MB/s)", "TX_TIME(ms)", "RX_BANDWIDTH(MB/s)", "RX_TIME(ms)");
            test_status += riffa_data_loopback_test(256);
            test_status += riffa_data_loopback_test(256*16);
            test_status += riffa_data_loopback_test(256*256);
            test_status += riffa_data_loopback_test(256*1024);
            test_status += riffa_data_loopback_test(1024*1024);
            test_status += riffa_data_loopback_test(1024*1024*16);
            printf("-----------------------------------------------------------------------------------------------\n");
        }

        // judge PCIE test result with max bandwidth and test_status
        if((tx_bandwidth >= 1400) & (rx_bandwidth >= 1400) & (test_status == 0))
        {
            printf(GREEN_TEXT);
            printf("\n");
            printf("######     #     #####   #####\n");
            printf("#     #   # #   #     # #     #\n");
            printf("#     #  #   #  #       #\n");
            printf("######  #     #  #####   #####\n");
            printf("#       #######       #       #\n");
            printf("#       #     # #     # #     #\n");
            printf("#       #     #  #####   #####\n");
            printf("\n");
            printf(RESET_TEXT);
        }
        else
        {
            printf(RED_TEXT);
            printf("\n");
            printf("#######    #      ###   #\n");
            printf("#         # #      #    #\n");
            printf("#        #   #     #    #\n");
            printf("#####   #     #    #    #\n");
            printf("#       #######    #    #\n");
            printf("#       #     #    #    #\n");
            printf("#       #     #   ###   #######\n");
            printf("\n");
            printf(RESET_TEXT);
        }

        // close fpga
        fpga_close(fpga);
    }

    return 0;
}

int fpga_device_detec()
{
    // get FPGA infomations, if none, polulate error and exit.
    if(fpga_list(&info) != 0)
    {
        printf("Error: populating fpga_info_list fail!!!\n");
        return -1;
    }
    else if(info.num_fpgas == 0)
    {
        printf("Error: no FPGA device found, please check FPGA device or driver...\n");
        return -1;
    }
    else
    {
        printf("Number of FPGA device: %d \n", info.num_fpgas);
        return 0;
    }
}

int riffa_data_loopback_test(int data_length_in_byte)
{
    float tx_time;
    float rx_time;
    // init time capture
    GET_TIME_INIT(4);
    // reset FPGA
    fpga_reset(fpga);

   // malloc the arrays and fill them
   send_buffer = (unsigned int *)malloc(data_length_in_byte << 2);
   if(send_buffer == NULL)
   {
       printf("Error: Could not malloc memory for send_buffer \n");
       fpga_close(fpga);
   	return -1;
   }
   recv_buffer = (unsigned int *)malloc(data_length_in_byte << 2);
   if(send_buffer == NULL)
   {
       printf("Error: Could not malloc memory for recv_buffer \n");
       free(send_buffer);
       fpga_close(fpga);
   	return -1;
   }
   for(seq = 0; seq < data_length_in_byte; seq++)
   {
       send_buffer[seq] = seq + 1;
       recv_buffer[seq] = 0;
   }

   // test all channel read and write
   for(int channel = 0; channel < info.num_chnls[fpga_id]; channel++)
   {
        // send data
        GET_TIME_VAL(0);
        send_num = fpga_send(fpga, channel, send_buffer, data_length_in_byte, 0, 1, 25000);
        GET_TIME_VAL(1);

        // recive data if send success
        if(send_num != 0)
        {
            GET_TIME_VAL(2);
            recv_num = fpga_recv(fpga, channel, recv_buffer, data_length_in_byte, 25000);
            GET_TIME_VAL(3);
        }
        else
        {
            printf("Error: Send data fail in channel %d \n", channel);
            return -1;
        }

        // print recv_num data for check and calculate bandwidth
        if(recv_num != 0)
        {
            // cacu badwidth and used time
            tx_bandwidth = send_num*4.0/1024/1024/((TIME_VAL_TO_MS(1) - TIME_VAL_TO_MS(0))/1000.0);
            rx_bandwidth = recv_num*4.0/1024/1024/((TIME_VAL_TO_MS(3) - TIME_VAL_TO_MS(2))/1000.0);
            tx_time = TIME_VAL_TO_MS(1) - TIME_VAL_TO_MS(0);
            rx_time = TIME_VAL_TO_MS(3) - TIME_VAL_TO_MS(2);

            // print Result
            printf("| %-8d | %-10d | %-18.2f | %-11.4f | %-18.2f | %-11.4f |\n",channel ,data_length_in_byte ,tx_bandwidth, tx_time, rx_bandwidth, rx_time);
        }
        else
        {
            printf("Error: Recive data fail in channel %d \n", channel);
            return -1;
        }

        // clear recive buffer
        for(seq = 0; seq < data_length_in_byte; seq++)
        {
            recv_buffer[seq] = 0;
        }
    }
    return 0;
}

