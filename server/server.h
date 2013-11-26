#ifndef _SERVER_H
#define _SERVER_H

#define M_CONNECT '\0'
#define M_MOVE '\1'
#define M_START '\2'
#define M_STATE '\3'
#define M_DEAD '\4'
#define M_DISCONNECT '\5'
#define M_WAIT '\6'

extern int sport;
extern int server_sockfd;

void *start_server(void *param);
void stop_server();

#endif
