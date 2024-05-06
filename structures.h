#define MAX_CLIENT_ID 11
#define MAX_CLIENTS 128
#define MAX_UDP_MSG_SIZE 1552
#define MAX_TOPIC_NAME_LENGTH 51
#define MAX_UDP_PAYLOAD_SIZE 1500
#define MAX_TCP_PAYLOAD_SIZE 1601
#define MAX_DATA_TYPE 11
#define TOPIC_LEN 50
#define SF_ENABLED 1
#define SF_DISABLED 0
#define SUBSCRIBE 1
#define UNSUBSCRIBE 0
#define EXIT 2
#define MAX_IP 13


/**
 * Error checking macro.
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	                    \
	do {									                    \
		if (assertion) {					                    \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
			perror(call_description);		                    \
			exit(EXIT_FAILURE);				                    \
		}									                    \
	} while(0)

/**
 * Structure representation for a udp message.
 */
struct udp_msg {
	uint32_t port;
	char ip[MAX_IP];
    char topic[MAX_TOPIC_NAME_LENGTH];
    unsigned char type;
	char data_type[MAX_DATA_TYPE];
	char data[MAX_UDP_PAYLOAD_SIZE];
	char result[MAX_UDP_MSG_SIZE];
};

/**
 * Structure representation for a tcp message.
 */
struct tcp_msg {
	char type;
	char data[MAX_TCP_PAYLOAD_SIZE];
};

/**
 * Structure representation for a subscribe/unsubscribe message
 * sent by the tcp clients.
 */
struct subscribe_msg {
	int type;
	char topic[MAX_TOPIC_NAME_LENGTH];
	int wildcard; // 0 for nothing, 1 for *, 2 for +
};

struct Client {
    int socket;
    bool active;
    std::string id;  // Adăugăm id-ul clientului în structură
	char topic[TOPIC_LEN];
	std::vector<std::string> subscribed_topics;

};
