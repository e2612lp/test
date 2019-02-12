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

tcp_client_t tcp_client_info;

pktlist_net_t *p_netlist = NULL;
pthread_mutex_t netlist_mutex;
struct timeval tm_interval;


void netlist_init(pktlist_net_t **pplist)
{
	//TODO : assert
	*pplist = NULL;
}

bool netlist_isempty(pktlist_net_t *phead)
{
	//TODO : assert
	return (( phead == NULL )?(true):(false));
}

//insert the node to the tail,become the newest
int netlist_push(pktlist_net_t ** pphead, pktlist_net_t * node)
{

	//TODO :assert
	pktlist_net_t *ptmp  = *pphead;

	//printf("netlist_push\r\n");
	
	if(netlist_isempty(*pphead) == true)
	{
	    printf("netlist_push first node\r\n");
		*pphead = node;
		node->p_next = NULL;
	}
	else
	{
		while(ptmp->p_next != NULL)
		{
			ptmp = ptmp->p_next;
		}

		ptmp->p_next = node;
		node->p_next = NULL;
	}

	return 0;
}


//pop the phead_node(the oldest one) directly
pktlist_net_t *netlist_pop(pktlist_net_t **pphead)
{
	pktlist_net_t *ptmp  = *pphead;

	//printf("list pop..\r\n");
	if(true == netlist_isempty(ptmp))
	{	
		return NULL;
	}

	*pphead = ptmp->p_next;
	return ptmp;
}

void netlist_travel(pktlist_net_t *phead)
{
	//TODO :assert
	pktlist_net_t *ptmp  = phead;
	int i;

	printf("list travel..\r\n");

	for(i = 0; ptmp != NULL; ptmp = ptmp->p_next,i++)
	{ 
		printf("node:%d,addr:%d, data is %s\r\n", i, ptmp, ptmp->data);
	}
	
}

static int connect_to_server(void)
{
	while(connect(tcp_client_info.socket_fd, (struct sockaddr *) &(tcp_client_info.server_socket_addr),
			sizeof(struct sockaddr)) != 0)
	{
		//if connect error reconnect after 5 seconds
		perror("connect");
		printf("***reconnect after 5s***\n");
		sleep(5);
		printf("***reconnect...***\n");
	}
	printf("***connected***\n");

	#if 1//test message send
		char test_msg[] = "This is from client";
		send(tcp_client_info.socket_fd, test_msg ,sizeof(test_msg),0);
	#endif

	return 0;
}

/*
     recv socket data
 */
static int receive_packet(int client_fd)
{
	unsigned char 	buf[TCP_BUFFER_SIZE];
	int		recvbytes;
    fd_set fdset;
	pktlist_net_t *pnode = NULL;

    FD_ZERO(&fdset);
    FD_SET(client_fd, &fdset);

	tm_interval.tv_sec = 0;
    tm_interval.tv_usec = 5000;
	
	if(select(client_fd + 1, &fdset, NULL, NULL, &tm_interval) == -1)
	//if(select(client_fd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
    	printf("select fail,continue\r\n");
        return -1;      
	}

	if(FD_ISSET(client_fd, &fdset))
	{

		bzero(buf,sizeof(buf));
		recvbytes = recv(client_fd, buf, TCP_BUFFER_SIZE, 0);  //MSG_DONTWAIT

		if (recvbytes <= 0)
		{	
			//receive error or disconnected
			perror("recv");
			/*reconnect to server*/
			//printf("close socket id = %d\n", tcp_client_info.socket_fd);
			close(tcp_client_info.socket_fd);

			tcp_client_info.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (tcp_client_info.socket_fd == -1){
				perror("socket");
				return -1;
			}
			
			//printf("new socket id = %d\n", tcp_client_info.socket_fd);
			if (connect_to_server() != 0){
				printf("###connect_to_server error###\n");
				return -1;
			}
		}
		else
		{
			//receive success
			#if 1//test
			int i;
			printf("***GET:\n");
			for (i = 0; i < recvbytes; i++){
				printf("0x%02X_%c  ", *(buf+i), *(buf+i));
				if(( i != 0 )&&(i % 8 == 0))
				{
					printf("\n");
				}
			}
			#endif

			pnode = malloc(sizeof(pktlist_net_t));
			pnode->data = malloc(recvbytes);
			memcpy(pnode->data, buf, recvbytes);
			pnode->len = recvbytes;

			pthread_mutex_lock(&netlist_mutex);
			netlist_push(&p_netlist, pnode);
			pthread_mutex_unlock(&netlist_mutex);
			
		}
	}
	
	return 0;
}

/* *
tcp_client initialize function 
* port_num -  TCP server port number
* server_ip - TCP server ip
* */
int tcp_client_init(unsigned short port_num, char *server_ip)
{	
	int res;	
	struct in_addr test_addr;	/*initialize variable*/
	if (port_num > 0)
	{		
		tcp_client_info.server_port = port_num;
	}
	else
	{
		printf("###invalid tcp server port:%d###\n", port_num);
		return -1;
	}

	if (server_ip == NULL)
	{
		printf("###server_ip cannot be NULL###\n");
		return -1;
	}
	else
	{
		strcpy(tcp_client_info.server_ip, server_ip);//record ip
	}	/*建立socket描述符*/	

	if ((tcp_client_info.socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return -1;	
	}

	printf("socket id = %d\n", tcp_client_info.socket_fd);	/*	 * 填充服务器sockaddr结构	 */	
	bzero(&(tcp_client_info.server_socket_addr), sizeof(struct sockaddr_in));

	tcp_client_info.server_socket_addr.sin_family	= AF_INET;

	inet_pton(AF_INET, tcp_client_info.server_ip, &(tcp_client_info.server_socket_addr.sin_addr.s_addr));

	tcp_client_info.server_socket_addr.sin_port	= htons(tcp_client_info.server_port);
	bzero(&(tcp_client_info.server_socket_addr.sin_zero), 8);

	netlist_init(&p_netlist);

	return 0;
}


int receive_uart(void)
{
	pktlist_uart_t *pnode = NULL;

	pthread_mutex_lock(&uartlist_mutex);
	pnode = uartlist_pop(&p_uartlist);
	pthread_mutex_unlock(&uartlist_mutex);
	
	if(NULL != pnode)
	{
		printf("net get uart_data\r\n");

		send(tcp_client_info.socket_fd, pnode->data, pnode->len, 0);

		free(pnode->data);
		free(pnode);
	}

	usleep(3000);
	//printf("net recv uart\r\n");
}



/*tcp clinet running function*/
int tcp_client_start(void)
{
	pthread_t net_recv_thread_id;

	printf("***connect to ip:%s,port:%d .***\n", tcp_client_info.server_ip, tcp_client_info.server_port);
	/*connect to server*/
	if (connect_to_server() != 0){
		printf("###connect_to_server error###\n");
		return -1;
	}


    //mutex init
	if(pthread_mutex_init(&netlist_mutex, NULL) == -1)
	{
		printf("netlist_mutex init fail..\r\n");
		return -1;
	}

	//pthread_create(&net_recv_thread_id, NULL, (void *)net_recv, NULL);
	//printf("net_recv thread created\r\n");
	
	while(1)
	{
		//recv net data
		receive_packet(tcp_client_info.socket_fd);

		//recv uart data
		receive_uart();
	}
	
	return 0;
}

