/* server.c

Copyright 2013 Zdeněk Janeček (ycdmdj@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "server.h"
#include "game.h"

#define BUFFER_LEN 16

int sockfd[MAX_PLAYERS]; /**< server file descriptors */
int listening[MAX_PLAYERS]; /**< array of loop conditions */

/**
 * Initialize used arrays. Run before any pthread_create.
 */
void init_server()
{
   memset((void *) &sockfd, 0, MAX_PLAYERS*sizeof(int));
   memset((void *) &listening, 0, MAX_PLAYERS*sizeof(int));
}

/**
 * server user input listen loop
 * \param param cast to the struct con_info and fill with connection info.
 */
void *start_server(void *param)
{
   int client_len, server_len, n;
   struct sockaddr_in client_address;
   struct sockaddr_in server_address;
   char buffer[BUFFER_LEN];
   char ch;
   
   int uid = ((struct con_info *)param)->uid;
   in_port_t uport = ((struct con_info *)param)->port;
   in_addr_t uaddr = ((struct con_info *)param)->addr;
   
   free(param);
   
   sockfd[uid] = socket(AF_INET, SOCK_DGRAM, 0);
   server_address.sin_family = AF_INET;
   server_address.sin_addr = saddress;
   server_address.sin_port = 0;

   server_len = sizeof(server_address);

   if(bind(sockfd[uid], (struct sockaddr *) &server_address, server_len) != 0)
   {
      perror("oops: server bind error");
      pthread_exit(NULL);
   }
   
   client_len = sizeof(client_address);
   client_address.sin_addr.s_addr = uaddr;
   client_address.sin_port = uport;
   
   ch = M_START;
   sendto(sockfd[uid], &ch, 1, 0, (struct sockaddr*) &client_address, client_len);
   
   listening[uid] = 1;
   //printf("(I) Client thread %d is starting\n", uid);
   
   while (listening[uid])
   {
      n = recvfrom(sockfd[uid], &buffer, BUFFER_LEN, 0, (struct sockaddr *) &client_address, (socklen_t *) &client_len );
      
      // not from our client?
      if (n < 0 || client_address.sin_addr.s_addr != uaddr || client_address.sin_port != uport)
         continue;
      
      switch (buffer[0])
      {
         case M_MOVE:
            want_move(uid, buffer[1]);
            break;
         case M_START:
            want_start(uid);
            break;
         case M_DISCONNECT:
            want_rem_player(uid);
            break;
         case M_WAIT:
            want_be_alive(uid);
            break;
      }
   }

   // disconnect our client
   ch = M_DISCONNECT;
   sendto(server_sockfd, &ch, 1, 0, (struct sockaddr*) &client_address, client_len);

   pthread_exit(NULL);
}

/**
 * Shutdown one thread loop and free the socket file descriptor.
 */
void stop_server(int id)
{
   listening[id] = 0;
   shutdown(sockfd[id], SHUT_RDWR);
   close(sockfd[id]);
}
