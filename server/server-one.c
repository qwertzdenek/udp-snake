/* server-one.c

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

#define BUFFER_LEN 32
// connect packet
#define M_CONNECT '\0'

int main_listening = 1;

/**
 * \brief server user input listen loop
 */
void *start_server_one(void *param)
{
   int client_len, n;
   struct sockaddr_in client_address;
   char buffer[BUFFER_LEN];
   char *name;
   char *ptr;
   uint16_t color;

   client_len = sizeof(client_address);

   printf("Running main server on %s:%d\n", inet_ntoa(saddress), ntohs(sport));

   while (main_listening)
   {
      n = recvfrom(server_sockfd, &buffer, BUFFER_LEN, 0, (struct sockaddr *) &client_address, (socklen_t *) &client_len );

      if (n < 1)
         continue;
      
      if (buffer[0] == M_CONNECT)
      {
         ptr = strchr((char *) &buffer + 1, '\0'); // find end of string
         name = malloc(ptr - (char *) &buffer); // alloc string
         strcpy(name, (char *) &buffer + 1); // copy name
         color = atoi((char *) (ptr + 1)); // parse color
         want_new_player(name, color, client_address.sin_addr.s_addr, client_address.sin_port);
      }
   }

   printf("(I) Main server thread is exiting\n");
   pthread_exit(NULL);
}

void stop_server_one()
{
   main_listening = 0;
}
