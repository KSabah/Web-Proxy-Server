#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#define MAX 80 
#define MAX_REQUEST 8192
#define PORT 8080 
#define SA struct sockaddr

struct args
{
	int id;
	int connfd;
	char buffer[MAX_REQUEST];
};

void func(int sockfd) 
{ 
    char buff[MAX]; 
    int n;  
    bzero(buff, sizeof(buff)); 
    printf("Enter request: "); 
    n = 0; 
    while ((buff[n++] = getchar()) != '\n') 
        ; 
    write(sockfd, buff, sizeof(buff)); 
    bzero(buff, sizeof(buff)); 
    read(sockfd, buff, sizeof(buff)); 
    if ((strncmp(buff, "exit", 4)) == 0) { 
        printf("Goodbye.\n"); 
    }
}
  
int main(int argc, char** argv) 
{
    int port = PORT;	
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli;
    if(argv[1] != NULL)
        port = atoi(argv[1]);
    printf("You're connected on port number: %d\n", port);
    
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("Socket creation failed, aborting.\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created.\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(port); 
  
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("Connection with the server failed, aborting.\n"); 
        exit(0); 
    } 
    else
        printf("Connection established.\n"); 
  
    // function for chat 
    func(sockfd); 
  
    // close the socket 
    close(sockfd); 
} 