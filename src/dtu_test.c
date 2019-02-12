#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>
#include "common.h"
#include "net.h"
#include "uart.h"


int main(char argc, char *argv[])
{
	int i = 0;	int res;
	pthread_t 	tcp_thread_id, uart_thread_id;//返回的线程
	//pthread_mutex_t uartlist_mutex, netlist_mutex;

	char tcp_server_port[32];
	char tcp_server_ip[32];

    //net param init--start
	strcpy(tcp_server_ip, "192.168.1.22");	
	strcpy(tcp_server_port, "4000");

	res = tcp_client_init((unsigned short)atoi(tcp_server_port), tcp_server_ip);//set tcp clinet setting	
	if (res == -1)
	{
		printf("###tcp_server_init error###\n");
		return -1;
	}
	//net param init--end

	//uart param init --start
	
	//uart param init --end
	
	pthread_create(&tcp_thread_id, NULL, (void *)tcp_client_start, NULL);
	printf("tcp_client thread created\r\n");

	pthread_create(&uart_thread_id, NULL, (void *)uart_transmit, NULL);
	printf("uart_transmit thread created\r\n");


	while(1);
		
}
