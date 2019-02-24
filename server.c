/*
     From: https://github.com/shawnzam/webproxy-assignment-
 */
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include "mysocket.h"
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_REQUEST 8192
#define MAX 2048
#define WEBPORT 80
#define BUFFLEN 500
#define MAX_STR_LEN 100
#define MAX_MSG 200
#define CACHE_LIMIT 5

char* internet= "/usr/bin/google-chrome-stable";

/*
     This struct contains the info that is passed to a thread.
*/
struct args 
{
    int id;
    int connfd;
    char buffer[MAX_REQUEST];
};
/*
    This struct contains informations about the linked list used for blocking URLs
*/
typedef struct node 
{
    char val[MAX_STR_LEN];
    struct node *next;
} Node_t;   
/*
    This struct contains information for implementing the cache
*/
typedef struct cacheObject
{
	char request[MAX_MSG];
	char message[MAX_MSG];
	char site[MAX_MSG];
	struct cacheObject *next;
} Cache_t;
Cache_t* start;
/*
    This struct is filled from data from the remote http request. It contains the whole remote request, the method, path, 
    version, contenttype(not implemented yet), the host and page or file.
*/
struct req
{
    char request[MAX_REQUEST];
    char method[16];
    char path[MAX];
    char version[16];
    char contenttype[128];
    char host[MAX];
    char page[MAX];
};

void *handleConnection(void *args);
void getReqInfo(char *request, struct req *myreq);
void returnFile(int socket, char* filepath);
void sendRemoteReq(char filename[MAX], char host[MAX], int socket, char path[MAX]);
FILE *fp_log; //log
FILE *blocklist; //list of blocked URLs

void makeCache();
//Cache_t* inCache(char request[MAX_MSG]);
void addCache(Cache_t* obj);
void printCache();

/*
 Main setup the socket for listening and enters an infinate while loop. 
 When a connection arrives it spawns a thread and passes the request to the handleConnection function
 */
int main(int argc, char** argv) {
    /*
        Blocking URLs using keyword 'block'
    */
    if(strcmp(argv[1], "block") == 0){
        printf("Enter hostnames separated by a space to block in this format: example.com\nEnter a newline character on a blank line to exit\n");
        char buff[BUFFLEN];
        char *input = calloc(BUFFLEN, sizeof(char));
        while(*(fgets(buff, sizeof(buff), stdin)) != '\n')
        {
            input = realloc(input, strlen(input)+1+strlen(buff));
            strcat(input,buff); 
        }
        input[strcspn(input, "\n")] = 0;
        Node_t *head = NULL;
        head = malloc(sizeof(Node_t));
        Node_t *current = head;
        char delim[] = " ";
        char *data[sizeof(input)+1];
        char *ptr = strtok(input, delim);
        int i = 0;
	    while(ptr != NULL)
	    {
            data[i] = ptr;
            current->next = malloc(sizeof(Node_t));
            if(data[i]!=NULL)
            {
                strcpy(current->val, data[i]);
                current = current->next;
		        ptr = strtok(NULL, delim);
            }
            i++;
	    }      
        blocklist = fopen("server.blocklist", "a");
        current = head;
        while(current->next!=NULL){
            fprintf(blocklist, "%s\n", current->val);
            current = current->next;
        }
        fclose(blocklist);
        return 0;
    }
    else {
        time_t result;
    result = time(NULL);
    struct tm* brokentime = localtime(&result);
    
    int port = atoi(argv[1]);
    int srvfd  = makeListener(port);
    int threadID = 0;
    fp_log = fopen("server.log", "a");
    fprintf(fp_log, "Server started at %s \nListening on Port %d\n",asctime(brokentime), port);	
    printf("Server started at %s \nListening on Port %d\n",asctime(brokentime), port);	
    fclose(fp_log);
    while (1)
    {
        struct args* myargs;
        myargs = (struct args*) malloc(sizeof(struct args));
        myargs->connfd = listenFor(srvfd);
        fp_log = fopen("server.log", "a");
        pthread_t thread;
        threadID++;
        myargs->id = threadID;
        pthread_create(&thread, NULL, handleConnection, myargs); //thread never joins
        fprintf(fp_log, "Thread %d created\n", myargs->id); 
        printf("Thread %d created\n", myargs->id);
    }
    printf("Connection closed");
    close(srvfd);     
    return 0;
    }
}
/*
 Handles the connection by passing the request to the getReqInfo function for parsing 
 and then retuns the request http page via the sendRemoteReq() function.
 */
void *handleConnection(void *args)
{
    clock_t start_time = clock();
    struct args* myarg = (struct args*) args;
    struct req* myreq;
    myreq = (struct req*) malloc(sizeof(struct req));
    int bytesRead;
    bzero(myarg->buffer, MAX_REQUEST);
    bytesRead = read(myarg->connfd, myarg->buffer, sizeof(myarg->buffer));
    getReqInfo(myarg->buffer, myreq);
    blocklist = fopen("server.blocklist", "a+");
    Node_t *head = NULL;
    head = malloc(sizeof(Node_t));
    Node_t *current = head;
    char buff[BUFFLEN];
    char *input = calloc(BUFFLEN, sizeof(char));
    while(fgets(buff, sizeof(buff), blocklist))
    {
        input = realloc(input, strlen(input)+1+strlen(buff));
        strcat(input,buff); 
    } 
    char delim[] = "\n";
    char *data[sizeof(input)+1];
    char *ptr = strtok(input, delim);
    int i = 0;
	while(ptr != NULL)
    {
        data[i] = ptr;
        current->next = malloc(sizeof(Node_t));
        if(data[i]!=NULL)
        {
            strcpy(current->val, data[i]);
            current = current->next;
		    ptr = strtok(NULL, delim);
        }
            i++;
	}  
    current = head;
    while(current->next!=NULL)
    {
        if(strcmp(current->val,myreq->host) == 0)
        {
            fp_log = fopen("server.log", "a");
            fprintf(fp_log, "Tried to access blocked URL, connection aborted.");
            printf("This URL is blocked, aborting connection.\n");
            exit(0);
        }
        current = current->next;
    } 
    int tt = clock() - start_time;
    printf("Time taken: %dms\n", tt);
    sendRemoteReq(myreq->page, myreq->host, myarg->connfd, myreq->path);
    fprintf(fp_log, "Thread %d exits\n", myarg->id); 
    printf("Thread %d exits\n", myarg->id);
    pthread_exit(NULL);
}
/*
 parses the HTTP req and fills the struct req
 */
void getReqInfo(char *request, struct req *myreq)
{
    char host[MAX], page[MAX];
    strncpy(myreq->request, request, MAX_REQUEST-1);
    strncpy(myreq->method, strtok(request, " "), 16-1);
    strncpy(myreq->path, strtok(NULL, " "), MAX-1);
    strncpy(myreq->version, strtok(NULL, "\r\n"), 16-1);
    if(strstr(myreq->request, "https") != NULL)
        sscanf(myreq->path, "https://%99[^/]%99[^\n]", host, page);
    else 
        sscanf(myreq->path, "http://%99[^/]%99[^\n]", host, page);
    strncpy(myreq->host, host, MAX-1);
    strncpy(myreq->page, page, MAX-1);
    fprintf (fp_log, "method: %s\nversion: %s\npath: %s\nhost: %s\npage: %s\n", myreq->method, myreq->version, myreq->path, myreq->host, myreq->page);
}
/*
 Sends the HTTP request to the remote site and retuns the HTTP payload to the connected client.
 */
void sendRemoteReq(char filename[MAX], char host[MAX], int socket, char path[MAX])
{
    time_t result;
    result = time(NULL);
    struct tm* brokentime = localtime(&result);
    int chunkRead;
    int chunkWritten;
	size_t fd;
    char data[512];
	char reqBuffer[512];
    bzero(reqBuffer, 512);
    fd = connectTo(host, WEBPORT);
    strcat(reqBuffer, "GET ");
    strcat(reqBuffer, filename);
    strcat(reqBuffer, " HTTP/1.1\n");
    strcat(reqBuffer, "host: ");
    strcat(reqBuffer, host);
    strcat(reqBuffer, "\n\n");
    fprintf(fp_log, "Request sent to %s :\n%s\nSent at: %s\n", host, reqBuffer, asctime(brokentime));
    printf("Request sent to %s :\n%s\nSent at: %s\n", host, reqBuffer, asctime(brokentime));
    char* prog[3];
    prog[0] = internet;
    prog[1] = path;
    prog[2] = '\0';
    execvp(prog[0], prog);
    chunkRead = write(fd, reqBuffer, strlen(reqBuffer));
    int totalBytesRead = 0;
    int totalBytesWritten = 0;
    chunkRead = read(fd, data, sizeof(data));
    chunkWritten = write(socket, data, chunkRead);
    fprintf (fp_log, "Completed sending %s at %d bytes at %s\n------------------------------------------------------------------------------------\n", filename, totalBytesRead, asctime(brokentime));
    printf("Completed sending %s at %d bytes at %s\n------------------------------------------------------------------------------------\n", filename, totalBytesRead, asctime(brokentime));
    fclose(fp_log);
    close(fd);
    close(socket);
}

void makeCache()
{
    Cache_t *start = NULL;
    start = malloc(sizeof(Cache_t));
    Cache_t *current = start;
    int i = 0;
    while(i < CACHE_LIMIT)
    {
        current->next = malloc(sizeof(Cache_t));
        current = current->next;
        i++;
    }
    current->next = NULL;
}

void addCache(Cache_t* obj)
{
	free(start->next->next->next->next);
	start->next->next->next->next = NULL;
	obj->next = start;
	start = obj;
}

Cache_t* inCache(char request[MAX_MSG])
{
	int i = 0;
	Cache_t* obj = start;
	while(i < CACHE_LIMIT){
		printf("Obj REQ: %s\nREQ: %s\n", obj->request, request);
		if(strcmp(obj->request, request) == 0) {return obj;}
		else {obj = obj->next;}
		i++;
	}
	return obj;
}

void printCache()
{
    Cache_t* current;
	current = start;
	int i = 0;
	while(i < CACHE_LIMIT)
    {
		printf("Number: %d\nRequest: %s\nMessage: %s\nSite: %s\n\n", i, current->request, current->message, current->site);
		i++;
		current = current->next;
	}

}