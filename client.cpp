#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <list>

#include "RobustIO.h"
using namespace std;

void * clientStuff(void* psock);

int main(int argc, char **argv) {
	struct addrinfo hints;
	struct addrinfo *addr;
	struct sockaddr_in *addrinfo;
	int rc;
	int sock;
	char buffer[512];
	int len;
    string clientName;
    int ret;

    // Clear the data structure to hold address parameters
    memset(&hints, 0, sizeof(hints));

    // TCP socket, IPv4/IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    // Get address info for local host
    rc = getaddrinfo("localhost", NULL, &hints, &addr);
    if (rc != 0) {
        // Note how we grab errors here
        printf("Hostname lookup failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    // Copy the address info, use it to create a socket
    addrinfo = (struct sockaddr_in *) addr->ai_addr;

    sock = socket(addrinfo->sin_family, addr->ai_socktype, addr->ai_protocol);
    if (sock < 0) {
        printf("Can't connect to server\n");
		exit(1);
    }

    // Make sure port is in network order
    addrinfo->sin_port = htons(4321);

    // Connect to the server using the socket and address we resolved
    rc = connect(sock, (struct sockaddr *) addrinfo, addr->ai_addrlen);
    if (rc != 0) {
        printf("Connection failed\n");
        exit(1);
    }

    // Clear the address struct
    freeaddrinfo(addr);
    
    // ask for user name and send to server
    cout << "Enter username\n";
    getline(cin, clientName);
    RobustIO::write_string(sock, clientName);

    //create a thread to handle client reading from server
    pthread_t t_id;
	int *psock = (int*)malloc(sizeof(int));
	*psock = sock;

    ret = pthread_create(&t_id, NULL, clientStuff, psock);
    if (ret != 0 ){
        printf("Didn't work");
    }
    
    //while connected send message to server
    while(rc == 0) {
        string message;
        getline(cin,message);

        if(message == "exit"){
            RobustIO::write_string(sock, message);
            rc = 1;
        }
        else{
            RobustIO::write_string(sock, message);
        }
    }

    //close the socket
	close(sock);
}

//handles the client reading from server
void * clientStuff(void* psock) {
    int sock = *((int*)psock);
    free(psock);

    while(1) {
        string output = RobustIO::read_string(sock);

        //close thread is received exit otherwise output message
        if (output == "exit")
            pthread_exit(NULL);
        else
            cout << output << endl;
    }
    return NULL;
}