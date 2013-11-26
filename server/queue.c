/*	queue.c

	Implementation of a FIFO queue abstract data type.

	Copyright 2003 by Steven S. Skiena; all rights reserved.
   Permission is granted for use in non-commerical applications
   provided this copyright notice remains intact and unchanged. 
   
   http://www.cs.sunysb.edu/~skiena/392/programs/queue.c
   
   modifed by: Zdeněk Janeček 2013
*/

#include <stdbool.h>
#include <stdio.h>

#include "queue.h"

void init_queue(queue_t *mq)
{
   mq->first = 0;
   mq->last = QUEUESIZE - 1;
   mq->count = 0;
}

void enqueue(queue_t *mq, char *name, int color, in_addr_t address, uint16_t port)
{
   node_t *node;
   
   if (mq->count >= QUEUESIZE)
      printf("Warning: queue overflow enqueue\n");
   else
   {
      mq->last = (mq->last+1) % QUEUESIZE;
      node = (node_t *) &mq->q[ mq->last ];
      node->name = name;
      node->color = color;
      node->address = address;
      node->port = port;
      mq->count = mq->count + 1;
   }
}

void dequeue(queue_t *mq, char **p_name, int *p_color, in_addr_t *address, uint16_t *port)
{
   node_t *node;
   
   if (mq->count <= 0) printf("Warning: empty queue dequeue.\n");
   else
   {
      node = (node_t *) &mq->q[ mq->first ];
      if (p_color != NULL) *p_color = node->color;
      if (p_name != NULL) *p_name = node->name;
      if (address != NULL) *address = node->address;
      if (port != NULL) *port = node->port;
      mq->first = (mq->first+1) % QUEUESIZE;
      mq->count = mq->count - 1;
   }
}

int empty(queue_t *mq)
{
   if (mq->count <= 0) return (true);
   else return (false);
}
