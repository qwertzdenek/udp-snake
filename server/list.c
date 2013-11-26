#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "list.h"

int n = 0;
static struct Node_t *first = NULL;
static struct Node_t *last = NULL;

int count()
{
   return n;
}

bool isEmpty()
{
   return top == NULL;
}

void add(char *name, int color, in_addr_t address, in_port_t port)
{
   assert(name);

   node_t *node = (node_t *) malloc(sizeof(node_t));

   if (node == NULL)
   {
      perror("Malloc failed");
      return;
   }

   node->name = name;
   node->color = color;
   node->address = address;
   node->port = port;

   if (last == NULL)
   {
      node->next = NULL;
      first = node;
   }
   else
      node->next = last->next;

   last = node;
   n++;
}

bool poll(char **p_name, int *p_color, in_addr_t *address, in_port_t *port)
{
   node_t *tmp;

   if (top == NULL)
      return false;

   if (p_color != NULL) *p_color = first->color;

   if (p_name != NULL) *p_name = top->name;
   
   if (address != NULL) *address = first->address;
   
   if (port != NULL) *port = first->port;

   tmp = top;
   top = tmp->next;
   n--;

   free(tmp->name);
   free(tmp);

   return true;
}

void cleanup()
{
   while (n > 0)
      poll(NULL, NULL, NULL, NULL);
}
