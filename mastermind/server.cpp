#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	// maximum client backlog
#define MAXDATASIZE 200	// maximum data size
#define CLOSE_CONNECTION "exit99"	// defined code for client connection termination notification
char THECODE[10];

typedef enum {RED,GREEN,BLUE,YELLOW,ORANGE} color;
int StartMasterMind(int client, sockaddr_storage addr_in); 

//basic message structure
struct msgstruct {
        int length;
        char* send_data;
};

//basic method for sending messages
int sendMsg(int client, char* theMsg)
{
	msgstruct message;
	message.send_data = theMsg;
	message.length = strlen(message.send_data);

	return (send(client, message.send_data, message.length, 0));
}
// ------------------------------------------------------------------
//	TODO: This should eventually be encapsulated into it's own class
//-------------------------------------------------------------------
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int tcp_connect(const char *serv, const char *host = NULL)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(host, serv, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
        	StartMasterMind(new_fd,their_addr);
        }
       close(new_fd);  // parent doesn't need this
    }

    return 0;
}
//-----------------------------------------------------------------------
//  END TODO
//-----------------------------------------------------------------------

void InitializeGame(const char* port)
{
	tcp_connect(port);
}

std::vector<color> GetInputAsColorMap(char* input)
{
	std::vector<color> theCode;
	int theNumber = atoi(input);


	for (int i = 0 ; i < 4 ; i++)
	{
		theCode.push_back(static_cast<color>(theNumber % 10));
		theNumber /= 10;
	}
	std::reverse(theCode.begin(), theCode.end());
	return theCode;
}

int StartMasterMind(int client, sockaddr_storage addr_in) 
{
	struct sockaddr_storage their_addr = addr_in;
	socklen_t addr_len;
	char buf[MAXDATASIZE];
	buf[MAXDATASIZE] = '\0';

	sendMsg(client, "Welcome to ... M-A-S-T-E-R-M-I-N-D.\nThe game has begun.\nPlease enter a guess:");

	std::vector<color> ServerCodeColorVector;

	// there are 6 colors
	std::vector<color> the_colors(5);
	for (int i = 0; i < 5; ++i)
		the_colors[i] = color(i);

	if (strcmp(THECODE, "random") == 0 || strcmp(THECODE, "Random") == 0)
	{
		// initialize random seed and shuffle all_colors
		srand(time(NULL));
		std::random_shuffle(the_colors.begin(), the_colors.end());
		
		// fill real_code with the four first colors
		std::vector<color>::iterator it = the_colors.begin();
		ServerCodeColorVector.push_back(the_colors[0]);
		ServerCodeColorVector.push_back(the_colors[1]);
		ServerCodeColorVector.push_back(the_colors[2]);
		ServerCodeColorVector.push_back(the_colors[3]);
	}
	else
	{
		ServerCodeColorVector = GetInputAsColorMap(THECODE);
	}

	for (int i = 0; i < 8; ++i) {
		std::vector<color> current_try(4);
		int black_hits = 0, white_hits = 0;
		std::vector<int> correctColorIndex;
		std::vector<int> correctColor;

		bool exclude[4] = {false};
		
		addr_len = sizeof their_addr;
		recv(client, buf, MAXDATASIZE-1, 0/*, (struct sockaddr *)&their_addr, &addr_len*/);
		current_try = GetInputAsColorMap(buf);
 
		for (int j = 0; j < 4; j++) {
			if (color(current_try[j]) == ServerCodeColorVector[j]) {
				black_hits++;
				exclude[j] = true;
				correctColorIndex.push_back(j);
			}
		}
 
		for (int j = 0; j < 4; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				if (std::find(correctColorIndex.begin(), correctColorIndex.end(), j) != correctColorIndex.end())
					break;

				if (color(current_try[j]) == ServerCodeColorVector[k] &&
					!exclude[k])
				{
						correctColor.push_back(j);
						++white_hits;
						exclude[k] = true;
						break;
				}
			}
		}		
 
		char blacks[10];
		char whites[10];
		char hintMsg[50];

		sprintf(blacks, "%d", black_hits);
		sprintf(whites, "%d", white_hits);

		strcpy(hintMsg, blacks);
		strcat(hintMsg, " black hits and ");
		strcat(hintMsg, whites);
		strcat(hintMsg, " white hits.\n");

		sendMsg(client, hintMsg);
 
		if (black_hits == 4) {
			sendMsg(client, "You're a winner!\nGame server closing connection...\n");
			sendMsg(client, CLOSE_CONNECTION);
			close(client);
			return 0;
		}

		sendMsg(client, "Please enter your next guess: ");
	}	
 
	char realCode[10] = "";
	char helper[10];
	for (int i = 0; i < 4; ++i){
		sprintf(helper, "%d", ServerCodeColorVector[i]);
		strcat(realCode, helper);
	}
 
	char codeMsg[50] = "The real code was: ";
	sendMsg(client, strcat(codeMsg, realCode));
	sendMsg(client, "\nGame server closing connection...\n");
	sendMsg(client, CLOSE_CONNECTION);

	close(client);
	return 0;
}

int main(int argc, char** argv)
{
	std::cout << "Please enter a code (#### or \"random\"): ";
	std::cin >> THECODE;

	InitializeGame(argv[1]);
	return 0;
}

