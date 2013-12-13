/* server.h
Header file

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

#ifndef _SERVER_H
#define _SERVER_H

#define M_CONNECT '\0'
#define M_MOVE '\1'
#define M_START '\2'
#define M_STATE '\3'
#define M_DEAD '\4'
#define M_DISCONNECT '\5'
#define M_WAIT '\6'

extern int sport; /**< server port */
extern struct in_addr saddress;  /**< address in network byte order */
extern int server_sockfd;  /**< server socket */

void *start_server(void *param);
void stop_server();

#endif
