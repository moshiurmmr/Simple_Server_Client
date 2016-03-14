#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT "1212"
#define BACKLOG 10

// get the socket address (IPv4 or IPv6)
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET){
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
// client thread handle function
void *client_handle (void *);
int main1(void)
{
	int sockfd, new_fd, *new_sock; //
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage client_addr; // client's address
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints); // initialize hints with 0
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use the local IP address

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) !=0){
		fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(rv));
		return 1;
	}

	// loop through all the addrinfo linked list, servinfo and bind to the first one
	for (p = servinfo; p != NULL; p->ai_next){
		// socket to connect
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket ");
			continue;
		}
		// bind to the socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			perror("server: bind");
			continue;
		}
		// stop or get out of the loop once bind(ed) successfully to a address
		break;
	}

	// done with the server address, so free it
	free(servinfo);

	// no address is found in servinfo
	if (p == NULL){
		fprintf(stderr, "server: failed to bind \n");
		exit(1);
	}
	// if listening is failed
	if (listen(sockfd, BACKLOG) == -1){
		perror("listen");
		exit(1);
	}

	// bind is successful, now wait for incoming connections
	printf("Serve: waiting for incoming connections .... \n");
	while(1){
		sin_size = sizeof client_addr;
		new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &sin_size);
		if (new_fd == -1){
			perror("accept");
			continue;
		}
		inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);

		// create a thread for the new connection
		pthread_t new_thread;
		new_sock = malloc(1);
		*new_sock = new_fd;
		// create the therad
		if ((pthread_create(&new_thread, NULL, client_handle, (void *)new_sock)) < 0){
			perror("could not create a thread for the client");
			return 1;
		}
		printf("Handler has been assigned");

	}
	return 0;

}

// the socket handlign function
void *client_handle(void *sock_id)
{
	int sock = *(int *) sock_id;
	int n;
	char sendBuff[100], client_message[1000];
	while((n = recv(sock, client_message, 1000, 0)) > 0){
		send(sock, client_message, n, 0);
	}
	close(sock);
	if (n == 0){
		printf("The client is disconnected at socket %d. \n", sock);
	}
	else {
		perror("receive failed.\n");
	}
	return 0;
}
