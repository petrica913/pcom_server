CFLAGS = -Wall -Wextra -g++

build: server subscriber

all: server subscriber

server: server.cpp

subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

clean:
	rm -f server subscriber *.o
