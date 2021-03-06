/* game.c

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

/*!
\mainpage Snake game
\section intro_sec Introduction

Wanna to play classic game like on old Nokia phones? Now
you can play it with your friends over the network.

\section install_sec Installation
- Step 1

  extract

- Step 2

  compile

- Step 3

  run server: `./snake-server -m [mapname.map] -p [port number]`

- Step 4

  run up to 10 clients: `java kiv.janecekz.Presentation`
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>

#include "game.h"
#include "server-one.h"
#include "server.h"
#include "queue.h"

// **** game constants ****

#define MAX_PACKET_SIZE 1024
#define SNAKE_BODY_LEN 22
#define MAP_NAME_LEN 8
#define MAX_FRUITS 8
#define FRUIT_TIME 15
#define MAP_W 30
#define MAP_H 30

// indexes to the DIRS and INV_DIRS arrays
#define ITOP 0
#define ILEFT 1
#define IDOWN 2
#define IRIGHT 3

/** array of 4 move directions */
const char DIRS[] = {TOP, LEFT, DOWN, RIGHT};

/** indexes to the DIRS[] array to get his inversions (inverse to 0 member is 2) */
const int INV_DIRS[] = {2, 3, 0, 1};

/** default body lenght */
const int DEFAULT_LEN = 3;

/** down orientation */
const int DEFAULT_DOWN[] = {ITOP, ITOP, ITOP};

/** right orientation */
const int DEFAULT_RIGHT[] = {ILEFT, ILEFT, ILEFT};

/** top orientation */
const int DEFAULT_TOP[] = {IDOWN, IDOWN, IDOWN};

/** left orientation */
const int DEFAULT_LEFT[] = {IRIGHT, IRIGHT, IRIGHT};

/**
 * \brief map structure
 */
typedef struct
{
   int height;
   int width;
   /** basename of map (looks for basename.map file) */
   char map_name[MAP_NAME_LEN];
   /** map data (contains SNAKE, WALL, FRUIT and SPACE) */
   int map_matrix[MAP_H][MAP_W];
} map_t;

/**
 * \brief one snake means one player
 */
typedef struct
{
   int id; /**< player id and index to the players array */
   char *name; /**< player name */
   int color; /**< contains C_* constants */
   int hposx; /**< x position of the head on the map */
   int hposy; /**< y position of the head on the map */
   /** \brief body desription
    *
    * One array member is one part of his body.
    * Values are indexes to the DIRS[] array.
    * It means direction to the next part of the body.
    */
   int body[SNAKE_BODY_LEN];
   int body_len; /**< length of the snake body */
   in_addr_t address; /**< client address */
   uint16_t port; /**< client port */
} snake_t;

// **** variables ****
int server_sockfd;  /**< server socket */
in_port_t sport; /**< actual server port */
struct in_addr saddress; /**< actual server address */
map_t skel_map; /**< holds skeleton map structure */
map_t act_map; /**< holds actual map structure */
char *map = NULL; /**< filename of actual map */
int game = 0; /**< true if game is running */
queue_t add_requests; /**< client requests */

int pcount = 0; /**< count of players */
snake_t *players[MAX_PLAYERS]; /**< player array */
char directions[MAX_PLAYERS]; /**< desired indexes to DIRS for each player at one round */
char playing[MAX_PLAYERS]; /**< indicates wheather player is ready to play (0 - false, 1 - true, -1 - leaving) */
int score[MAX_PLAYERS]; /**< score counter for each player */
int fruits[2*MAX_FRUITS]; /**< stores XY coordinates of fruits */
int fruit_count = 0; /**< count of fruit on the map */
int fruit_counter = 0; /**< time to the next fruit respawn */

// **** FUNCTION PROTOTYPES ****
void spawn_player(snake_t *p);
void print_map(map_t *m);

// **** CODE SECTION ****

/**
 * loads desired map to the act_map structure
 * \param name filename of the map
 */
void load_map(char *name)
{
   char basename[20];
   char *ext;
   char c = ' ';
   FILE *f;
   int i, j;
   int width, height;

   if (name == NULL)
      return;

   memset((void *) &skel_map, 0, sizeof(map_t));
   memset((void *) &act_map, 0, sizeof(map_t));

   f = fopen(name, "r");

   if (f == NULL)
   {
      printf("%s", strerror(errno));
      return;
   }

   // backup original name
   strcpy((char *) &basename, name);

   ext = strrchr((char *) &basename, '.');

   if (ext != NULL)
      *ext = '\0';

   strcpy((char *) &skel_map.map_name, (char *) &basename);
   strcpy((char *) &act_map.map_name, (char *) &basename);

   i = 0;
   j = 0;

   // parse width and height
   fgets((char *) basename, 3, f);
   width = atoi((char *) basename);
   fseek(f, 1L, SEEK_CUR);
   fgets((char *) basename, 3, f);
   height = atoi((char *) basename);
   fseek(f, 1L, SEEK_CUR);

   skel_map.height = height;
   skel_map.width = width;

   act_map.height = height;
   act_map.width = width;

   while ((c = (char) fgetc(f)) != EOF)
   {
      if (c == '\n')
      {
         if (++i >= MAP_H)
         {
            printf("(E) Map %s is too high (>%d)\n", basename, MAP_H);
            break;
         }
         printf("\n");
         j = 0;
         continue;
      }

      switch (c)
      {
      case '#':
         skel_map.map_matrix[i][j] = MAX_PLAYERS + 1;
         printf("#");
         break;

      case ' ':
         skel_map.map_matrix[i][j] = MAX_PLAYERS + 3;
         printf("-");
         break;
      }

      if (++j >= MAP_W)
      {
         printf("(E) Map %s is too wide (>%d)\n", basename, MAP_W);
         break;
      }
   }

   fclose(f);

   printf("(I) loaded %s\n", name);
}

/**
 * Grows snake. Tail is added to the direction saved on the
 * old tail. Simple operation that modifies body array.
 * \param s snake to grow
 */
void grow_snake(snake_t *s)
{
   if (s->body_len >= SNAKE_BODY_LEN)
      return;
   
   s->body[s->body_len] = s->body[s->body_len - 1];
   s->body_len++;
}

/**
 * Test if the position is occupied by another snake head.
 * \param posx X coordination
 * \param posy Y coordination
 * \param from snake id to test from
 */
void col_heads(int posx, int posy, int from)
{
   int i;
   snake_t *pl;

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (i == from)
         continue;

      pl = players[i];
      if (pl != NULL && playing[i])
      {
         if (pl->hposx == posx && pl->hposy == posy)
         {
            playing[i] = 0;
            playing[from] = 0;
         }
      }
   }
}

/**
 * Test collision between old and new state of the map.
 */
void solve_collisions()
{
   int i, j, posx, posy;
   int square;
   snake_t *pl;

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      pl = players[i];
      if (pl != NULL && playing[i] == 1)
      {
         posx = pl->hposx;
         posy = pl->hposy;
         square = act_map.map_matrix[posy][posx];

         if (square <= MAX_PLAYERS + 1)
         {
            playing[i] = 0;
         }
         // This is fruit
         else if (square == MAX_PLAYERS + 2)
         {
            j = 0;
            while (posy != fruits[j] || posx != fruits[j + 1])
               j += 2;
            
            fruits[j] = -1;
            fruits[j + 1] = -1;
            
            grow_snake(players[i]);
         }

         // find head collisions
         col_heads(posx, posy, i);
      }
   }
}

/**
 * Erase act_map and replace by new state. Fruit is managed by spawn_fruit
 * function.
 */
void rasterize_act_map()
{
   int i, j;
   int coordx, coordy;

   // copy empty skeleton to act_map
   memcpy((void *) &act_map.map_matrix, (void *) &skel_map.map_matrix, sizeof(int)*MAP_W*act_map.height);

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (players[i] != NULL && playing[i] == 1)
      {
         coordx = players[i]->hposx;
         coordy = players[i]->hposy;

         for (j = 0; j < players[i]->body_len; j++)
         {
            act_map.map_matrix[coordy][coordx] = i + 1;

            switch (players[i]->body[j])
            {
            case ITOP:
               coordy--;
               break;
            case ILEFT:
               coordx--;
               break;
            case IDOWN:
               coordy++;
               break;
            case IRIGHT:
               coordx++;
               break;
            }
         }
      }
   }
}

/**
 * Counts actual game score to the score array
 */
void count_score()
{
   int i;

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (players[i] != NULL && playing[i] == 1)
      {
         score[i] += players[i]->body_len;
      }
   }
}

/**
 * prints snake info to the stdout
 * \param s snake to print
 */
void print_snake(const snake_t *s)
{
   char body[s->body_len + 1];
   int i;

   for (i = 0; i < s->body_len; i++)
   {
      body[i] = DIRS[s->body[i]];
   }

   body[s->body_len] = '\0';
   printf("snake %d: (%d, %d) %s\n", s->id, s->hposx, s->hposy, (char *) body);
}

/**
 * prints help screen to the stdout
 */
void help()
{
   printf("Použití: snake-server [PŘEPÍNAČ]…\n"
                        "Přepínače\n  -h   help screen\n"
                        "  -m {name.map}   map to load\n"
                        "  -p {port number}   listen on port\n"
                        "  -a {IPv4 address/name}   listen on address\n");
}

/**
 * moves snake to the direction choosen. It is not checked.
 * \param s snake to move
 * \param dir move to this direction
 */
void move_snake(snake_t *s, char dir)
{
   // move body
   int i;

   // ignore way to my body
   if (DIRS[s->body[0]] == dir)
      dir = NONE;

   for (i = s->body_len - 1; i > 0; i--)
   {
      s->body[i] = s->body[i - 1];
   }

   if (dir == NONE)
      dir = DIRS[INV_DIRS[s->body[0]]];

   // set head and position
   switch (dir)
   {
   case TOP:
      s->body[0] = IDOWN;
      s->hposy--;
      break;

   case LEFT:
      s->body[0] = IRIGHT;
      s->hposx--;
      break;

   case DOWN:
      s->body[0] = ITOP;
      s->hposy++;
      break;

   case RIGHT:
      s->body[0] = ILEFT;
      s->hposx++;
      break;

   default:
      break;
   }
}

/**
 * registers desired move for the next round
 * \param client_address client IP address
 * \param port client port
 * \param dir char representation of direction
 */
void want_move(int id, int dir)
{
   if (id < 0)
      return;

   #ifdef DEBUG
   printf("Want to move player %d to %c (playing %d)\n", id, dir, playing[id]);
   #endif
   directions[id] = dir;
}

/**
 * registers desired new player for the next round
 * \param name player name
 * \param color color id. Use constants C_* from `game.h`.
 * \param client_address client IP address
 * \param port client port
 */
void want_new_player(char *name, int color, in_addr_t address, uint16_t port)
{
   enqueue(&add_requests, name, color, address, port);
}

/**
 * registers desired remove of the player for the next round
 * \param client_address client IP address
 * \param port client port
 */
void want_rem_player(int id)
{
   if (id < 0)
      return;

   playing[id] = -1;
}

/**
 * registers that player is ready to play
 * \param client_address client IP address
 * \param port client port
 */
void want_start(int id)
{
   if (id < 0)
      return;

   spawn_player(players[id]);

   playing[id] = 1;
}

/**
 * registers that player is ready to play
 * \param client_address client IP address
 * \param port client port
 */
void want_be_alive(int id)
{
   if (id < 0)
      return;
   
   playing[id] = 0;
}

/**
 * Returns first free id to `players` array
 * \return id or -1 when player id was not found
 */
int find_id()
{
   int act = 0;

   while (players[act] != NULL)
   {
      act++;
      if (act >= MAX_PLAYERS)
         return -1;
   }

   return act;
}

/**
 * Prints main map to the stdout
 * \param m map to show
 */
void print_map(map_t *m)
{
   int i, j;
   char c = ' ';

   printf("map %s (%d, %d)\n", m->map_name, m->height, m->width);

   for (i = 0; i < m->height; i++)
   {
      for (j = 0; j < m->width; j++)
      {
         if (m->map_matrix[i][j] <= MAX_PLAYERS)
            c = 's';
         else if (m->map_matrix[i][j] == MAX_PLAYERS + 1)
            c = '#';
         else if (m->map_matrix[i][j] == MAX_PLAYERS + 2)
            c = 'o';
         else if (m->map_matrix[i][j] == MAX_PLAYERS + 3)
            c = ' ';
         
         printf("%c", c);
      }
      printf("\n");
   }

   printf("_______________________________________\n");
}

/**
 * Creates STATUS packet
 * \param buf output buffer
 * \return length of the message
 */
int write_status(char *buf)
{
   int len = 2; // first is packet head (not our problem)
   int i, j, count = 0;
   char num[10];

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (playing[i] >= 0 && players[i] != NULL)
      {
         count++;
         // print his id
         sprintf(num, "%i", i);
         strcpy(buf + len, (char *) &num);
         len += strlen(num) + 1;
         
         // copy name
         strcpy(buf + len, players[i]->name);
         len += strlen(players[i]->name) + 1;

         // print his color
         sprintf(num, "%i", players[i]->color);
         strcpy(buf + len, (char *) &num);
         len += strlen(num) + 1;

         // now the score
         sprintf(num, "%i", score[i]);
         strcpy(buf + len, (char *) &num);
         len += strlen(num) + 1;
      }
   }

   buf[1] = count;

   buf[len++] = '\0';

   sprintf(num, "%i", act_map.height);
   strcpy(buf + len, (char *) &num);
   len += strlen(num) + 1;

   sprintf(num, "%i", act_map.width);
   strcpy(buf + len, (char *) &num);
   len += strlen(num) + 1;

   for (i = 0; i < act_map.height; i++)
   {
      for (j = 0; j < act_map.width; j++)
      {
         buf[len++] = (char) act_map.map_matrix[i][j];
      }
   }

   buf[len++] = '\0';
   return len;
}

/**
 * Writes fruit positions on the map.
 */
void spawn_fruit()
{
   int randx = 0;
   int randy = 0;
   int i;

   if (fruit_counter == 0)
   {
      /* initialize random seed: */
      srand (time(NULL));

      fruit_count = rand() % MAX_FRUITS;

      for (i = 0; i < 2*fruit_count; i += 2)
      {
         do
         {
            randx = rand() % act_map.width;
            randy = rand() % act_map.height;
         }
         while (act_map.map_matrix[randy][randx] != MAX_PLAYERS + 3);

         fruits[i] = randy;
         fruits[i + 1] = randx;
      }
      fruit_counter = FRUIT_TIME;
   }
   else fruit_counter--;

   for (i = 0; i < 2*fruit_count; i += 2)
   {
      if (fruits[i] == -1 || fruits[i + 1] == -1)
         continue;
      act_map.map_matrix[fruits[i]][fruits[i + 1]] = MAX_PLAYERS + 2;
   }
}

/**
 * Gives the player position
 * \param p snake move
 */
void spawn_player(snake_t *p)
{
   int rand_dir;

   p->body_len = DEFAULT_LEN;

   /* initialize random seed: */
   srand (time(NULL));
   rand_dir = rand() % 4;

   p->hposx = act_map.width / 2;
   p->hposy = act_map.height / 2;

   switch (rand_dir)
   {
   case 0:
      memcpy((int *) p->body, (int *) DEFAULT_TOP, DEFAULT_LEN*sizeof(int));
      break;
   case 1:
      memcpy((int *) p->body, (int *) DEFAULT_LEFT, DEFAULT_LEN*sizeof(int));
      break;
   case 2:
      memcpy((int *) p->body, (int *) DEFAULT_DOWN, DEFAULT_LEN*sizeof(int));
      break;
   case 3:
      memcpy((int *) p->body, (int *) DEFAULT_RIGHT, DEFAULT_LEN*sizeof(int));
      break;
   }
}

/**
 * Add player to the game
 * \param name player name
 * \param color C_* constant
 * \param client_address client IP address
 * \param port client port
 * \return player id when was added to the game. -1 when wasn't.
 */
int add_player(char *name, int color, in_addr_t address, uint16_t port)
{
   snake_t s;
   snake_t *player;
   int id, i;
   
   // find player with the same name
   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (players[i] != NULL && (!strcmp(name, players[i]->name) || color == players[i]->color))
      {
         return -1;
      }
   }
   
   id = find_id();

   if (id < 0)
      return -1;

   memset((void *) &s, 0, sizeof(snake_t));

   s.id = id;
   s.name = name;
   s.color = color;
   s.address = address;
   s.port = port;

   player = malloc(sizeof(snake_t));

   if (player == NULL)
   {
      perror("Malloc failed");
      return -1;
   }

   memcpy(player, &s, sizeof(snake_t));
   players[id] = player;
   playing[id] = 0;
   score[id] = 0;
   spawn_player(player);
   pcount++;

   return id;
}

/**
 * main game loop
 */
int run()
{
   // main server thread
   pthread_t listener;
   // listening threads (server.c)
   pthread_t clients[MAX_PLAYERS];
   pthread_attr_t attr;
   FILE *map_file;
   FILE *log_file;
   char *basic_map = "basic.map";
   char *pl_name = NULL;
   char ch;
   char buffer[MAX_PACKET_SIZE]; // for bigger packets
   int pl_color, i, rc, len, server_len;
   in_addr_t address;
   uint16_t port;
   struct sockaddr_in client_address;
   struct sockaddr_in server_address;
   struct in_addr sin_addr;
   int client_len;
   struct con_info *info;
   time_t rawtime;
   struct tm *timeinfo;
   int round = 0;

   if (map == NULL)
   {
      map = malloc(strlen(basic_map) + 1);
      strcpy(map, basic_map);
   }

   map_file = fopen(map, "r");

   if (map_file == NULL)
   {
      printf("(E) Map %s not found\n", map);
      return 1;
   }
   else
      fclose(map_file);

   time(&rawtime);
   timeinfo = localtime(&rawtime);
   strftime(buffer, sizeof(buffer), "Log-%Y-%d-%m-%H%S", timeinfo);
   log_file = fopen(buffer, "w");

   init_queue(&add_requests);
   init_server();
   
   // INITIALIZE main server socket
   server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   server_address.sin_family = AF_INET;
   server_address.sin_addr = saddress;
   server_address.sin_port = sport;

   server_len = sizeof(server_address);

   if(bind(server_sockfd, (struct sockaddr *) &server_address, server_len) != 0)
   {
      perror("oops: server bind error");
      return -1;
   }

   // client init
   client_address.sin_family = AF_INET;
   client_len = sizeof(struct sockaddr_in);

   load_map(map);
   printf("(I) Running game on map %s\n", map);

   /* Initialize and set thread detached attribute */
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   
   // initialize main server thread
   rc = pthread_create(&listener, &attr, start_server_one, NULL);

   if (rc)
   {
      printf("(E) return code from pthread_create() is %d\n", rc);
      return -1;
   }

   memset((void *) &playing, 0, MAX_PLAYERS*sizeof(int));
   memset((void *) players, 0, MAX_PLAYERS*sizeof(snake_t *));
   memset((void *) &clients, 0, MAX_PLAYERS*sizeof(pthread_t));

   game = 1;
   fruit_counter = FRUIT_TIME;

   while (game)
   {
      round++;
      
      // preprocess
      for (i = 0; i < MAX_PLAYERS; i++)
      {
         if (players[i] != NULL)
         {
            if (playing[i] == 1)
            {
               move_snake(players[i], directions[i]);
            }
            else if (playing[i] == 2 || playing[i] == -1)
            {
               // write running round
               sprintf(buffer,"%d: ", round);
               fwrite(buffer, 1, strlen(buffer), log_file);
               
               // and log info
               sprintf(buffer, "Player %s is disconnecting\n", players[i]->name);
               fwrite(buffer, 1, strlen(buffer), log_file);
               printf(buffer);

               free(players[i]->name);
               free(players[i]);
               players[i] = NULL;
               score[i] = 0;
               playing[i] = 0;
               
               stop_server(i);
               pthread_join(clients[i], NULL);
            }
         }
      }
      
      solve_collisions();
      rasterize_act_map();
      spawn_fruit();
      count_score();
      
      memset(&buffer, 0, MAX_PACKET_SIZE);
      len = write_status((char *) &buffer);
      
      // postprocess
      for (i = 0; i < MAX_PLAYERS; i++)
      {
         if (players[i] != NULL)
         {
            client_address.sin_addr.s_addr = players[i]->address;
            client_address.sin_port = players[i]->port;
            
            if (playing[i] == 0)
            {
               #ifdef DEBUG
               printf("** player %d is dead\n", i);
               #endif
               playing[i] = 2; // wait for response
               ch = M_DEAD;
               sendto(server_sockfd, &ch, 1, 0, (struct sockaddr*) &client_address, client_len);
            }
            
            buffer[0] = M_STATE;
            sendto(server_sockfd, &buffer, len, 0, (struct sockaddr*) &client_address, client_len);
         }
      }

      while (!empty(&add_requests))
      {
         dequeue(&add_requests, &pl_name, &pl_color, &address, &port);
         rc = add_player(pl_name, pl_color, address, port);

         if (rc == -1)
         {
            client_address.sin_addr.s_addr = address;
            client_address.sin_port = port;
            ch = M_DISCONNECT;
            sendto(server_sockfd, &ch, 1, 0, (struct sockaddr*) &client_address, client_len);
            free(pl_name);
         }
         else
         {
            info = (struct con_info *) malloc(sizeof(struct con_info));
            info->uid = rc;
            info->addr = address;
            info->port = port;
            
            sin_addr.s_addr = address;
            
            // write running round
            sprintf(buffer,"%d: ", round);
            fwrite(buffer, 1, strlen(buffer), log_file);
            
            // and log info
            sprintf(buffer,"Connected player %d %s, color %d and address %s:%d\n", rc, pl_name, pl_color, inet_ntoa(sin_addr), ntohs(port));
            fwrite(buffer, 1, strlen(buffer), log_file);
            printf(buffer);

            // initialize server thread
            rc = pthread_create(&clients[rc], &attr, start_server, (void *) info);
            if (rc)
            {
               printf("(E) return code from pthread_create() is %d\n", rc);
               free(info);
               want_rem_player(rc);
            }
         }
      }

      memset(directions, NONE, MAX_PLAYERS);
      usleep(GAME_SPEED);
   }
   
   pthread_attr_destroy(&attr);

   // write runtime
   sprintf(buffer,"Running time was %d ticks.\n", round);
   fwrite(buffer, 1, strlen(buffer), log_file);
   fclose(log_file);
   
   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (players[i] != NULL)
      {
         stop_server(i);
         pthread_join(clients[i], NULL);
         
         // disconnect our client
         ch = M_DISCONNECT;
         client_address.sin_addr.s_addr = players[i]->address;
         client_address.sin_port = players[i]->port;
         sendto(server_sockfd, &ch, 1, 0, (struct sockaddr*) &client_address, client_len);
         
         #ifdef DEBUG
         printf("removed %d, client ID %d\n", i, (unsigned)clients[i]);
         #endif
      }
   }

   stop_server_one();
   shutdown(server_sockfd, SHUT_RDWR);
   close(server_sockfd);
   pthread_join(listener, NULL);
   return 0;
}

/**
 * callback for signals to end the game politely
 */
void end(int signum)
{
   game = 0;
}

void clean_while_parse(char **map, char *message)
{
   if (message != NULL)
      printf("%s\n",message);
   if (map != NULL)
   {
      free(*map);
      *map = NULL;
   }
}

/**
 * Tests wheather string s contains number
 * \param s string to test
 */
int is_num(char *s)
{
   int res = 1;
   char *ptr = s;
   
   while (*ptr != 0)
   {
      res &= isdigit((char) *ptr) > 0 ? 1 : 0;
      ptr++;
   }
   
   return res;
}

/**
 * main function
 */
int main(int argc, char *argv[])
{
   int res = 0;
   int i;
   struct addrinfo *saddr;
   struct addrinfo hints;
   char *eptr;
   long lport;
   
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_INET;
   saddress.s_addr = INADDR_ANY;
   sport = htons(9700);
   
   signal(SIGINT, end);

   // parse arguments
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-h") == 0)
      {
         help();
         return 0;
      }
      else if (strcmp(argv[i], "-m") == 0)
      {
         if (i < argc - 1)
         {
            map = malloc(strlen(argv[i + 1]) + 1);

            if (map == NULL)
            {
               perror("Malloc failed");
               return 1;
            }

            strcpy(map, (char *) argv[i + 1]);
            i++;
         }
      }
      else if (strcmp(argv[i], "-p") == 0)
      {
         if (i < argc - 1)
         {
			lport = strtol(argv[i + 1], &eptr, 10);
            if (argv[i + 1] == eptr || lport < 0 || lport > 65535)
            {
               clean_while_parse(&map, "(EE) Wrong port");
               return 1;
            }
            sport = htons((uint16_t) lport);
            i++;
         }
      }
      else if (strcmp(argv[i], "-a") == 0)
      {
         if (i < argc - 1)
         {
            res = getaddrinfo(argv[i + 1], NULL, &hints, &saddr);
            if (res < 0)
            {
               clean_while_parse(&map, "(EE) Error while parsing address");
               return 1;
            }
            
            if (is_num(argv[i + 1]))
            {
               clean_while_parse(&map, "(EE) Not valid IP address");
               return 1;
            }
            
            saddress = ((struct sockaddr_in *) saddr->ai_addr)->sin_addr;
            freeaddrinfo(saddr);
            i++;
         }
      }
      else
      {
         clean_while_parse(&map, "(EE) Wrong arguments");
         help();
         return 1;
      }
   }
   
   // run server
   res = run();

   // cleanup
   if (map != NULL)
	 free(map);

   for (i = 0; i < MAX_PLAYERS; i++)
   {
      if (players[i] != NULL)
      {
         free(players[i]->name);
         free(players[i]);
      }
   }

   /* Last thing that main() should do */
   pthread_exit(&res);
}
