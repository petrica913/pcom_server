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
#include <regex>
#include <set>

#include "./structures.h"
#include <cmath>

using namespace std;

bool matchTopic(const std::string& topic, const std::string& subscription) {
    std::string regexStr = subscription;
    
    regexStr = std::regex_replace(regexStr, std::regex("\\*"), ".*");
    regexStr = std::regex_replace(regexStr, std::regex("\\+"), "[^/]+");
    std::regex regexPattern(regexStr);

    return std::regex_match(topic, regexPattern);
}


void handle_stdin_input(const unordered_map<string, Client>& clients, int tcp_sock, int udp_sock) {
    string com;
    cin >> com;

    if (com == "exit") {
        for (auto& pair : clients) {
            uint32_t size = 4;
            int rc = send(pair.second.socket, &size, sizeof(uint32_t), 0);
            rc = send(pair.second.socket, "exit", size, 0);

            close(pair.second.socket);
        }

        close(udp_sock);
        close(tcp_sock);
        exit(0);
    }
}

void handle_tcp_client_connection(int tcp_sock, unordered_map<string, Client>& clients, fd_set& read_fd, int& max_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int new_client = accept(tcp_sock, (struct sockaddr *) &client_addr, &client_len);
    DIE(new_client < 0, "accept");

    struct tcp_msg client_tcp_msg;
    memset(&client_tcp_msg, 0, sizeof(tcp_msg));

    int rc = (int) recv(new_client, &client_tcp_msg, sizeof(tcp_msg), 0);
    DIE(rc < 0, "recv error");

    string client_id = client_tcp_msg.data;

    if (clients.find(client_id) != clients.end()) {
        if (!clients[client_id].active) {
            clients[client_id].socket = new_client;
            clients[client_id].active = true;
        } else {
            char msg[5] = "exit";
            uint32_t size = strlen(msg);

            // send exit message to client
            rc = send(new_client, &size, sizeof(uint32_t), 0);
            DIE(rc < 0, "send() failed");

            rc = send(new_client, msg, size, 0);
            DIE(rc < 0, "send() failed");
            FD_CLR(new_client, &read_fd);
            close(new_client);
            cout << "Client " << client_id << " already connected.\n";
            return;
        }
    } else {
        clients[client_id] = {new_client, true, client_id, "", vector<string>()};
    }

    FD_SET(new_client, &read_fd);
    max_fd = max(new_client, max_fd);

    auto string_address = inet_ntoa(client_addr.sin_addr);
    auto string_port = ntohs(client_addr.sin_port);

    cout << "New client " << client_id << " connected from " <<
         string_address << ":" << string_port << "\n";
}

struct udp_msg handle_udp_messages(int udp_sock) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct udp_msg udp_message;
    char buffer[MAX_UDP_MSG_SIZE];
    
    int rc = recvfrom(udp_sock, buffer, MAX_UDP_MSG_SIZE, 0, (struct sockaddr *) &client_addr, &client_len);
    DIE (rc < 0, "recvfrom handle_udp error\n");

    memset(udp_message.topic, 0, TOPIC_LEN + 1);
    memcpy(udp_message.topic, buffer, 50); // VEZI aici cum gestionezi topicul cu wildcarduri
    memset(udp_message.data, 0, MAX_UDP_PAYLOAD_SIZE + 1);

    char msg[MAX_UDP_PAYLOAD_SIZE + 1];
    memset(msg, 0, MAX_UDP_PAYLOAD_SIZE + 1);
    memcpy(msg, buffer + TOPIC_LEN + 1, MAX_UDP_PAYLOAD_SIZE);

    uint8_t type;
    memcpy(&type, buffer + TOPIC_LEN, sizeof(uint8_t));

    if (type == 0) {
        udp_message.type = 0; // TIP INT
        memcpy(udp_message.data_type, "INT", MAX_DATA_TYPE);
        uint32_t number;
        memcpy(&number, msg + 1, sizeof(uint32_t));
        number = ntohl(number);
        if (msg[0] == 1 && number != 0) { // daca avem un octet de semn 1 atunci avem un numar negativ
            strcpy(udp_message.data, "-");
            strcat(udp_message.data, to_string(number).c_str());
        } else {
            strcpy(udp_message.data, to_string(number).c_str());
        }
    }
    if (type == 1) { //SHORT_REAL
        udp_message.type = 1;
        memcpy(udp_message.data_type, "SHORT_REAL", MAX_DATA_TYPE);
        float number = abs (ntohs(*(uint16_t *)msg));
        snprintf(udp_message.data, sizeof(udp_message.data), "%.2f", number / 100);
    }
    if (type == 2) { // FLOAT
        udp_message.type = 2;
        memcpy(udp_message.data_type, "FLOAT", MAX_DATA_TYPE);
        char sign = msg[0]; 
        uint32_t int_part_network;
        memcpy(&int_part_network, msg + 1, sizeof(uint32_t));
        uint32_t int_part = ntohl(int_part_network);
        uint8_t power;
        memcpy(&power, msg + 1 + sizeof(uint32_t), sizeof(uint8_t));
        
        double real_number = int_part * std::pow(10, -power);

        if (sign == 1) {
            real_number = -real_number;
        }
        snprintf(udp_message.data, 1500, "%.*f", power, real_number);
    }

    if (type == 3) {
        udp_message.type = 3;
        memcpy(udp_message.data_type, "STRING", MAX_DATA_TYPE);
        strncpy(udp_message.data, msg, 1500);
    }

    memset(udp_message.result, 0, MAX_UDP_MSG_SIZE);
    udp_message.port = ntohs(client_addr.sin_port);
    sprintf(udp_message.result, "%s:%d - %s - %s - %s"
            , inet_ntoa(client_addr.sin_addr), udp_message.port, udp_message.topic
            , udp_message.data_type, udp_message.data);

    return udp_message;
}

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
                    handle_stdin_input(clients, tcp_sock, udp_sock);
                    continue;
                }

                else if (i == tcp_sock) {
                    handle_tcp_client_connection(tcp_sock, clients, read_fd, max_fd);
                    // continue;
                }

                else if (i == udp_sock) {
                    struct udp_msg udp_message;
                    // udp_message.port = ntohs()
                    udp_message = handle_udp_messages(udp_sock);
                    std::set<std::string> sent_topics;
                    // uint32_t size = strlen(udp_message.)
                    for (auto client : clients) {
                        if (client.second.active) {
                            for (auto &topic : client.second.subscribed_topics) {
                                if (matchTopic(udp_message.topic, topic)) {
                                    if (sent_topics.find(udp_message.topic) == sent_topics.end()) {
                                        sent_topics.insert(udp_message.topic);
                                        uint32_t size = strlen(udp_message.result);
                                        int bytes_sent = send(client.second.socket, &size, sizeof(uint32_t), 0);
                                        DIE (rc < 0, "send error in main");
                                        bytes_sent = send(client.second.socket, &udp_message.result, size, 0);
                                        DIE (rc < 0, "send error in main");
                                    }
                                }
                            }
                        }
                    }
                    // continue;
                } else {
                    // handle message from client
                    struct tcp_msg msg;
                    uint32_t size;
                    rc = recv(i, &size, sizeof(uint32_t), 0);
                    rc = recv(i, &msg, size, 0);
                    
                    int type = atoi(&msg.type);

                    string client_id;
                    for (auto &pair : clients) {
                        if (pair.second.socket == i) {
                            client_id = pair.second.id;
                            break;
                        }
                    }
                    if (rc == 0) {
                        clients[client_id].active = false;
                        clients[client_id].socket = -1;
                        cout << "Client " << client_id << " disconnected.\n";
                        FD_CLR(i, &read_fd);
                        FD_CLR(i, &temp_fd);
                        close(i);
                    } else {
                        if (type == SUBSCRIBE) {
                            clients[client_id].subscribed_topics.push_back(msg.data);
                        } else if (type == UNSUBSCRIBE) {
                            auto& subscribed_topics = clients[client_id].subscribed_topics;
                            for (auto it = subscribed_topics.begin(); it != subscribed_topics.end(); ++it) {
                                if (it->compare(msg.data) == 0) {
                                    subscribed_topics.erase(it);
                                    break;
                                }
                            }

                        }
                    }
                }
            }
        }
    }
    close(udp_sock);
    close(tcp_sock);

    return 0;
}
