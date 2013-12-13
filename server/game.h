/* game.h
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

#ifndef _GAME_H
#define _GAME_H

#include <netinet/in.h>

#define SNAKE_NAME_LEN 8

#define TOP 'T'
#define LEFT 'L'
#define DOWN 'D'
#define RIGHT 'R'
#define NONE 'N'

#define C_WHITE 50
#define C_RED 51
#define C_GREEN 52
#define C_BLUE 53
#define C_BROWN 54
#define C_YELLOW 55
#define C_ORANGE 56
#define C_PURPLE 57
#define C_BLACK 58
#define C_GREY 59

void want_move(in_addr_t address, uint16_t port, int dir);
void want_new_player(char *name, int color, in_addr_t address, uint16_t port);
void want_rem_player(in_addr_t address, uint16_t port);
void want_start(in_addr_t address, uint16_t port);
void want_be_alive(in_addr_t address, uint16_t port);

void end(int signum);

#endif
