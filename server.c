#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define VOICE_BUF_SIZE 2048

typedef struct {
    struct sockaddr_in addr;
    socklen_t addrlen;
} VoiceClient;

int sockaddr_cmp(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr &&
           a->sin_port == b->sin_port;
}

void handle_new_connection(int server_fd, int client_sockets[], int max_clients) {
    int new_socket = accept(server_fd, NULL, NULL);
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = new_socket;
            printf("New TCP client connected\n");
            return;
        }
    }
    close(new_socket); 
}

void handle_client_activity(int i, int client_sockets[], int max_clients, char buffer[], int buffer_size) {
    int sd = client_sockets[i];
    int valread = read(sd, buffer, buffer_size);
    if (valread == 0) {
        close(sd);
        client_sockets[i] = 0;
        printf("TCP client disconnected\n");
        return;
    }
    buffer[valread] = '\0';
    for (int j = 0; j < max_clients; j++) {
        if (client_sockets[j] > 0 && j != i) {
            send(client_sockets[j], buffer, strlen(buffer), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    int server_fd, udp_fd, port_no, client_sockets[MAX_CLIENTS]; 
    struct sockaddr_in tcp_addr, udp_addr;
    int opt = 1, max_sd, activity;
    fd_set readfds;
    char buffer[BUFFER_SIZE] = {0};
    char voice_buffer[VOICE_BUF_SIZE];
    VoiceClient voice_clients[MAX_CLIENTS];
    int voice_client_count = 0;

    if (argc < 2) {
        fprintf(stderr, "NO PORT PROVIDED!\n");
        exit(EXIT_FAILURE);
    }

    port_no = atoi(argv[1]);

    for (int i = 0; i < MAX_CLIENTS; i++) client_sockets[i] = 0;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("TCP Socket failed!");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed!");
        exit(EXIT_FAILURE);
    }

    struct linger sl = { .l_onoff = 1, .l_linger = 5 };
	
    if (setsockopt(server_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) < 0) {
        perror("setsockopt(SO_LINGER)");
        exit(EXIT_FAILURE);
    }

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(port_no);

    if (bind(server_fd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP Bind failed!");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed!");
        exit(EXIT_FAILURE);
    }

    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket");
        exit(EXIT_FAILURE);
    }

    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port_no); 
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind");
        close(udp_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d (TCP + UDP)...\n", port_no);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds); 
        FD_SET(udp_fd, &readfds);    
        max_sd = (server_fd > udp_fd) ? server_fd : udp_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
                if (sd > max_sd) max_sd = sd;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select error");
            continue;
        }

        //tcp
        if (FD_ISSET(server_fd, &readfds)) {
            handle_new_connection(server_fd, client_sockets, MAX_CLIENTS);
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0 && FD_ISSET(client_sockets[i], &readfds)) {
                handle_client_activity(i, client_sockets, MAX_CLIENTS, buffer, BUFFER_SIZE);
            }
        }

        //udp
        if (FD_ISSET(udp_fd, &readfds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            ssize_t recvlen = recvfrom(udp_fd, voice_buffer, sizeof(voice_buffer), 0,
                                       (struct sockaddr*)&cli_addr, &cli_len);
            if (recvlen > 0) {
                // Check for known client
                int known = 0;
                for (int i = 0; i < voice_client_count; ++i) {
                    if (sockaddr_cmp(&voice_clients[i].addr, &cli_addr)) {
                        known = 1;
                        break;
                    }
                }
                if (!known && voice_client_count < MAX_CLIENTS) {
                    voice_clients[voice_client_count].addr = cli_addr;
                    voice_clients[voice_client_count].addrlen = cli_len;
                    ++voice_client_count;
                    printf("New voice client: %s:%d\n",
                        inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                }

                // Relay voice to other clients
                for (int i = 0; i < voice_client_count; ++i) {
                    if (!sockaddr_cmp(&voice_clients[i].addr, &cli_addr)) {
                        sendto(udp_fd, voice_buffer, recvlen, 0,
                               (struct sockaddr*)&voice_clients[i].addr,
                               voice_clients[i].addrlen);
                    }
                }
            }
        }
    }

    close(server_fd);
    close(udp_fd);
    return 0;
}
