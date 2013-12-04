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

int listening = 0;

int server_sockfd; /**< server file descriptor */

/**
 * \brief server user input listen loop
 */
void *start_server(void *param)
{
   int server_len, client_len, n;
   struct sockaddr_in server_address;
   struct sockaddr_in client_address;
   char buffer[BUFFER_LEN];
   char *name;
   char *ptr;
   uint16_t color;

   server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY);
   server_address.sin_port = htons(sport);

   server_len = sizeof(server_address);
   client_len = sizeof(client_address);

   if(bind(server_sockfd, (struct sockaddr *) &server_address, server_len) != 0)
   {
      perror("oops: server bind error");
      end(0);
      pthread_exit(NULL);
   }

   listening = 1;

   while (listening)
   {
      n = recvfrom(server_sockfd, &buffer, BUFFER_LEN, 0, (struct sockaddr *) &client_address, (socklen_t *) &client_len );
      
      if (n < 1)
         continue;
      
      switch (buffer[0])
      {
         case M_CONNECT:
            ptr = strchr((char *) &buffer + 1, '\0'); // find end of string
            name = malloc(ptr - (char *) &buffer); // alloc string
            strcpy(name, (char *) &buffer + 1); // copy name
            color = atoi((char *) (ptr + 1)); // parse color
            want_new_player(name, color, client_address.sin_addr.s_addr, client_address.sin_port);
            break;
         case M_MOVE:
            want_move(client_address.sin_addr.s_addr, client_address.sin_port, buffer[1]);
            break;
         case M_START:
            want_start(client_address.sin_addr.s_addr, client_address.sin_port);
            break;
         case M_DISCONNECT:
            want_rem_player(client_address.sin_addr.s_addr, client_address.sin_port);
            break;
         case M_WAIT:
            want_be_alive(client_address.sin_addr.s_addr, client_address.sin_port);
            break;
      }
   }

   printf("(I) Server is exiting\n");
   pthread_exit(NULL);
}

void stop_server()
{
   listening = 0;
}
