#ifndef UART_H_
#define UART_H_

extern pktlist_uart_t *p_uartlist;
extern pthread_mutex_t uartlist_mutex;

void uartlist_init(pktlist_uart_t **pplist);
bool uartlist_isempty(pktlist_uart_t *phead);
int uartlist_push(pktlist_uart_t ** pphead, pktlist_uart_t * node);
pktlist_uart_t *uartlist_pop(pktlist_uart_t **pphead);
void uartlist_travel(pktlist_uart_t *phead);
void *uart_transmit(unsigned char *val);

#endif /* UART_H_*/