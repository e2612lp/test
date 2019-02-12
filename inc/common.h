#ifndef COMMON_H_
#define COMMON_H_

typedef signed char bool;
#define false  (-1)  
#define true   (0)

typedef struct pktlist_uart_st
{
	struct pktlist_uart_st *p_next;
	char *data;
	unsigned int len;
	unsigned int node_info;  //data status or point
}pktlist_uart_t;

typedef struct pktlist_net_st
{
	struct pktlist_net_st *p_next;
	char *data;
	unsigned int len;
	unsigned int node_info;  //data status or point
}pktlist_net_t;


#endif /* COMMON_H_*/