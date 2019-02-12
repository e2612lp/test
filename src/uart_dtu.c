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
#include "uart.h"
#include "net.h"

typedef struct 
{
	char buf[64];
    unsigned char tm_threshold;
	bool trans_start_flg;
	bool trans_complete_flg;
	int cnt;
	int pos;
}uart_recv_info_st;

pktlist_uart_t *p_uartlist = NULL;
pthread_mutex_t uartlist_mutex;
pthread_t uart_recv_tmthreshold_id;

int uart_fd = -1;
char write_buf[32] = {"uart_com test.\r\n"};
fd_set fdset;
struct timeval tm_interval;


void uartlist_init(pktlist_uart_t **pplist)
{
	//TODO : assert
	*pplist = NULL;
}

bool uartlist_isempty(pktlist_uart_t *phead)
{
	//TODO : assert
	return (( phead == NULL )?(true):(false));
}

//insert the node to the tail,become the newest
int uartlist_push(pktlist_uart_t ** pphead, pktlist_uart_t * node)
{

	//TODO :assert
	pktlist_uart_t *ptmp  = *pphead;

	//printf("uartlist_push\r\n");
	
	if(uartlist_isempty(*pphead) == true)
	{
	    printf("uartlist_push first node\r\n");
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
pktlist_uart_t *uartlist_pop(pktlist_uart_t **pphead)
{
	pktlist_uart_t *ptmp  = *pphead;
	//printf("list pop..\r\n");
	
    if(true == uartlist_isempty(ptmp))
	{	
		return NULL;
	}

	*pphead = ptmp->p_next;
	return ptmp;
}

void uartlist_travel(pktlist_uart_t *phead)
{
	//TODO :assert
	pktlist_uart_t *ptmp  = phead;
	int i;

	printf("list travel..\r\n");

	for(i = 0; ptmp != NULL; ptmp = ptmp->p_next,i++)
	{ 
		printf("node:%d,addr:%d, data is %s\r\n", i, ptmp, ptmp->data);
	}
	
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if ( tcgetattr( fd,&oldtio) != 0) { 
        perror("SetupSerial 1");
        return -1;
    }
    
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD; 
    newtio.c_cflag &= ~CSIZE; 

    switch( nBits )
    {
        case 7:
            newtio.c_cflag |= CS7;
        break;
        case 8:
           newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
        case 'O':
	    newtio.c_cflag |= PARENB;
	    newtio.c_cflag |= PARODD;
	    newtio.c_iflag |= (INPCK | ISTRIP);
	break;
	case 'E': 
	    newtio.c_iflag |= (INPCK | ISTRIP);
	    newtio.c_cflag |= PARENB;
	    newtio.c_cflag &= ~PARODD;
        break;
        case 'N': 
            newtio.c_cflag &= ~PARENB;
        break;
    }

    switch( nSpeed )
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
        break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
        break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
        break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
        break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
        break;
    }

    if( nStop == 1 )
        newtio.c_cflag &= ~CSTOPB;
    else if ( nStop == 2 )
        newtio.c_cflag |= CSTOPB;


    newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    newtio.c_oflag  &= ~OPOST;   /*Output*/
   
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        perror("com set error");
        return -1;
    }
    
    printf("set done!\n");
    return 0;
}

int open_port(void)
{
    int fd = -1,ret = 0;
	
    fd = open("/dev/ttyS1",O_RDWR|O_NOCTTY|O_NDELAY);
	if (-1 == fd)
	{
		perror("Can't Open Serial Port");
		return(-1);
	}
	else
    { 
		printf("open ttyS1 .....\n");    
	}

    if(isatty(STDIN_FILENO)==0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");
	
	printf("fd-open=%d\n",fd);
    return fd;
}

void setTimer(unsigned int seconds,unsigned int mseconds)
{
    struct timeval temp;
 
    temp.tv_sec = seconds;
    temp.tv_usec = mseconds;
	int err;

	do
	{
		select(0, NULL, NULL, NULL, &temp);
	}while(err<0 && errno==EINTR); 

    return ;
}



int uart_tx_proc(void)
{
	pktlist_net_t *pnode = NULL;

	pthread_mutex_lock(&netlist_mutex);
	pnode = uartlist_pop(&p_netlist);
	pthread_mutex_unlock(&netlist_mutex);
	
	if(NULL != pnode)
	{
		printf("uart get net_data\r\n");

		write(uart_fd, pnode->data, pnode->len);
		
		free(pnode->data);
		free(pnode);
	}

	usleep(5000);
}

int uart_rx_proc(void)
{
	char recv_buff[1600] = {0};
    int pre_read = 0,ret = -1;
	pktlist_uart_t *pnode = NULL;

	FD_ZERO(&fdset);
    FD_SET(uart_fd, &fdset);

	tm_interval.tv_sec = 0;
    tm_interval.tv_usec = 5000;
	
	if(select(uart_fd + 1, &fdset, NULL, NULL, &tm_interval) == -1)
	//if(select(uart_fd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
    	printf("select fail,continue\r\n");
        return -1;      
	}
	
	if(FD_ISSET(uart_fd, &fdset))
	{   			
		setTimer(0, 60000);  // wait 60ms			
		setTimer(0, 60000);  // wait 60ms
		setTimer(0, 20000);  // wait 

		memset((char *)recv_buff, 0, 1600);
		ioctl(uart_fd, FIONREAD, &pre_read);
		printf("pre_rd:%d..\r\n", pre_read);
				
		ret = read(uart_fd, ((char *)recv_buff), 1600);
		//printf("recv is %s\r\n", recv_buff);       
        //printf("read:%d..\r\n", ret);
        
		pnode = malloc(sizeof(pktlist_uart_t));
		pnode->data = malloc(ret + 1);
		pnode->len = ret;

		memcpy(pnode->data, recv_buff, ret);

		pthread_mutex_lock(&uartlist_mutex);
		uartlist_push(&p_uartlist, pnode);
		pthread_mutex_unlock(&uartlist_mutex);
		
    } //end of 	if(FD_ISSET(uart_fd,&fdset))
}


void *uart_transmit(unsigned char *val)
{
	int ret = -1;
	pthread_t uart_recv_thread_id;
	
	uart_fd = open_port();
	if(ret = set_opt(uart_fd, 115200, 8, 'N',1) == -1)
	{
		printf("uart_set_opt fail..");
	    return NULL;
	}

	//mutex init
	if(ret = pthread_mutex_init(&uartlist_mutex, NULL) == -1)
	{
		printf("uartlist_mutex init fail..\r\n");
		return NULL;
	}

	uartlist_init(&p_uartlist);

	write(uart_fd, write_buf, strlen(write_buf));


	while(1)
	{
		uart_rx_proc();
		uart_tx_proc();
    }//end of while(1)

	
}



#if 0
int main_uart(char argc, char *argv[])
{

	tm_interval.tv_sec = 1;
    tm_interval.tv_usec = 0;
    int i = 0;
    char recv_buff[1600] = {0};
    int timer_cnt = 0;
    int read_cnt = 0;
    int pre_read = 0;
	pktlist_uart_t *pnode = NULL;

    struct timeval first, last;

    sleep(2);

    #if 1
	uart_fd = open_port();
	if(ret = set_opt(uart_fd, 115200, 8, 'N',1) == -1)
	{
		printf("uart_set_opt fail..");
	    return -1;
	}
   
    write(uart_fd,write_buf, strlen(write_buf));
    #endif

	//uart_recv_info_init();
	//creat  recv_timer threshold
    //pthread_create(&uart_recv_tmthreshold_id, NULL, (void *)*uart_rx_threshold, &uart_recv_info.tm_threshold);

	/*
	void uartlist_init(pktlist_uart_t **pplist)
	bool uartlist_isempty(pktlist_uart_t *phead)
	int uartlist_push(pktlist_uart_t ** pphead, pktlist_uart_t * node)
	pktlist_uart_t *uartlist_pop(pktlist_uart_t **pphead)
	void uartlist_travel(pktlist_uart_t *phead)
	*/
	#define UARTLIST_TEST 0
	#if UARTLIST_TEST

		printf("uartlist test start\r\n");
	
	    uartlist_init(&p_uartlist);

		char *data = "test_node_1_data\r\n";
		int len = strlen("test_node_1_data\r\n") + 1;
		
		pnode = malloc(sizeof(pktlist_uart_t));
		pnode->data = malloc(len);
		strncpy(pnode->data, data, len);
		pnode->len = len;
        
		printf("insert node 1:%d\r\n", pnode);

		uartlist_push(&p_uartlist, pnode);

		data = "test_node_2_data\r\n";
	    len = strlen("test_node_2_data\r\n") + 1;
	
		pnode = malloc(sizeof(pktlist_uart_t));
		pnode->data = malloc(len);
		strncpy(pnode->data, data, len);
		pnode->len = len;

		printf("insert node 2:%d\r\n", pnode);
		uartlist_push(&p_uartlist, pnode);

		data = "test_node_3_data\r\n";
	    len = strlen("test_node_3_data\r\n") + 1;
	
		pnode = malloc(sizeof(pktlist_uart_t));
		pnode->data = malloc(len);
		strncpy(pnode->data, data, len);
		pnode->len = len;

		printf("insert node 3:%d\r\n", pnode);
		uartlist_push(&p_uartlist, pnode);

		data = "test_node_4_data\r\n";
	    len = strlen("test_node_4_data\r\n") + 1;
	
		pnode = malloc(sizeof(pktlist_uart_t));
		pnode->data = malloc(len);
		strncpy(pnode->data, data, len);
		pnode->len = len;

		printf("insert node 4:%d\r\n", pnode);
		uartlist_push(&p_uartlist, pnode);

		
		uartlist_travel(p_uartlist);

		pnode = uartlist_pop(&p_uartlist);
		printf("pop pnode is %d, pnode_data:%s\r\n", pnode, pnode->data);
        free(pnode->data);		
        free(pnode);

		pnode = uartlist_pop(&p_uartlist);
		printf("pop pnode is %d, pnode_data:%s\r\n", pnode, pnode->data);
        free(pnode->data);		
        free(pnode);
		
		pnode = uartlist_pop(&p_uartlist);
		pnode = uartlist_pop(&p_uartlist);
		pnode = uartlist_pop(&p_uartlist);

		uartlist_travel(p_uartlist);

		printf("uartlist test finish\r\n");	
	
	#endif //end of UARTLIST_TEST

	#if 1
    while(1)
	{

		FD_ZERO(&fdset);
        FD_SET(uart_fd,&fdset);
		
		//ret = select(uart_fd + 1, &fdset, NULL, NULL, &tm_interval);
		ret = select(uart_fd + 1, &fdset, NULL, NULL, NULL);
		
		if(-1 == ret)
        {
        	printf("select fail,continue\r\n");
            continue;      
		}

		if(FD_ISSET(uart_fd, &fdset))
		{   
				
			setTimer(0, 60000);  // wait 60ms			
			setTimer(0, 60000);  // wait 60ms
			setTimer(0, 20000);  // wait 
			
			ioctl(uart_fd, FIONREAD, &pre_read);
			printf("pre_rd:%d..\r\n", pre_read);
			
			
			ret = read(uart_fd, ((char *)recv_buff), 1600);
			printf("recv is %s\r\n", recv_buff);       
            printf("read:%d..\r\n", ret);

			memset((char *)recv_buff, 0, 1600);
	    } //end of 	if(FD_ISSET(uart_fd,&fdset))

    }//end of while(1)
    #endif
    
}
#endif 

