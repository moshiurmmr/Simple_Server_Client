#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT "1212"
#define MAXDATASIZE 100 // maximum data that can be sent at once
#define NUM_THREADS 5

//get the socket address, either IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void *ConnectToServer(void *t)
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	char s[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	// prepare the hints addrinfo struct with necessary information
	memset(&hints, 0, sizeof(hints)); // initialize hints to 0
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through the sever address and connect to the first one
	for (p = servinfo; p!=NULL; p->ai_next){
		// create a socket
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			perror("Client: socket");
			continue;
		}
		// connect to the socket
		if((connect(sockfd, p->ai_addr, p->ai_addrlen)) == -1){
			close(sockfd);
			perror("Client: connect");
			continue;
		}
		// we have got a connected socket
		break;
	}
	if (p == NULL){
		printf(stderr, "Client: failed to connect. \n");
		return 2;
	}
	inet_ntop(p->ai_family, get_in_addr((struct addrinfo *)p->ai_addr),
			s, sizeof s);
	printf("Client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // done with this structure

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1){
		perror("receive");
		exit(1);
	}
	buf[numbytes] = "\0"; // end of message
	printf("Client: received %s \n", buf);

	close(sockfd);

	return 0;

}

int main(void)
{
	pthread_t client[NUM_THREADS];
	pthread_attr_t attr;
	int rc;
	long t;
	void *status;

	// initalize and set thread detached attribute
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// Now spawn client threads that connecto to the server
	for(t=0; t<NUM_THREADS; t++){
		printf("Creating client#%ld \n", t);
		rc = pthread_create(&client[t], &attr, ConnectToServer, (void *)t);
		if (rc){
			printf("Error, return code from client's thread is %d\n", rc);
			exit(-1);
		}
	}
	// free the attribute and wait for all the client threads to complete
	pthread_attr_destroy(&attr);
	for(t=0; t<NUM_THREADS; t++){
		rc = pthread_join(client[t], &status);
		if (rc){
			printf("Error, return from pthread_join() is %d/n", rc);
			exit(-1);
		}
		printf("The client# %ld has been joined with status %ld\n", t, (long)status);

	}
	printf("All client threads are completed. Exiting. \n");
	pthread_exit(NULL);
}
