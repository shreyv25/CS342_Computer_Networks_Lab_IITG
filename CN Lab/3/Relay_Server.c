#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define max_buffer_size 1024

int main(int argc, char *argv[]){
	if(argc != 2){
		printf("\nInput format: %s <Server port number> , terminating! \n",argv[0]);       //Invalid input by user
		return -1;
	}

	int server_id = socket(AF_INET, SOCK_STREAM, 0); 					//returns the socket file descriptor

	if(server_id < 0){ 												// failed to get valid socket file descriptor
		perror("Socket error, terminating! \n");
		return -1;
	}

	struct sockaddr_in server_address;									//initializing server address attributes
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(atoi(argv[1]));
	
	char buffer[max_buffer_size];
	
	int peernode_count = 0;
	int peernode_ids[max_buffer_size];
	struct sockaddr_in peernode_addresses[max_buffer_size];

	if(bind(server_id, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
		perror("Unable to bind, terminating!\n");						//bind failed
		return -1;
	}

	printf("Server established with server port number = %d\n\n",ntohs(server_address.sin_port)); //server running

	listen(server_id,5); 												//accepting connections, backlog=5

	struct sockaddr_in client_address;
	int client_address_len = sizeof(client_address);

	for (int i=0;i>=0;i++){ //infinite loop, server doesn't close unless program terminated by user or by error
	
		memset(buffer,'\0',sizeof(buffer));

		//clients that are requesting connection with the server
		int client_id = accept(server_id, (struct sockaddr *)&client_address , (socklen_t *)&client_address_len);

		if(client_id < 0){
			perror("Accept error, couldn't accept connection\n");
			return -1;
		}
		printf("Connection accepted \n");

		//Message received from Client

		if(recv(client_id,buffer,max_buffer_size,0) < 0){
			perror("Message not received by server\n");
			return -1;
		}
		
		//printf("%s\n",buffer);
		char *type = strtok(buffer, "#");
		char *temp = strtok(NULL,"#");
		printf("Message: %s \n",temp);
		temp = strtok(NULL,"#");
		
		int portno;
		if(temp != NULL){
			portno = atoi(temp);
		}

		char message[] = "Hi there! This is the server.";	
		int message_len=strlen(message);	

		if(*type == '1'){
			// Peer node
			peernode_ids[peernode_count] = client_id;
			peernode_addresses[peernode_count].sin_family = client_address.sin_family;
			peernode_addresses[peernode_count].sin_port = portno;
			peernode_addresses[peernode_count].sin_addr.s_addr = client_address.sin_addr.s_addr;

			printf("Peer node port: %d\n",portno);
			printf("Peer node IP: %s\n",inet_ntop(AF_INET, &(client_address.sin_addr), temp, max_buffer_size));

			//Server sends connection confirmation
			if(send(client_id,message,message_len,0) != message_len){
			perror("Message not sent from server\n");
			return -1;
			}

			peernode_count++;
		}
		else if(*type == '0'){
			// Peer Client
			char ip_addr[max_buffer_size];

			printf("Peer client port: %d\n",ntohs(client_address.sin_port));
			inet_ntop(AF_INET, &(client_address.sin_addr), ip_addr, max_buffer_size);
			printf("Peer client IP: %s\n",ip_addr);

			sprintf(message,"%s$%d",message,peernode_count);
			printf("Number of peer nodes = %d\n",peernode_count);

			//fetching details of all peer nodes
			for(int i=0;i<peernode_count;i++){
				inet_ntop(AF_INET, &(peernode_addresses[i].sin_addr), ip_addr, max_buffer_size);
				sprintf(message,"%s:%s:%d",message,ip_addr,peernode_addresses[i].sin_port);
			}

			//sending details of all the peer nodes to the peer client
			if( send(client_id,message ,strlen(message),0) != strlen(message) ){
				perror("Message not sent\n");
				exit(-1);
			}
		}

		else{
			perror("Invalid Client request\n");  //not peer node or peer client
			return -1;
		}

		printf("\n");
	}

	close(server_id); //close socket
}

