# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g -std=c++11

# Portul pe care asculta serverul (de completat)
PORT = 10234

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

#Client ID
ClientID = "Andrei"
all: server subscriber

# Compileaza server.c
server: server.cpp

# Compileaza client.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ClientID} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
