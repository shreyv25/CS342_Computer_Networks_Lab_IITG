#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define max_buffer_size 1024

int main(int argc, char *argv[])
{

    if(argc != 4)           //Invalid input by user
    {
        printf("\nInvalid Input. \nInput format: %s <Server IP peer_node_address> <Server port number> <Peer port number>  \n",argv[0]);
        return -1;
    }

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);    //socket file id for peer node

    if ( socket_id < 0)                                 // failed to get valid socket file id
    {
        perror("\n Error in peer node creation.\n");
        return -1;
    }

    struct sockaddr_in server_addr,peer_node_addr;

    server_addr.sin_family = AF_INET;                     //initializing peer_node_addr attributes
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[2]));

    char buffer[max_buffer_size]={0};

    if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr)<=0)
    {
        perror("\n Invalid server IP peer_node_addr. Terminating.\n"); //invalid IP peer_node_addr provided by user
        return -1;
    }

    if( connect(socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
       perror("\n Connection of peer node to server failed. Terminating. \n");
       return -1;
    }

    // Requesting response from relay server
    char  msg[] = "1#Hey! This is peer node." ;
    sprintf(msg,"%s#%d",msg,atoi(argv[3]));
    int msg_len = strlen(msg);

    if( send(socket_id, msg, msg_len,0)!= msg_len)              //sending message from peer node to server
    {
        perror("Message not sent from peer node to server.\n");
        return -1;
    }

    // Response received from relay sever

    if(recv(socket_id,buffer,max_buffer_size,0) < 0){
            perror("Message sent by server, not received by peer node.\n");
            return -1;
        }

    printf("Server sent: %s\n",buffer);

    // Close connection with server
    close(socket_id);

    //Phase three, accepting connections from Peer_Clients
    peer_node_addr.sin_family = AF_INET;
    peer_node_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    peer_node_addr.sin_port = htons(atoi(argv[3]));

    if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\n Socket creation error for peer node in phase three \n");
        return -1;
    }

    if(bind(socket_id, (struct sockaddr *)&peer_node_addr, sizeof(peer_node_addr)) < 0){
        perror("Unable to bind peer node in Phase three.");
        return -1;
    }

    listen(socket_id,10);                   //peer node waiting for connection request from peer client
    printf("Port number of peer node: %d\n", ntohs(peer_node_addr.sin_port));

    int peer_node_address_len = sizeof(peer_node_addr);

    for (int i=0;i>=0;i++){ //infinite loop, peer node doesn't close unless program terminated by user or by error

        char fname[max_buffer_size]={0}; //reinitialize fname buffer for every request

        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        int client_id;

        //accept connection request from peer client
        if((client_id=accept(socket_id,  (struct sockaddr *)&client_addr , (socklen_t *)&client_addr_len)) < 0)
        {
            perror("Client connection request not accepted\n");
            return -1;
        }

        //receive filename that peer client is requesting
        if(recv(client_id,fname,max_buffer_size,0) < 0)
        {
            perror("Filename not received\n");
            return -1;
        }

        printf("Client server requesting for file %s\n", fname);
        int f_descriptor =open(fname,O_RDONLY);

        char f_msg[max_buffer_size];

        // File not found

        if(f_descriptor==-1)
        {
            sprintf(f_msg, "0@");
            int f_msglen = strlen(f_msg);
            printf("File not found\n");

            //telling the peer client that file was not found
            if(send(client_id,f_msg,f_msglen,0) != f_msglen){
                perror("Message not sent\n");
                return -1;
            }

            //move to next peer node
            continue;
        }

        // File found, sending file
        struct stat file_stats;

        if (fstat(f_descriptor, &file_stats) < 0)
        {
                fprintf(stderr, "Error in obtaining file stats : %s", strerror(errno)); //error in obtaining file stats

                exit(EXIT_FAILURE);
        }

        fprintf(stdout, "File size: %ld bytes\n", file_stats.st_size);

        sprintf(f_msg, "1@%ld",file_stats.st_size);
        int f_msg_len = strlen(f_msg);
        int p = send(client_id,f_msg,f_msg_len,MSG_NOSIGNAL);               //sending file stats to peer client

        if(p != f_msg_len){
            printf("File stats not sent\n");
            return -1;
        }

        off_t offset = 0;
        long int sent_bytes=0,remaining_data = file_stats.st_size;

        // Sending file data
        while (((sent_bytes = sendfile(client_id, f_descriptor, &offset, BUFSIZ)) > 0) && (remaining_data > 0))
        {
            remaining_data -= sent_bytes;
            fprintf(stdout, "Server sent %ld bytes from file's data, offset = %ld and remaining data = %ld\n", sent_bytes, offset, remaining_data);
        }
        printf("File transfer complete.\n\n");
    }

    close(socket_id); //closing the socket

    return 0;
}
