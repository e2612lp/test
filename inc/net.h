#ifndef NET_CONTROL_CLINET_H_
#define NET_CONTROL_CLINET_H_

#include <arpa/inet.h>

#define TCP_BUFFER_SIZE	1024//max buff of receive buffer for tcp

typedef struct{//tcp client class
		unsigned short  	server_port;
		char 				server_ip[64];
		int 				socket_fd;//socket
		struct 				sockaddr_in server_socket_addr;
}tcp_client_t;


extern pktlist_net_t *p_netlist;
extern pthread_mutex_t netlist_mutex;


int tcp_client_init(unsigned short port_num, char *server_ip);
int tcp_client_start(void);

#endif /* NET_CONTROL_CLINET_H_ */
