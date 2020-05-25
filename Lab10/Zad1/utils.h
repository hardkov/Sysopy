#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <endian.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <poll.h>
#include <time.h>
#include <netdb.h>

#define _BSD_SOURCE

#define MAX_PLAYERS 10
#define MAX_MSG_LEN 16

#define PING_INTERVAL 10
#define PING_TIMEOUT 5

typedef struct pollfd pollfd;

// msg : type:data

typedef enum MSG_TYPE {
    LOGIN_APPROVED, // type
    LOGIN_REJECTED, // type:reason
    LOGIN_REQUEST, // type:name
    GAME_FINISHED, // type_result
    GAME_MOVE, // type:field_no
    GAME_WAITING, // type
    LOGOUT, // type
    GAME_FOUND, // type:mark
    PING // type
} MSG_TYPE;

typedef struct message {
    MSG_TYPE type;
    char data[MAX_MSG_LEN - 1];
} message;

typedef struct player {
    int fd;
    char name[MAX_MSG_LEN];
    int alive;
} player;

message* read_message(int sock_fd);

message* read_message_nonblocking(int sock_fd);

void send_message(int sock_fd, MSG_TYPE type, char* content);

player* create_player(int fd, char* name);

typedef enum FIELD {
    O = 0,
    X = 1,
    EMPTY = 100
} FIELD;

typedef struct game {
    int player1_idx;
    int player2_idx;
    FIELD board[9];
} game;

typedef enum GAME_STATUS {
    O_WIN,
    X_WIN,
    PLAYING,
    DRAW
} GAME_STATUS;

game* create_new_game(int player1_idx, int player2_idx);

void make_move(game* g, int idx, FIELD mark);

char* parse_board(game* g);

GAME_STATUS check_game_status(game* g);

#endif //UTILS_H