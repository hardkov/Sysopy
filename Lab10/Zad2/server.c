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

    if((sock_un_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) perror("socket");

    if(bind(sock_un_fd, (struct sockaddr*) &sock_unix, sizeof(sock_unix)) < 0) perror("bind");

    printf("UNIX socket slucha na %s\n", socket_path);

    struct hostent* host_entry = gethostbyname("localhost");
    struct in_addr host_address = *(struct in_addr*) host_entry->h_addr;

    sock_inet.sin_family = AF_INET;
    sock_inet.sin_port = htons(port_number);
    sock_inet.sin_addr.s_addr = host_address.s_addr;

    if((sock_in_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) perror("socket");

    if(bind(sock_in_fd, (struct sockaddr*) &sock_inet, sizeof(sock_inet)) < 0) perror("bind");

    printf("INET socket slucha na %s:%d\n", inet_ntoa(host_address), port_number);
}

void stop_server() {
    exit(0);
}

int register_player(int fd, struct sockaddr* addr, char* name) {
    int free_index = -1;
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i] != NULL && strcmp(players[i]->name, name) == 0) return -1;
        if(players[i] == NULL && free_index == -1) free_index = i;
    }
    if(free_index == -1) return -1;
    players[free_index] = create_player(fd, addr, name);
    return free_index;
}

void unregister_player(char* name) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i] && strcmp(players[i]->name, name) == 0) {
            players[i] = NULL;
        }
    }
}

int get_user_index(char* name) {
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i] != NULL && strcmp(players[i]->name, name) == 0) return i;
    }
    return -1;
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
        send_message_to(players[registered_index]->fd, GAME_WAITING, NULL, players[registered_index]->addr);
        waiting_player = registered_index;
    } else {
        printf("Jest czekajacy gracz %d\n", waiting_player);
        int game_index = add_game(registered_index, waiting_player);
        player_games[registered_index] = game_index;
        player_games[waiting_player] = game_index;
        if((rand() % 2) == 0) {
            send_message_to(players[registered_index]->fd, GAME_FOUND, "X", players[registered_index]->addr);
            send_message_to(players[waiting_player]->fd, GAME_FOUND, "O", players[waiting_player]->addr);
            player_marks[registered_index] = X;
            player_marks[waiting_player] = O;
        } else {
            send_message_to(players[registered_index]->fd, GAME_FOUND, "O", players[registered_index]->addr);
            send_message_to(players[waiting_player]->fd, GAME_FOUND, "X", players[waiting_player]->addr);
            player_marks[registered_index] = O;
            player_marks[waiting_player] = X;
        }
        waiting_player = -1;
    }
}

void* connection_thread() {
    pollfd fds[2];

    fds[0].fd = sock_un_fd;
    fds[0].events = POLLIN;

    fds[1].fd = sock_in_fd;
    fds[1].events = POLLIN;

    waiting_player = -1;

    while(1) {
        for(int i = 0; i < 2; i++) {
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }

        poll(fds, 2, -1);

        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < 2; i++) {
            if(fds[i].revents & POLLIN) {
                printf("Otrzymałem wiadomosc\n");
                struct sockaddr* addr = (struct sockaddr*) malloc(sizeof(struct sockaddr));
                socklen_t len = sizeof(&addr);
                message* msg = read_message_from(fds[i].fd, addr, &len);

                if(msg->type == LOGIN_REQUEST) {
                    printf("Rejestrowanie uzytkownika o nazwie %s\n", msg->user);
                    int registered_index = register_player(fds[i].fd, addr, msg->user);
                    printf("Gracz zarejestrowany na indeksie %d\n", registered_index);
                    if(registered_index < 0) {
                        send_message_to(fds[i].fd, LOGIN_REJECTED, "user_exists", addr);
                    } else {
                        send_message_to(fds[i].fd, LOGIN_APPROVED, NULL, addr);
                        make_match(registered_index);
                    }
                } else if(msg->type == LOGOUT) {
                    unregister_player(msg->user);
                    printf("Uzytkownik %s wylogowywuje sie\n", msg->user);
                } else if(msg->type == GAME_MOVE) {
                    int index = get_user_index(msg->user);
                    printf("Wykonano ruch: %s\n", msg->data);
                    game* g = games[player_games[index]];
                    int idx = atoi(msg->data);
                    FIELD mark = player_marks[index];
                    make_move(g, idx, mark);
                    int other = g->player1_idx == index ? g->player2_idx : g->player1_idx;

                    GAME_STATUS status = check_game_status(g);
                    if(status == PLAYING) {
                        send_message_to(players[other]->fd, GAME_MOVE, parse_board(g), players[other]->addr);
                    } else {
                        send_message_to(players[other]->fd, GAME_FINISHED, "finished", players[other]->addr);
                        send_message_to(players[index]->fd, GAME_FINISHED, "finished", players[index]->addr);
                    }
                } else if(msg->type == PING) {
                    players[get_user_index(msg->user)]->alive = 1;
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
                send_message_to(players[i]->fd, PING, NULL, players[i]->addr);
            }
        }

        pthread_mutex_unlock(&players_mutex);

        printf("Czekanie na odpowiedzi...\n");

        sleep(PING_TIMEOUT);

        pthread_mutex_lock(&players_mutex);

        for(int i = 0; i < MAX_PLAYERS; i++) {
            if(players[i] != NULL && players[i]->alive == 0) {
                printf("Gracz %d nie odpowiedzial na ping, rozlaczanie...\n", i);
                unregister_player(players[i]->name);
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