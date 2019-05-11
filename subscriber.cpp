#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"
#include <iostream>
#include <string>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

// verifica daca exista exit in string
bool exit(char* buffer) {
	if (strncmp(buffer, "exit", 4) == 0) {
		return true;
	}
	return false;
}

// intoarce numarul de token-uri despartite prin " "
int nrOfTokens(char* buffer) {
	char* tok = (char*) malloc(BUFLEN * sizeof(char));
	char* s = (char*) malloc(BUFLEN * sizeof(char));
	strcpy(s, buffer);
	
	tok = strtok(s, " ");
	int nrArguments = 0;
	
	while(tok != NULL) {
		nrArguments++;
	   	tok = strtok(NULL, " ");
	}
	return nrArguments;
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	// multimea de citire folosita in select()
	fd_set read_fds;
	fd_set tmp_fds;
	
	// valoare maxima file descriptor din multimea read_fds
	int fdmax;

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	
	if (argc < 4) {
		usage(argv[0]);
	}

	// socket tcp
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	
	DIE(sockfd < 0, "socket");
	
	// IPv4
	serv_addr.sin_family = AF_INET;
	
	// portul dat ca argument
	serv_addr.sin_port = htons(atoi(argv[3])); 
	
	// adresa ip data ca argument
	ret = inet_aton(argv[2], &serv_addr.sin_addr); 
	DIE(ret == 0, "inet_aton");

	// dezactivare algoritm Neagle
	int a = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &a, sizeof(int)); 
	DIE(ret < 0, "TCP Socket Options Error");

	// conectare la server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)); 
	DIE(ret < 0, "connect");
	
	FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
	fdmax = sockfd;


	// imediat dupa conectare trimitem numele, iar serverul se ocupa de corelarea numelui cu socketul
	// prin care s-au conectat
	memset(buffer, 0, BUFLEN);
	memcpy(buffer, argv[1], strlen(argv[1]));

	ret = send(sockfd, buffer, strlen(buffer), 0);
	DIE(ret < 0, "send");

	while (1) {
  		tmp_fds = read_fds;
  		
  		// se selecteaza multimea descriptorilor de citire
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
  		
  		for (int i = 0; i <= fdmax; ++i) {
  			if (FD_ISSET(i, &tmp_fds)) {
  				// daca este activ un socket si nu este stdin(0)
  				if (i != 0) {
  					memset(buffer, 0, BUFLEN);
  					int received;
  					// facem recv
  					if ((received = recv(i, buffer, sizeof(buffer), 0)) < 1) {
  						// daca e 0 inseamna ca serverul a inchis conexiunea
  						if (received == 0) {
                        	std::cout << "S-a inchis conexiunea cu serverul\n";
                        } else {
                        // altfel eroare in recv
                            std::cout << "Eroare la recv";
						}
						// in ambele cazuri inchidem socketul
  						close(i);
  						// si il stergem din multimea de citire
  						FD_CLR(i, &read_fds);
  					} else {
  						// altfel afisam mesajul
  						std::string buf(buffer);
  						std::cout << buf << std::endl;
  					}
  				}  else {
  					// daca este de la stdin(0)
  					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					// verificam daca este exit si daca da, inchidem socket-ul si iesim din program
					if (exit(buffer)) {
						close(sockfd);
						return 0;
					}
					// altfel aflam nr de token-uri din stringul primit de la tastatura
					int nrTokens = nrOfTokens(buffer);
					// verificam daca acesta corespunde cu formatul de comanda potrivit spre a fi trimis
					// catre server si daca da, trimitem
					char* s = (char*) malloc (BUFLEN * sizeof(char));
					char* tok = (char*) malloc (BUFLEN * sizeof(char));
					strcpy(s, buffer);
					tok = strtok(s, " ");
					std::string subs(tok);
					if (subs == "unsubscribe" && nrTokens == 2 || subs == "subscribe" && nrTokens == 3) {
						n = send(sockfd, buffer, strlen(buffer) - 1, 0);
						DIE(n < 0, "send");
					} else {
						// altfel afisam mesajul:
						std::cout << "Comanda gresita!\n" << "Va rugam, reintroduceti comanda" << std::endl;
					}
  				}
  			}
  		}
		
	}

	close(sockfd);

	return 0;
}
