#ifndef _LIST_H
#define _LIST_H

#include <stdbool.h>
#include <netinet/in.h>

int count();
bool isEmpty();
void add(char *name, int color, in_addr_t address, in_port_t port);
bool poll(char **p_name, int *p_color, in_addr_t *address, in_port_t *port);
void cleanup();

#endif
