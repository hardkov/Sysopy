#include "utils.h"

player* players[MAX_PLAYERS];

int port_number;
char* socket_path;

int sock_un_fd;
struct sockaddr_un sock_unix;

int sock_in_fd;
struct sockaddr_in sock_inet;

pthread_t connection_tid;
pthread_t ping_tid;

int waiting_player;

game* games[MAX_PLAYERS / 2];
FIELD player_marks[MAX_PLAYERS];
int player_games[MAX_PLAYERS];

pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;

void start_server() {
    sock_unix.sun_family = AF_UNIX;
    strcpy(sock_unix.sun_path, socket_path);

    if((sock_un_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) perror("socket");

    if(bind(sock_un_fd, (struct sockaddr*) &sock_unix, sizeof(sock_unix)) < 0) perror("bind");

    if(listen(sock_un_fd, MAX_PLAYERS) < 0) perror("listen");

    printf("UNIX socket slucha na %s\n", socket_path);

    struct hostent* host_entry = gethostbyname("localhost");
    struct in_addr host_address = *(struct in_addr*) host_entry->h_addr;

    sock_inet.sin_family = AF_INET;
    sock_inet.sin_port = htons(port_number);
    sock_inet.sin_addr.s_addr = host_address.s_addr;

    if((sock_in_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror("socket");

    if(bind(sock_in_fd, (struct sockaddr*) &sock_inet, sizeof(sock_inet)) < 0) perror("bind");

    if(listen(sock_in_fd, MAX_PLAYERS) < 0) perror("listen");

    printf("INET socket slucha na %s:%d\n", inet_ntoa(host_address), port_number);
}

void stop_server() {
    if(pthread_cancel(connection_tid) < 0) perror("pthread_cancel");

    if(pthread_cancel(ping_tid) < 0) perror("pthread_cancel");

    if(shutdown(sock_un_fd, SHUT_RDWR) < 0) perror("shutdown");

    if(close(sock_un_fd) < 0) perror("close");

    if(unlink(socket_path) < 0) perror("unlink");

    if(shutdown(sock_in_fd, SHUT_RDWR) < 0) perror("shutdown");

    if(close(sock_in_fd) < 0) perror("close");

    exit(0);
}

void close_connection(int fd) {
    if(shutdown(fd, SHUT_RDWR) < 0) perror("shutdown");
    if(close(fd) < 0) perror("close");
}

int register_player(int fd, char* name) {
    int free_index = -1;
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i] != NULL && strcmp(players[i]->name, name) == 0) return -1;
        if(players[i] == NULL && free_index == -1) free_index = i;
    }
    if(free_index == -1) return -1;
    players[free_index] = create_player(fd, name);
    return free_index;
}

void unregister_player(int fd) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i] && players[i]->fd == fd) {
            players[i] = NULL;
        }
    }
}

int log_player_in(int sock_fd) {
    printf("Nowe logowanie...\n");

    int player_sock_fd;
    if((player_sock_fd = accept(sock_fd, NULL, NULL)) < 0) perror("accept");

    message* msg = read_message(player_sock_fd);
    printf("Otrzymalem nazwe %s\n", msg->data);

    int registered_index = register_player(player_sock_fd, msg->data);
    if(registered_index < 0) {
        printf("Login odrzucony...\n");
        send_message(player_sock_fd, LOGIN_REJECTED, "name_exists");
        close_connection(player_sock_fd);
    } else {
        printf("Login zaakceptowany...\n");
        send_message(player_sock_fd, LOGIN_APPROVED, NULL);
    }
    return registered_index;
}

int add_game(int player1, int player2) {
    for(int i = 0; i < MAX_PLAYERS / 2; i++) {
        if(games[i] == NULL) {
            games[i] = create_new_game(player1, player2);
            return i;
        }
    }
    return -1;
}

void remove_game(int index) {
    games[index] = NULL;
}

void make_match(int registered_index) {
    if(waiting_player < 0) {
        printf("Nie ma zadnego czekajacego gracza\n");
        send_message(players[registered_index]->fd, GAME_WAITING, NULL);
        waiting_player = registered_index;
    } else {
        printf("Jest czekajacy gracz %d\n", waiting_player);
        int game_index = add_game(registered_index, waiting_player);
        player_games[registered_index] = game_index;
        player_games[waiting_player] = game_index;

        if((rand() % 2) == 0) {
            send_message(players[registered_index]->fd, GAME_FOUND, "X");
            send_message(players[waiting_player]->fd, GAME_FOUND, "O");
            player_marks[registered_index] = X;
            player_marks[waiting_player] = O;
        } else {
            send_message(players[registered_index]->fd, GAME_FOUND, "O");
            send_message(players[waiting_player]->fd, GAME_FOUND, "X");
            player_marks[registered_index] = O;
            player_marks[waiting_player] = X;
        }
        waiting_player = -1;
    }
}

void* connection_thread() {
    pollfd fds[MAX_PLAYERS + 2];

    fds[MAX_PLAYERS].fd = sock_un_fd;
    fds[MAX_PLAYERS].events = POLLIN;

    fds[MAX_PLAYERS + 1].fd = sock_in_fd;
    fds[MAX_PLAYERS + 1].events = POLLIN;

    waiting_player = -1;

    while(1) {
        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < MAX_PLAYERS; i++) {
            fds[i].fd = players[i] != NULL ? players[i]->fd : -1;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }

        pthread_mutex_unlock(&players_mutex);

        fds[MAX_PLAYERS].revents = 0;
        fds[MAX_PLAYERS + 1].revents = 0;

        poll(fds, MAX_PLAYERS + 2, -1);

        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < MAX_PLAYERS + 2; i++) {
            if(i < MAX_PLAYERS && players[i] == NULL) continue;

            if(fds[i].revents & POLLHUP) {
                close_connection(fds[i].fd);
                unregister_player(fds[i].fd);
            } else if(fds[i].revents & POLLIN) {
                if(fds[i].fd == sock_un_fd || fds[i].fd == sock_in_fd) {
                    int registered_index = log_player_in(fds[i].fd);
                    printf("Gracz zarejestrowany na indeksie %d\n", registered_index);
                    if(registered_index >= 0) make_match(registered_index);
                } else {
                    printf("Otrzymalem wiadomosc od gracza\n");
                    message* msg = read_message(fds[i].fd);
                    if(msg->type == GAME_MOVE) {
                        printf("Wykonany ruch: %s\n", msg->data);
                        game* g = games[player_games[i]];
                        int idx = atoi(msg->data);
                        FIELD mark = player_marks[i];
                        make_move(g, idx, mark);
                        int other = g->player1_idx == i ? g->player2_idx : g->player1_idx;

                        GAME_STATUS status = check_game_status(g);
                        if(status == PLAYING) {
                            send_message(fds[other].fd, GAME_MOVE, parse_board(g));
                        } else {
                            send_message(fds[i].fd, GAME_FINISHED, "finished");
                            send_message(fds[other].fd, GAME_FINISHED, "finished");
                        }
                    } else if(msg->type == PING) {
                        players[i]->alive = 1;
                    } else if(msg->type == LOGOUT) {
                        close_connection(fds[i].fd);
                        unregister_player(fds[i].fd);
                    }
                }
            }
        }

        pthread_mutex_unlock(&players_mutex);
    }

    return NULL;
}

void* ping_thread() {
    while(1) {
        sleep(PING_INTERVAL);

        printf("Pingowanie graczy...\n");

        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < MAX_PLAYERS; i++) {
            if(players[i] != NULL) {
                players[i]->alive = 0;
                send_message(players[i]->fd, PING, NULL);
            }
        }

        pthread_mutex_unlock(&players_mutex);

        printf("Czekanie na ping zwrotny...\n");

        sleep(PING_TIMEOUT);

        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < MAX_PLAYERS; i++) {
            if(players[i] != NULL && players[i]->alive == 0) {
                printf("Gracz %d nie odpowiedzial na ping, rozlaczanie...\n", i);
                close_connection(players[i]->fd);
                unregister_player(players[i]->fd);
            }
        }

        pthread_mutex_unlock(&players_mutex);
    }


    return NULL;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    if(argc < 3){
        printf("Usage: %s port path", argv[0]);
        exit(EXIT_FAILURE);
    }

    port_number = atoi(argv[1]);
    socket_path = argv[2];

    signal(SIGINT, stop_server);

    start_server();

    if(pthread_create(&connection_tid, NULL, connection_thread, NULL) < 0) perror("pthread_create");
    if(pthread_create(&ping_tid, NULL, ping_thread, NULL) < 0) perror("pthread_create");

    if(pthread_join(connection_tid, NULL) < 0) perror("pthread_join");
    if(pthread_join(ping_tid, NULL) < 0) perror("pthread_join");

    stop_server();

    return 0;
}