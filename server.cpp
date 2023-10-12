#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <pthread.h>
#include <list>

#include "RobustIO.h"

void * serverStuff(void* pconn);

std::list<int> connList;
std::list<std::string> chatLog;

int main(int argc, char **argv) {
	int sock, conn;
	int i;
	int rc;
	struct sockaddr address;
	socklen_t addrLength = sizeof(address);
	struct addrinfo hints;
	struct addrinfo *addr;
	char buffer[512];
	int ret;

	// Clear the address hints structure
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // IPv4/6, socket is for binding
	// Get address info for local host
	if((rc = getaddrinfo(NULL, "4321", &hints, &addr))) {
		printf("host name lookup failed: %s\n", gai_strerror(rc));
		exit(1);
	}

	// Create a socket
    sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if(sock < 0) {
		printf("Can't create socket\n");
		exit(1);
	}

	// Set the socket for address reuse, so it doesn't complain about
	// other servers on the machine.
    i = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

	// Bind the socket
    rc = bind(sock, addr->ai_addr, addr->ai_addrlen);
    if(rc < 0) {
		printf("Can't bind socket\n");
		exit(1);
	}

	// Clear up the address data
    freeaddrinfo(addr);

	// Listen for new connections, wait on up to five of them
    rc = listen(sock, 5);
    if(rc < 0) {
		printf("listen failed\n");
		exit(1);
	}

	// Listen for connections and handle clients
	while(1) {
		conn = accept(sock, (struct sockaddr*) &address, &addrLength);
		//adds the connection to the list of connections on server.
		connList.push_back(conn);

		//creates thread for each new client connecting
		pthread_t t_id;
		int *pconn = (int*)malloc(sizeof(int));
		*pconn = conn;
		ret = pthread_create(&t_id, NULL, serverStuff, pconn);
		if (ret != 0 ){
			printf("Didn't work");
		}
	}
}

//handles the server input and output
void * serverStuff(void* pconn) {
		int conn = *((int*)pconn);
		free(pconn);
		//reads first output from client which is user name.
		std::string clientName;
		clientName = RobustIO::read_string(conn);

		//builds the message for whenever a new client connects to the server
		std::ostringstream buildConnect;
		buildConnect << clientName << " has connected." << std::endl;
		std::string connectMessage = buildConnect.str();
		printf("%s has connected.\n",clientName.c_str());

		//send a message to all client when a new user has joined
		for(auto it = connList.begin(); it != connList.end(); ++it) {
						RobustIO::write_string(*it,connectMessage);
					}

		//builds the message for whenever a client disconnects from the server
		std::ostringstream buildDisconnect;
		buildDisconnect << clientName << " had disconnected." << std::endl;
		std::string disconnectMessage = buildDisconnect.str();	

		//sends the chat log history to a new client whenever it connects to the server
		for(auto it = chatLog.begin(); it != chatLog.end(); ++it)
			RobustIO::write_string(conn, *it);

		//while connection is active handle the reading and writing of server
		while (conn >= 0) {
        auto s = RobustIO::read_string(conn);
				std::ostringstream buildMessage;

				//if server receives exit it will exit out of while loop
				if(s == "exit"){
					printf("%s has disconnected.\n", clientName.c_str());
					
					//removes the connection from the list of connection
					connList.remove(conn);

					//sends message to all user that user has disconnected
					for(auto it = connList.begin(); it != connList.end(); ++it) {
						RobustIO::write_string(*it,disconnectMessage);
					}

					//send message back to client to close thread
					RobustIO::write_string(conn,s);

					//set connection value to -1 to exit while loop
					conn = -1;

				}
				else{
					//builds the message sent from the user
					buildMessage << clientName << ": " << s << std::endl;
					std::string message = buildMessage.str();

					//checks to see if chatLog has 12 message if not add message to chatLog
					if(chatLog.size() < 12)
						chatLog.push_back(message);
					//if the chatLog has 12 message then we'll remove the oldesn't message and add the new one
					else if(chatLog.size() == 12){
						chatLog.pop_front();
						chatLog.push_back(message);
					}

					printf("%s: %s\n", clientName.c_str(),s.c_str());

					//sends the message to all clients with the most recent message
					for(auto it = connList.begin(); it != connList.end(); ++it)
						RobustIO::write_string(*it,message);
				}
    }
		close(conn);
		pthread_exit(NULL);
		return NULL;
}