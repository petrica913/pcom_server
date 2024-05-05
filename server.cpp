#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>

#include "./structures.h"

using namespace std;

int main(int argc, char *argv[]) {
    
    DIE(argc != 2, "bad arguments");

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int rc, flag;

    int port = stoi(argv[1]);
    DIE (port == 0, "port error");

    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    DIE (udp_sock < 0, "udp socket error");

    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE (tcp_sock < 0, "tcp socket error");

    struct sockaddr_in udp_addr;
    memset((char *) &udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind (udp_sock, (struct sockaddr *) &udp_addr, sizeof(udp_addr));
    DIE(rc < 0, "bind udp");

    struct sockaddr_in tcp_addr;
    memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind (tcp_sock, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr));
    DIE(rc < 0, "bind tcp");

    rc = listen(tcp_sock, MAX_CLIENTS);
    DIE (rc < 0, "listen error");

    // disable nagle's alg
    flag = 1;
    rc = setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (const void*) &flag, sizeof(int));
    DIE (rc < 0, "nagle error");
    
    fd_set read_fd, temp_fd;
    FD_ZERO(&read_fd);
    FD_SET(0, &read_fd);
    FD_ZERO(&temp_fd);
    FD_SET(0, &temp_fd);
    FD_SET(udp_sock, &read_fd);
    FD_SET(tcp_sock, &read_fd);

    int max_fd = max(udp_sock, tcp_sock);

    unordered_map<string, Client> clients;

    while (true) {
        temp_fd = read_fd;

        rc = select(max_fd + 1, &temp_fd, NULL, NULL, NULL);
        DIE(rc < 0, "select error");

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &temp_fd)) {

                if (i == STDIN_FILENO) {
                    string com;
                    cin >> com;

                    if (com == "exit") {
                        for (auto& pair : clients) {
                            close(pair.second.socket);
                        }

                        close(udp_sock);
                        close(tcp_sock);
                        return 0;
                    }
                }

                // handle the case where the messages are coming on tcp
                if (i == tcp_sock) {
                    flag = 1;
                    rc = setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (const void*) &flag, sizeof(int));
                    DIE (rc < 0, "nagle's alg");

                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int new_client = accept(tcp_sock, (struct sockaddr *) &client_addr, &client_len);
                    DIE (new_client < 0, "accept");

                    struct tcp_msg client_tcp_msg;
                    memset(&client_tcp_msg, 0, sizeof(tcp_msg));

                    rc = (int) recv(new_client, &client_tcp_msg, sizeof(tcp_msg), 0);
                    DIE(rc < 0, "recv error");

                    string client_id = client_tcp_msg.data;

                    if (clients.find(client_id) != clients.end()) {
                        if (!clients[client_id].active) {
                            // Dacă clientul este inactiv, îl marcam ca activ și actualizăm socketul asociat
                            clients[client_id].socket = new_client;
                            clients[client_id].active = true;
                        } else {
                            close(new_client);
                            cout << "Client " << client_id << " already connected.\n";
                            continue;
                        }
                    } else {
                        // Dacă clientul nu există în map, îl adăugăm cu starea activă
                        clients[client_id] = {new_client, true, client_id};
                    }


                    FD_SET(new_client, &read_fd);

                    max_fd = max(new_client, max_fd);

                    auto string_address = inet_ntoa(client_addr.sin_addr);
                    auto string_port = ntohs(client_addr.sin_port);

                    cout << "New client " << client_id << " connected from " <<
                         string_address << ":" << string_port << "\n";
                }

                // handle the case where the messages are coming on udp
                if (i == udp_sock) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    struct udp_msg udp_message;
                    
                    // Receive the UDP message
                    rc = recvfrom(udp_sock, &udp_message, sizeof(udp_message), 0, (struct sockaddr *)&client_addr, &client_len);
                    DIE(rc < 0, "recvfrom error");

                    // Process the received UDP message
                    // Here you can implement your logic to handle the received message
                }

            }
        }
    }
    close(udp_sock);
    close(tcp_sock);

    return 0;
}
