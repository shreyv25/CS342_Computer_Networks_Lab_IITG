#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define max_size 1024

int main(int argc, char *argv[])
{
    if(argc != 3)               //Invalid input by user
    {
        printf("Please give a valid Input\n Input format: %s <IP address of server> <Port number of server> \n",argv[0]);
        return 1;
    }

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);    //Declaration and initialization of socket file descriptor for client
    if (socket_id < 0)                                  //Error Case!!!, failed to get valid socket file descriptor
    {
        printf("\n Oh,Failed to get valid socket file descriptor, terminating! \n");
        return -1;
    }

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;                //initializing peer_node_address attributes
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[2]));

    char buffer[max_size] = {0};

    if(inet_pton(AF_INET, argv[1], &server_address.sin_addr)<=0)//Error case, Failed to get valid server IP
    {
        printf("\n Failed to get valid server IP, terminating!\n");
        return -1;
    }

    if( connect(socket_id, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  //Error case, Connection with sever is failed
    {
       printf("\n Unable to connect to the server, terminating! \n");
       return -1;
    }

    // Requesting response from relay server
    char * message = "0#Hey!!! This is the client." ;//Starting Handshaking
    int msg_len = strlen(message);
    //If sent message has not length equal to initial length then, this is an error case
    if( send(socket_id, message, msg_len,0)!= msg_len){
        perror("\n Message not sent from client to server\n");
        return -1;
    }
    //Response from relay server, again
    if(recv(socket_id,buffer,max_size,0) < 0)
    {
        perror("\n Message sent by server, not received by client\n");
        return -1;
    }

    char *temp = strtok(buffer,"$");
    printf("Server is responding with: %s\n\n",temp);
    temp=strtok(NULL,"$");

    int peer_node_count = 0;
    peer_node_count = atoi(strtok(temp,":"));
    printf("PeerNode Count : %d\n\n",peer_node_count);

    temp = strtok(NULL, ":");

    //array to store ports of peer nodes
    char* peernode_ports[peer_node_count];
    //array to store addresses of peer nodes
    struct sockaddr_in peernode_address[peer_node_count];
    int i = 0;

    while(temp != NULL) //extracting peer node addresses and ports from message sent by relay server
    {
        peernode_address[i].sin_family = AF_INET;

        if(inet_pton(AF_INET, temp, &(peernode_address[i].sin_addr.s_addr)) < 0)
        {
            printf("Invalid Address\n");
            return -1;
        }
        temp = strtok(NULL, ":");
        peernode_address[i].sin_port = htons(atoi(temp));
        temp = strtok(NULL, ":");
        i++;
    }

    // Details of all peer nodes
    for(i=0; i< peer_node_count;i++)
    {
        char ip_addr[max_size];
        printf("PeerNode IP: %s\n",inet_ntop(AF_INET, &(peernode_address[i].sin_addr), ip_addr, max_size));
        printf("PeerNode port: %d\n",ntohs(peernode_address[i].sin_port));
    }
    close(socket_id);

    char filename[max_size];
    printf("Please, enter the name of the file: \n");
    scanf("%s",filename); //Filename input from user

    int found=0;
    //search in all peer nodes
    for(i=0;i<peer_node_count && found<1;i++){
        char ip_addr[max_size];
        printf("Peer node number:%d\n",i+1);
        printf("Peer node port: %d\n",ntohs(peernode_address[i].sin_port));
        printf("Peer node IP: %s\n",inet_ntop(AF_INET, &(peernode_address[i].sin_addr), ip_addr, max_size));

        if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        // If connection is not established, then we will simply show error
        if( connect(socket_id, (struct sockaddr *)&(peernode_address[i]), sizeof(peernode_address[i])) < 0)
        {
           printf("\n Oh , connection to peer node was not established\n");
           return -1;
        }

        //Else, Connection is established
        printf("Connection to peer node is established\n");

        if( send(socket_id, filename, strlen(filename),0)!=strlen(filename)) //sending filename to peer node
        {
            perror("Error, filename was not sent\n");
            return -1;
        }

        memset(buffer,'\0',max_size);

        if (recv(socket_id, buffer, 1024, 0) < 0)
        {
            perror("Receive error\n");
            exit(-1);
        }

        long int file_size;

        char *temp = strtok(buffer, "@");
        char * peer_file = temp;

        if(*peer_file=='0') //File not found
        {
            printf("File not found in peer node number %d\n\n",i+1);
            close(socket_id);
            continue;
        }

        printf("File is found in peer node %d\n",i+1);
        found= 1; //Found in atleast one peer node

        if(temp != NULL){
            temp = strtok(NULL,"$");
            file_size= atoi(temp);
        }

        long int remaining_bytes =file_size; //number of bytes left to be transferred

        printf("File size = %ld \n",file_size);

        FILE *rcvd_file=fopen(filename,"w");
        int len;

        while( remaining_bytes >0 && ( (len=recv(socket_id, buffer, 1024, 0)) > 0))
        {
            fwrite(buffer, sizeof(char), len, rcvd_file);
            remaining_bytes  -= len;
            printf("Buffer currently contains: %s \n",buffer); //Printing deatails of current buffer
            printf("Received bytes = %d bytes, Remaining bytes  = %ld bytes \n",len, remaining_bytes );//To show updates on bytes received and left
        }

        printf("Congratulations, File transfer is completed sucessfully\n\n"); //File completely transferred
        fclose(rcvd_file); //close file
        close(socket_id);  //close socket
    }

    if(found==0) //found not found in any of the peers
    {
        printf("File not found in all peer_nodes\n");
    }

    return 0;
}
