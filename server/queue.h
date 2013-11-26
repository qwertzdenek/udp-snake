/*	queue.c

	header file for FIFO queue abstract data type.

	Copyright 2003 by Steven S. Skiena; all rights reserved.
   Permission is granted for use in non-commerical applications
   provided this copyright notice remains intact and unchanged. 
   
   http://www.cs.sunysb.edu/~skiena/392/programs/queue.c
   
   modifed by: Zdeněk Janeček 2013
*/

#ifndef _QUEUE_H
#define _QUEUE_H

#include <netinet/in.h>

#define QUEUESIZE   10

typedef struct
{
   char *name;
   int color;
   in_addr_t address;
   uint16_t port;
} node_t;

typedef struct
{
   node_t q[QUEUESIZE+1];  /* body of queue */
   int first;              /* position of first element */
   int last;               /* position of last element */
   int count;              /* number of queue elements */
} queue_t;

void init_queue(queue_t *mq);
void enqueue(queue_t *mq, char *name, int color, in_addr_t address, in_port_t port);
void dequeue(queue_t *mq, char **p_name, int *p_color, in_addr_t *address,
   in_port_t *port);
int empty(queue_t *mq);

#endif
