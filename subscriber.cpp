#include <iostream>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cctype>

#include "./structures.h"

using namespace std;

int main(int argc, char *argv[]) {

    int sockfd, size_id;
    struct sockaddr_in server_addr;
    char buffer[MAX_UDP_MSG_SIZE], client_id[MAX_CLIENT_ID];

    memset(client_id, 0, MAX_CLIENT_ID);

    int port = stoi(argv[3]);
    DIE (port == 0, "bad port\n");

    int rc, nagle = 1;

    DIE(argc != 4, "not enough params\n");
    // Disable buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // get tcp socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "bad socket");
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    rc = inet_aton(argv[2], &server_addr.sin_addr);
    DIE(rc == 0, "inet_aton");

    // send connection to server
    rc = connect(sockfd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    DIE(rc < 0, "connect fail");
    
    // disable nagles alg
    rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(int));
    DIE(rc < 0, "nagle");

    // get client id
    memcpy(client_id, argv[1], sizeof(argv[1]));
    size_id = strlen(client_id);

    // set file descriptors
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(0, &read_fds);

    fd_set temp_fds;
    FD_ZERO(&temp_fds);
    FD_SET(sockfd, &read_fds);

    int max_fd = sockfd;

    struct tcp_msg client_tcp_msg;
    memset(&client_tcp_msg, 0, sizeof(struct tcp_msg));
    client_tcp_msg.type = 0;
    memcpy(&client_tcp_msg.data, client_id, MAX_CLIENT_ID);
    rc = (int) send(sockfd, &client_tcp_msg, sizeof(tcp_msg), 0);
    

    while(true) {
        temp_fds = read_fds;

        rc = select(max_fd + 1, &temp_fds, NULL, NULL, NULL);
        DIE(rc < 0, "select");

        if (FD_ISSET(STDIN_FILENO, &temp_fds)) {
            cin.getline(buffer, MAX_UDP_MSG_SIZE);
            char *token = strtok(buffer, " ");
            char *arg[25];
            int i = 0;

            while (token != NULL) {
                arg[i] = token;
                i++;
                token = strtok(NULL, " ");
            }
            DIE(i > 2, "too many arguments");

            if (strcmp(arg[0], "exit") == 0) {
                client_tcp_msg.type = '0' + EXIT;
                break;
            } else if (strcmp(arg[0], "subscribe") == 0) {
                client_tcp_msg.type = '0' + SUBSCRIBE;
                strcpy(client_tcp_msg.data, arg[1]);
                cout << "Subscribed to topic.\n";
            } else if (strcmp(arg[0], "unsubscribe") == 0) {
                client_tcp_msg.type = '0' + UNSUBSCRIBE;
                strcpy(client_tcp_msg.data, arg[1]);
                cout << "Unsubscribed from topic.\n";
            }
            uint32_t size = sizeof(client_tcp_msg);
            rc = send(sockfd, &size, sizeof(uint32_t), 0);
            rc = send(sockfd, &client_tcp_msg, size, 0);
        } else { // handle messages coming from the server
            memset(buffer, 0, MAX_UDP_MSG_SIZE);
            uint32_t size;
            rc = recv(sockfd, &size, sizeof(uint32_t), 0);
            rc = recv(sockfd, buffer, size, 0);

            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }
            cout << buffer << "\n";
        }
    }
    close(sockfd);
    return 0;
}
