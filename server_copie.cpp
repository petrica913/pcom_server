#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <sstream>
#include <iomanip>
#include <queue>

#include "utils.h"
#include "helpers.h"

using namespace std;

// find client after fd
int find_client_idx(const vector<subscriber_t> clients, const int fd) {
    for (int idx = 0; idx < clients.size(); idx++) {
        if (clients[idx].fd == fd) {
            return idx;
        }
    }
    return -1;
}

udp_message_t
get_udp_message (struct udp_message_t udp_message, char buffer[BUFLEN]) {
    char msg[MAX_MSG];
    char help[MAXB];

    memset(udp_message.topic, 0, TOPIC_LEN + 1);
    memcpy(udp_message.topic, buffer, 50);
    memset(udp_message.type, 0, MAX_DATA_TYPE);
    memset(udp_message.payload, 0, MAX_MSG);

    // get message to convert
    memset(msg, 0, MAX_MSG);
    memcpy(msg, buffer + TOPIC_LEN + 1, MAX_MSG);

    // get message type
    uint8_t type;
    memcpy(&type, buffer + TOPIC_LEN, sizeof(uint8_t));
    float num;

    if (type == 0) {
        memcpy(udp_message.type, "INT", MAX_DATA_TYPE);
        if (msg[0] == 1) {
            strcpy(udp_message.payload, "-");
        }
        uint32_t number_32t;
        memcpy(&number_32t, msg + 1, sizeof(uint32_t));

        number_32t = ntohl(number_32t);
        memset(help, 0, MAXB);
        sprintf(help, "%d", number_32t);

        strcat(udp_message.payload, help);

    } else if (type == 1) {
        memcpy(udp_message.type, "SHORT_REAL", MAX_DATA_TYPE);

        uint16_t number_16t;
        memcpy(&number_16t, msg, sizeof(uint16_t));
        number_16t = ntohs(number_16t);
    
        num = 1.0 * number_16t / 100;
        
        // crazy conversions because i didn't knew how to work
        // with strings in c++ before this homework =)
        stringstream s;
        s << fixed << setprecision(2) << num;
        string ss = s.str();
        strcat(udp_message.payload, ss.c_str());

    } else if (type == 2) {
        memcpy(udp_message.type, "FLOAT", MAX_DATA_TYPE);

        if (msg[0] == 1) {
            strcpy(udp_message.payload, "-");
        }
        uint32_t number;
        memcpy(&number, msg + 1, sizeof(uint32_t));
        number = ntohl(number);

        uint8_t power;
        memcpy(&power, msg + 1 + sizeof(uint32_t), sizeof(uint8_t));
        int pwr = pow(10, (int)power);

        num = (float)number / pwr;
        memset(help, 0, MAXB);

        if ((int)num == num) {
            // if the float has no zecimals
            sprintf(help, "%d", (int)num);
            strcat(udp_message.payload, help);
        } else {
            stringstream s;
            s << fixed << setprecision(power) << num;
            string ss = s.str();
            strcat(udp_message.payload, ss.c_str());
        }

    } else if (type == 3) {
        memcpy(udp_message.type, "STRING", MAX_DATA_TYPE);
        strcpy(udp_message.payload, msg);
    }

    // create message
    memset(udp_message.msg, 0, BUFLEN);

    // c type opperations
    strcpy(udp_message.msg, udp_message.ip);
    strcat(udp_message.msg, ":");
    char buf[MAXB];
    memset(buf, 0, MAXB);
    sprintf(buf, "%d", udp_message.port);
    strcat(udp_message.msg, buf);
    strcat(udp_message.msg, " - ");
    strcat(udp_message.msg, udp_message.topic);
    strcat(udp_message.msg, " - ");
    strcat(udp_message.msg, udp_message.type);
    strcat(udp_message.msg, " - ");
    strcat(udp_message.msg, udp_message.payload);

    return udp_message;
}


int main(int argc, char *argv[]) {
    // sockets
    int socket_tcp, socket_udp;
    struct sockaddr_in serv_addr;
    // buffer to read data from stdin
    char buffer[BUFLEN];
    // port number
    int port = atoi(argv[1]);
    // struct to monitor multiple fds for different events
    vector<struct pollfd> pfds(MAXC);
    int nfds = 3;
    int rc;
    // vector for clients
    vector<struct subscriber_t> clients;

    if (argc != 2) {
        fprintf(stderr, "\n Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // set port
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // create TCP socket -----------------------------------------
    socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(socket_tcp < 0, "TCP socket failed");

    // deactivate Nagle algorithm
    int nagle = 1;
	rc = setsockopt(socket_tcp, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(int));
	DIE(rc < 0, "deactivating nagle algorithm failed");

    rc = bind(socket_tcp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind() failed");

    // create UDP socket ------------------------------------------
    socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(socket_udp < 0, "UDP socket failed");

    rc = bind(socket_udp, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind() failed");

    // listen for clients
    int listenfd = listen(socket_tcp, MAXC);
    DIE(rc < 0, "listen() failed");

    // read data from stdin
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;

    // add tcp socket
    pfds[1].fd = socket_tcp;
    pfds[1].events = POLLIN;

    // add udp socket
    pfds[2].fd = socket_udp;
    pfds[2].events = POLLIN;

    while (1) {
        // wait for readiness notification
        rc = poll(pfds.data(), nfds, -1);
        DIE(rc < 0, "poll() failed");

        if ((pfds[0].revents & POLLIN) != 0) {
            // read data from stdin
            memset(buffer, 0, BUFLEN);
            cin.getline(buffer, BUFLEN);

            if (strncmp(buffer, "exit", 4) == 0) {
                // disconnect each client
                while (clients.size() > 0) {
                    if (clients[0].conn) {
                        char msg[5] = "exit";
                        uint32_t size = strlen(msg);

                        rc = send(clients[0].fd, &size, sizeof(uint32_t), 0);
                        DIE(rc < 0, "send() failed");

                        rc = send(clients[0].fd, msg, size, 0);
                        DIE(rc < 0, "send() failed");
                        close(clients[0].fd);
                    }
                    clients.erase(clients.begin());
                }
                break;
            }
        }

        for (int i = 1; i < nfds; i++) {
            if ((pfds[i].revents & POLLIN) != 0) {
                if (pfds[i].fd == socket_tcp) {
                    // new client from tcp
                    struct sockaddr_in sub_addr;
                    socklen_t len = sizeof(sub_addr);

                    // accept an incoming connection from a new client
                    int new_client = accept(socket_tcp, (struct sockaddr *)&sub_addr, &len);
                    DIE(new_client < 0, "accept() failed");

                    uint32_t size;
                    rc = recv(new_client, &size, sizeof(uint32_t), 0);
                    DIE(rc < 0, "recv() failed");

                    memset(buffer, 0, BUFLEN);
                    // receive message from client (ID)
                    rc = recv(new_client, buffer, size, 0);
                    DIE(rc < 0, "recv() failed");

                    // check if another client is already connected with given ID
                    int idx;
                    for (idx = 0; idx < clients.size(); idx++) {
                        if (strcmp(clients[idx].id, buffer) == 0) {
                            break;
                        }
                    }

                    if (idx != clients.size()) {
                        // ID has been previously used
                        if (clients[idx].conn) {
                            // client with same ID is alredy connected
                            cout << "Client " << buffer << " already connected.\n";

                            char msg[5] = "exit";
                            uint32_t size = strlen(msg);

                            // send exit message to client
                            rc = send(new_client, &size, sizeof(uint32_t), 0);
                            DIE(rc < 0, "send() failed");

                            rc = send(new_client, msg, size, 0);
                            DIE(rc < 0, "send() failed");
                            
                            close(new_client);
                            break;
                        } else {
                            // client reconnects
                            clients[idx].conn = true;
                            clients[idx].fd = new_client;

                            // send messages from queue (store-and-forward)
                            while(!clients[idx].q.empty()) {
                                uint32_t size = strlen(clients[idx].q.front());
                                rc = send(new_client, &size, sizeof(uint32_t), 0);
                                rc = send(new_client, clients[idx].q.front(), size, 0);
                                clients[idx].q.pop();
                            }
                        }
                    }
                    
                    if (idx == clients.size()) {
                        // add new client in clients vector
                        struct subscriber_t newc;
                        strcpy(newc.id, buffer);
                        newc.conn = true;
                        newc.fd = new_client;
                        clients.push_back(newc);
                    }

                    // add new client fd
                    pfds[nfds].fd = new_client;
                    pfds[nfds].events = POLLIN;
                    nfds++;

                    cout << "New client " << buffer << " connected from " << inet_ntoa(sub_addr.sin_addr)
                         << ":" << ntohs(sub_addr.sin_port) << ".\n";
                } else if (pfds[i].fd == socket_udp) { 
                    // message from udp
                    struct udp_message_t udp_message;

                    memset(buffer, 0, BUFLEN);
                    struct sockaddr_in sub_addr;
                    socklen_t len = sizeof(sub_addr);

                    // receive message
                    rc = recvfrom(socket_udp, buffer, BUFLEN, 0, (struct sockaddr *) &sub_addr, &len);
                    DIE(rc < 0, "recvfrom() failed");

                    // save port and ip
                    udp_message.port = ntohs(sub_addr.sin_port);
                    strcpy(udp_message.ip, inet_ntoa(sub_addr.sin_addr));

                    // get message
                    udp_message = get_udp_message(udp_message, buffer);
                    uint32_t size = strlen(udp_message.msg);

                    // send messages to subscribers
                    for (int j = 0; j < clients.size(); j++) {
                        for (int k = 0; k < clients[j].topics.size(); k++) {
                            if (strcmp(clients[j].topics[k].topic, udp_message.topic) == 0) {
                                if (clients[j].conn) {
                                    rc = send(clients[j].fd, &size, sizeof(uint32_t), 0);
                                    DIE(rc < 0, "send() failed");

                                    rc = send(clients[j].fd, udp_message.msg, size, 0);
                                    DIE(rc < 0, "send() failed");
                                    break;
                                } else if (clients[j].topics[k].sf) {
                                    clients[j].q.push(udp_message.msg);
                                }
                            }
                        }
                    }

                } else {
                    // message from client already connected - tcp
                    struct message_t msg;
                    uint32_t size;

                    rc = recv(pfds[i].fd, &size, sizeof(uint32_t), 0);
                    DIE(rc < 0, "recv() failed");
                    rc = recv(pfds[i].fd, &msg, size, 0);
                    DIE(rc < 0, "recv() failed");

                    // get client
                    int idx = find_client_idx(clients, pfds[i].fd);

                    // no message => disconnect
                    if (rc == 0) {
                        // find client to disconnect (after fd)
                        int fdc = clients[idx].fd;
                        cout << "Client " << clients[idx].id << " disconnected.\n";

                        // check client as disconnected
                        clients[idx].conn = false;

                        // close connection
                        close(fdc);

                        // update pfds
                        pfds.erase(pfds.begin() + i);
                        nfds--;
                    } else {
                        // message from client
                        if (msg.type == SUBSCRIBE) {
                           struct topic_t t;
                           strcpy(t.topic, msg.topic);
                           t.sf = msg.sf;

                           clients[idx].topics.push_back(t);

                        } else if (msg.type == UNSUBSCRIBE) {
                            for (int t = 0; t < clients[idx].topics.size(); t++) {
                                if (strcmp(clients[idx].topics[t].topic, msg.topic) == 0) {
                                    clients[idx].topics.erase(clients[idx].topics.begin() + t);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // close sockets
    close(socket_tcp);
    close(socket_udp);

    return 0;
}