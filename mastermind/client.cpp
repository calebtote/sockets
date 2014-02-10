/*
** client.cpp -- a mastermind client
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once
#define CLOSE_CONNECTION "exit99"	// defined code for client connection termination notification
bool gameStarted = false;

// basic structure for messages
struct msgstruct {
          int length;
          char* send_data;
 };		

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int getNextMsg(int sockfd, sockaddr_storage their_addr)
{
	char buf[MAXDATASIZE];
	int numbytes;

	memset(&buf[0], 0, sizeof(buf));

	//socklen_t addr_len = sizeof their_addr;
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0/*, (struct sockaddr *)&their_addr, &addr_len*/)) == -1) {
		perror("recv");
		exit(1);
	}
	//std::cout << "number of bytes: " <<  numbytes << std::endl;

	std::string test(buf);
	if (test == "exit99"){
		close(sockfd);
		return -1;
	}

	printf("%s\n",buf);
	return 0;
}

int main(int argc, char *argv[])
{
	int sockfd;

	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 3) {
	    fprintf(stderr,"usage: client hostname port\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	while(1){
		getNextMsg(sockfd, their_addr);

		char response[100];
		std::cin >> response;
		struct msgstruct message;
		message.send_data = response;
		message.length = strlen(message.send_data);

		int n = send(sockfd, response, strlen(response), 0/*, p->ai_addr, p->ai_addrlen*/);
		getNextMsg(sockfd, their_addr);
	}
}

