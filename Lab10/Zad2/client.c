#include "utils.h"

int is_connected_local;
char* name;
char* server_address;
int port_number;

int server_sock_fd;

pthread_t input_tid;
char input[2];

void open_connection() {
    if(is_connected_local) {
        struct sockaddr_un server_sock;

        server_sock.sun_family = AF_UNIX;
        strcpy(server_sock.sun_path, server_address);

        if((server_sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) perror("socket");

        struct sockaddr_un player_sock;
        player_sock.sun_family = AF_UNIX;
        sprintf(player_sock.sun_path, "%s", name);

        if(bind(server_sock_fd, (struct sockaddr*) &player_sock, sizeof(player_sock)) < 0) perror("bind");

        if(connect(server_sock_fd, (struct sockaddr*) &server_sock, sizeof(server_sock)) < 0) perror("connect");
    } else {
        struct sockaddr_in* server_sock = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));

        server_sock->sin_family = AF_INET;
        server_sock->sin_port = htons(port_number);
        server_sock->sin_addr.s_addr = inet_addr(server_address);

        if((server_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) perror("socket");

        struct sockaddr_in player_sock;
        player_sock.sin_family = AF_INET;
        player_sock.sin_port = 0;
        player_sock.sin_addr.s_addr = inet_addr(server_address);

        if(bind(server_sock_fd, (struct sockaddr*) &player_sock, sizeof(player_sock)) < 0) perror("bind");

        if(connect(server_sock_fd, (struct sockaddr*) server_sock, sizeof(struct sockaddr_in)) < 0) perror("connect");
    }
}

void close_connection() {
    send_message(server_sock_fd, LOGOUT, NULL, name);
    printf("Wylogowano z serwera\n");

    if(close(server_sock_fd) < 0) perror("close");

    exit(0);
}

void* input_thread() {
    printf("Twoj ruch: ");
    scanf("%s", input);

    return NULL;
}

void read_input() {
    input[0] = '.';
    if(pthread_create(&input_tid, NULL, input_thread, NULL) < 0) perror("pthread_create");
    message* msg;
    while(input[0] == '.') {
        msg = read_message_nonblocking(server_sock_fd);
        if(msg != NULL) {
            if(msg->type == PING) {
                printf("Zostalem spingowany\n");
                send_message(server_sock_fd, PING, NULL, name);
            }
        }
    }
}

int main(int argc, char** argv) {
    if(argc < 2){
        printf("Usage:\n%s LOCAL name addr\n%s NOLOCAL name addr port\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }


    is_connected_local = strcmp(argv[1], "LOCAL") ? 0 : 1;

    if((is_connected_local && argc < 4) || (!is_connected_local && argc < 5)){
        printf("Usage:\n%s LOCAL name addr\n%s NOLOCAL name addr port\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    name = argv[2];
    server_address = argv[3];

    if(!is_connected_local) {
        port_number = atoi(argv[4]);
    }

    signal(SIGINT, close_connection);

    open_connection();

    send_message(server_sock_fd, LOGIN_REQUEST, NULL, name);
    message* msg = read_message(server_sock_fd);

    if(msg->type == LOGIN_APPROVED) {
        printf("Zarejestrowany\n");
        while(1) {
            msg = read_message(server_sock_fd);

            if(msg->type == GAME_WAITING) {
                printf("Czekanie na gre...\n");
            } else if(msg->type == GAME_FOUND) {
                printf("Znaleziono gre. Twoj znak to %s\n", msg->data);

                if(msg->data[0] == 'X') {
                    read_input();
                    printf("Wykonywanie ruchu %s\n", input);
                    send_message(server_sock_fd, GAME_MOVE, input, name);
                }

                while(1) {
                    msg = read_message(server_sock_fd);

                    if(msg->type == GAME_MOVE) {
                        printf("Przeciwnik wykonwywal ruch:\n");
                        printf("%s", msg->data);

                        read_input();
                        printf("Wykonywanie ruchu %s\n", input);
                        send_message(server_sock_fd, GAME_MOVE, input, name);
                    } else if(msg->type == GAME_FINISHED) {
                        printf("Gra skonczona: %s\n", msg->data);
                        break;
                    } else if(msg->type == PING) {
                        printf("Zostalem spingowany\n");
                        send_message(server_sock_fd, PING, NULL, name);
                    }
                }

                break;
            } else if(msg->type == PING) {
                printf("Zostalem spingowany\n");
                send_message(server_sock_fd, PING, NULL, name);
            }

        }
    } else {
        printf("Odrzucono logowanie. Powod: %s\n", msg->data);
    }

    close_connection();

    return 0;
}