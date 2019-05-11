#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <iostream>
#include <iomanip>
#include <math.h>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include "classes.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

// am facut aceasta functie pentru a evita lipirea mesajelor
void justWait() {
	for(int i = 0; i < 1000000; i++){}
}

// sterge topicul din lista de topicuri a clientului conectat la sockfd == i
void deleteTopic(std::vector<Client> &clients, std::string topicToDelete, int i) {
	for (int j = 0; j < clients.size(); ++j) {
		if (clients[j].sockfd == i) {
			for (auto it = clients[j].topics.begin(); it != clients[j].topics.end(); ++it) {
				if (topicToDelete == it->topic) {
					clients[j].topics.erase(it);
					break;
				}
			}
		}

	}
}

// verifica daca exista exit in string
bool exit(char* buffer) {
	if (strncmp(buffer, "exit", 4) == 0) {
		return true;
	}
	return false;
}

int main(int argc, char *argv[])
{
	int tcpSockfd, newsockfd, portno, udpSockfd;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;
	std::vector<Client> clients;

	// multimea de citire folosita in select()
	fd_set read_fds;

	// multime folosita temporar
	fd_set tmp_fds;

	// valoare maxima fd din multimea read_fds		
	int fdmax;

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// socketul TCP pe care vom face listen pentru a ne conecta cu clientii TCP
	tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpSockfd < 0, "socket");

	// socketul UDP pe care vom astepta mesaje de la clientii UDP
	udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");
	
	// dezactivare algoritm Neagle
	int a = 1;
	ret = setsockopt(tcpSockfd, IPPROTO_TCP, TCP_NODELAY, &a, sizeof(int));
	DIE(ret < 0, "TCP Socket Options Error");
	
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	// IPv4
	serv_addr.sin_family = AF_INET;
	
	// portul dat ca argument
	serv_addr.sin_port = htons(portno);
	
	// orice adresa
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind pe socketul TCP
	ret = bind(tcpSockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	// bind pe socketul UDP
	ret = bind(udpSockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	// listen pe socketul TCP pentru conexiuni
	ret = listen(tcpSockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga socketul TCP in multimea read_fds
	FD_SET(tcpSockfd, &read_fds);

	// se adauga stdin(0) in multimea read_fds
	FD_SET(0, &read_fds);

	// se adauga socketul UDP in multimea read_fds
	FD_SET(udpSockfd, &read_fds);

	fdmax = udpSockfd;
	
	while (1) {
		tmp_fds = read_fds; 
		
		// se selecteaza multimea descriptorilor de citire
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// socket-ul tcp pe care facem listen
				if (i == tcpSockfd) {
					
					// acceptam
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcpSockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// dezactivam Neagle pe acest socket
					a = 1;
					ret = setsockopt(tcpSockfd, IPPROTO_TCP, TCP_NODELAY, &a, sizeof(int));
					DIE(ret < 0, "TCP Socket Options Error");

					// facem recv in buffer
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, BUFLEN, 0);
					std::string name(buffer);
					
					// verificam daca acel client a mai fost conectat inainte
					bool comedAgain = false;
					for (int j = 0; j < clients.size(); j++) {
						if (clients[j].name == name) {
							comedAgain = true;
							clients[j].sockfd = newsockfd;
							clients[j].connected = true;
							// daca da, ii trimitem toate mesajele pe care le are in inbox
							for (int k = 0; k < clients[j].inbox.size(); k++) {
								send(clients[j].sockfd, clients[j].inbox[k].c_str(), clients[j].inbox[k].length(), 0);
								justWait();
							}
							
							clients[j].inbox.clear();
							break;
						}
					}
					
					// daca nu, il adaugam in vectorul de clienti
					if (!comedAgain) {
						clients.push_back(Client(name, newsockfd));
					}
					
					// afisam mesajul de conectare
					printf("New Client %s connected from %s:%d\n", buffer,
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					
					// adaugam socketul in multimea de citire
					FD_SET(newsockfd, &read_fds);
					
					// actualizam fdmax
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
				// stdin
				} else if (i == 0) {
					// copiem mesajul in buffer
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);
					
					// daca este mesaj pentru exit, inchidem socketii pe care asteptam
					// si oprim programul
					if (exit(buffer)) {		
						close(tcpSockfd);
						close(udpSockfd);
						return 0; 
					} else {
						std::cout << "Singura comanda disponibila in server este \"exit\" pentru iesire!\n";
					}
				// socket-ul de udp
				} else if (i == udpSockfd) {
					// copiem mesajul in buffer
					memset(buffer, 0 , BUFLEN);
					recvfrom(udpSockfd, buffer, BUFLEN, 0, (struct sockaddr *) &cli_addr, &clilen);

					// string cu topicul mesajului de la clientii UDP
					std::string topic(buffer);
					
					// tipul
					uint8_t tip;
					memcpy(&tip, buffer + 50, sizeof(uint8_t));

					// incepem sa construim mesajul
					std::string message = std::string(inet_ntoa(cli_addr.sin_addr)) + ":" +  std::to_string(ntohs(cli_addr.sin_port));

					// pentru INT
					if (tip == 0) {
						uint8_t sign;
						uint32_t int_nr;
						// copiem semnul din buffer
						memcpy(&sign, buffer + 51, sizeof(uint8_t));
						// copiem numarul din buffer
						memcpy(&int_nr, buffer + 52, sizeof(uint32_t));
						// transformam valoarea
						uint32_t value = ntohl(int_nr);
						// daca sign == 1 => nr negativ
						if (sign == 1) {
							// adaugam tipul si valoarea in mesaj
							message += " - " + topic + " - " + "INT" + " - " + "-" + std::to_string(value); 
						} else {
							message += " - " + topic + " - " + "INT" + " - " + std::to_string(value);
						}
					// pentru SHORT
					} else if (tip == 1) {
						uint16_t short_nr;
						// copiem numarul din buffer
						memcpy(&short_nr, buffer + 51, sizeof(uint16_t));
						// transformam valoarea
						uint16_t value = ntohs(short_nr);
						// impartim cu 100
						double dbl = value / 100.0;
						// adaugam tipul si valoarea in mesaj
						message += " - " + topic + " - " + "SHORT_REAL" + " - " + std::to_string(dbl);
					// pentru FLOAT
					} else if (tip == 2) {
						uint8_t sign;
						uint32_t float_nr;
						uint8_t putere10;
						// copiem semnul din buffer
						memcpy(&sign, buffer + 51, sizeof(uint8_t));
						// copiem numarul din buffer
						memcpy(&float_nr, buffer + 52, sizeof(uint32_t));
						// copiem puterea lui 10 cu care urmeaza sa impartim
						memcpy(&putere10, buffer + 56, sizeof(uint8_t));
						// facem transformarea
						uint32_t value = ntohl(float_nr);
						double dbl = (double) value;
						// impartim succesiv
						while (putere10) {
							dbl /= 10.0;
							putere10--;
						}
						// sign == 1 => numar negativ
						if (sign == 1) {
							// adaugam tipul si valoarea in mesaj
							message += " - " + topic + " - " + "FLOAT" + " - " + "-" + std::to_string(dbl);
						} else {
							message += " - " + topic + " - " + "FLOAT" + " - " + std::to_string(dbl);
						}
					// copiem din buffer textul si il introducem in mesaj
					} else if (tip == 3) {
						std::string txt(buffer + 51);
						message += " - " + topic + " - " + "STRING" + " - " + txt;
					}

					// pentru fiecare client
					for (int j = 0; j < clients.size(); ++j) {
						bool exist = false;
						int SFactive;
						// verificam daca topicul primit exista in lista lor de topicuri l-a care s-au abonat
						for (auto it = clients[j].topics.begin(); it != clients[j].topics.end(); it++) {
							if (it->topic == topic) {
								exist = true;
								SFactive = it -> SF;
								break;
							}
						}
						// daca exista
						if (exist) {
							// daca sunt conectati
							if (clients[j].connected) {
								// trimitem mesajul
								send(clients[j].sockfd, message.c_str(), message.length(), 0);
								// si asteptam putin
								justWait();
							// daca nu sunt conectati
							} else {
								// dar au SF activ
								if (SFactive == 1) {
									// adaugam in inbox-ul lor mesajul
									clients[j].inbox.push_back(message);
								}
							}
						}
					}
				// am primit date de la clienti
				} else {
					memset(buffer, 0, BUFLEN);

					// facem recv in buffer
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					// conexiunea s-a inchis
					if (n == 0) {
						// deconectam clientul
						for (int j = 0; j < clients.size(); ++j) {
							if (clients[j].sockfd == i) {
								// afisam mesajul de deconectare
								std::cout << "Client " << clients[j].name << " disconnected\n";
								clients[j].connected = false;
								break;
							}
						}
						// inchidem socketul acestuia
						close(i);
						
						// scoatem din read_fds acel socket 
						FD_CLR(i, &read_fds);
					// poate fi doar un mesaj de subscribe sau de unsubscribe
					} else {
						char* tok = (char*) malloc(BUFLEN * sizeof(char));
						char* s = (char *) malloc(BUFLEN * sizeof(char));
						// copiem buffer-ul intr-un altul pentru a putea aplica strtok pe acesta fara a modifica ceva
						strcpy(s , buffer);
						tok = strtok(s, " ");

						// daca este un mesaj de unsubscribe
						if (strstr(tok, "unsubscribe") != nullptr ) {
							// cream stringul ce reprezinta topicul pe care trebuie sa-l eliminam din
							// lista acestui client
							tok = strtok(NULL, " ");
							std::string topicToDelete(tok);
							// si il stergem
							deleteTopic(clients, topicToDelete, i);
						// daca nu este unsubscribe, mai poate fi doar subscribe
						} else {
							// cream stringul ce reprezinta topicul pe care trebuie sa-l adaugam
							// in lista clientului
							Topic topicToADD;
							tok = strtok(NULL, " ");
							// mesajul
							std::string topicToAdd(tok);
							topicToADD.topic = topicToAdd;
							// SF-ul topicului
							tok = strtok(NULL, " ");
							int sf = atoi(tok);
							topicToADD.SF = sf;

							// adaugam in lista acelui client noul topic
							for (int j = 0; j < clients.size(); ++j) {
								if (clients[j].sockfd == i) {
									clients[j].topics.push_back(topicToADD);
								}
							}
						}
					}
				}
			}
		}
	}
	close(tcpSockfd);
	close(udpSockfd);
	return 0;
}
