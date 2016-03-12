#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//buffer size for reading from socket
#define BUFFER_SIZE 1000
#define MIN_ARGS 4
//argument for host name
#define ARG_HOSTNAME argv[1]
//argument for port
#define ARG_PORT argv[2]
//argument for input file
#define ARG_FILE argv[3]
//Max no. of files to be read from a file
#define MAX_FILES 100
//Max characters allowed for a file name
#define MAX_FILE_NAME_SIZE 100
//Max time to wait (in seconds) for chunked response from server
#define MAX_WAIT_FOR_RESPONSE_CHUNK 900000

char *generateHttpRequestHeader(char *filename, char *hostname,int port, int *header_length);
/*
 * Prints the error to the standard error stream and exits the program
 */
void signal_error(char *err_msg) {
	fprintf(stderr, err_msg);
	fprintf(stderr, "shutting down");
	exit(1);
}
int main(int argc, char *argv[]) {
	if (argc < MIN_ARGS) {
		if (argc == MIN_ARGS - 1) {
			signal_error(
					"Insufficient arguments. Please enter a file name. Example of proper invocation: ./client localhost 50413 file.txt");
		}
		else if (argc == MIN_ARGS - 2) {
			signal_error(
					"Insufficient arguments. Please enter a port no of the server to connect to and file name.  Example of proper invocation: ./client localhost 50413 file.txt");
		}
		else{
			signal_error(
					"Insufficient arguments. Please enter host name, port no of the server to connect to,file name.  Example of proper invocation: Example of proper invocation: ./client localhost 50413 file.txt");
		}

	} else {
		int socket_file_descr,port;
		unsigned int sockaddr_in_length;
		struct hostent *server_entry;
		struct sockaddr_in server,f;
		port = atoi(ARG_PORT);
		sockaddr_in_length=sizeof (struct sockaddr_in);
		//AF_INET is the domain, SOCK_DGRAM is the communication style
		socket_file_descr=socket(AF_INET, SOCK_DGRAM, 0);
		if(socket_file_descr<=-1){
			signal_error("Failed creating a socket for the client");
		}
		//fetching host information
				server_entry = gethostbyname(ARG_HOSTNAME);
				if (server_entry == NULL) {
					signal_error("Host not found");
				}
				//clearing the struct
				memset(&server, 0, sizeof(server));
				server.sin_family = AF_INET;
				memmove((char *)&server.sin_addr.s_addr,
						(char *)server_entry->h_addr,
						server_entry->h_length);
				//assigning the network byte equivalent port no
				server.sin_port = htons(port);
			int header_length=0;
			//generating http request header
			char *http_request=generateHttpRequestHeader(ARG_FILE,ARG_HOSTNAME,port,&header_length);
			//sending http request
			if(sendto(socket_file_descr,http_request,
					header_length,0,(const struct sockaddr *)&server,sockaddr_in_length)<0){
				signal_error("Error in sending request to server");
			}
			unsigned int bytes_received=0;
			int n=1;
			unsigned char response[BUFFER_SIZE];
			n = recvfrom(socket_file_descr,response,256,0,(struct sockaddr *)&f, &sockaddr_in_length);
			if(n==-1){
				signal_error("error in receiving data");
			}
			struct timeval start, end;
			unsigned long time_taken;
			//start measuring time
			gettimeofday(&start, NULL);
			printf("%s",response);
			bytes_received=n;
			//setting up the max waiting time for chunk of a response to arrive
			fd_set set;
			struct timeval max_wait;
			FD_ZERO(&set);
			FD_SET(socket_file_descr, &set);
			max_wait.tv_sec = 0;
			max_wait.tv_usec = MAX_WAIT_FOR_RESPONSE_CHUNK;
			bzero(response,sizeof(response));
			while(select(socket_file_descr + 1, &set, NULL, NULL, &max_wait)== 1){
			   n = recvfrom(socket_file_descr,response,256,0,(struct sockaddr *)&f, &sockaddr_in_length);
			   if(n==-1){
			   				signal_error("error in receiving data");
			   			}
			   bytes_received+=n;
			   printf("%s",response);
			   bzero(response,sizeof(response));
				//setting up the max waiting time for chunk of a response to arrive

			   FD_ZERO(&set);
			   			FD_SET(socket_file_descr, &set);
			   			max_wait.tv_sec = 0;
			   			max_wait.tv_usec = MAX_WAIT_FOR_RESPONSE_CHUNK;
			}
			//end measuring time
		    gettimeofday(&end, NULL);
		    time_taken = (end.tv_sec - start.tv_sec) * 1000000.0;
		    time_taken += (end.tv_usec - start.tv_usec);
			printf("\n**Time taken for receiving the response(micro seconds):%lu",time_taken);
			printf("\n**Total bytes received:%u\n",bytes_received);
	}
	return 0;
}
/**
 * Generates HTTP request header
 */
char *generateHttpRequestHeader(char *filename, char *hostname,int port,int *header_length){
	char request_header[300] = "";
	snprintf(request_header, sizeof request_header,
				"GET /%s HTTP/1.1\r\nHost: %s:%d\r\nConnection: %s\r\n\r\n",
				filename,
				hostname,
				port,
				"keep-alive"
			);
	//trimming the request header
	char *request_header_trimmed = (char *) calloc(1, strlen(request_header) + 1);
		{
			int i = 0;
			for (; i < strlen(request_header) + 1; i++) {
				request_header_trimmed[i] = request_header[i];
			}
		}
	*header_length=strlen(request_header) + 1;
	return request_header_trimmed;
}



