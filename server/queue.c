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

/**
 * Initialize used arrays in this queue
 * \param mq queue structure to process
 */
void init_queue(queue_t *mq)
{
   mq->first = 0;
   mq->last = QUEUESIZE - 1;
   mq->count = 0;
}

/**
 * Insert one node to the queue.
 * \param mq structure to process
 * \param name gived string to safe
 * \param color gived int to save
 * \param address IP address in in_addr_t format
 * \param port port int in_port_t format
 */
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

/**
 * Retreive one node from the queue.
 * \param mq structure to process
 * \param name gived string to load
 * \param color gived int to load
 * \param address IP address in in_addr_t format
 * \param port port int in_port_t format
 */
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

/**
 * Tests for the empty queue
 * \param mq queue to process
 * \return 1 if queue is empty and 0 afterwards 
 */
int empty(queue_t *mq)
{
   if (mq->count <= 0) return (true);
   else return (false);
}
