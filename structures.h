#define MAX_CLIENT_ID 11
#define MAX_CLIENTS 128
#define MAX_UDP_MSG_SIZE 1700
#define MAX_UDP_PAYLOAD_SIZE 1501
#define MAX_TCP_PAYLOAD_SIZE 1601
#define MAX_DATA_TYPE 11
#define TOPIC_LEN 50
#define SUBSCRIBE 1
#define UNSUBSCRIBE 0
#define EXIT 2
#define MAX_IP 13


#define DIE(assertion, call_description)	                    \
	do {									                    \
		if (assertion) {					                    \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
			perror(call_description);		                    \
			exit(EXIT_FAILURE);				                    \
		}									                    \
	} while(0)

struct udp_msg {
	uint32_t port;
	char ip[MAX_IP];
    char topic[TOPIC_LEN + 1];
    unsigned char type;
	char data_type[MAX_DATA_TYPE];
	char data[MAX_UDP_PAYLOAD_SIZE];
	char result[MAX_UDP_MSG_SIZE];
};

struct tcp_msg {
	char type;
	char data[MAX_TCP_PAYLOAD_SIZE];
};

struct subscribe_msg {
	int type;
	char topic[TOPIC_LEN + 1];
};

struct Client {
    int socket;
    bool active;
    std::string id;
	char topic[TOPIC_LEN];
	std::vector<std::string> subscribed_topics;
};
