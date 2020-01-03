//my tcp server
// create parent socket to listen and child socket to accept connections
#include "headerFiles.h"
int main(int argc, char *argv[]){
    printf("Nobutaka Kim, T00057442\n");
    int parentfd;  //socket descriptor
    int childfd; //reference to sockfd, childfd in demo
    int portno; //listen port num
    int len_clntaddr;  //length of client address
    struct sockaddr_in servaddr, clientaddr; 
    char buffer[1024]; 
    int bytes = 0;  //num bytes for each recv and echo
    int wait_size = 16;  //size of waiting clients
    //textbook vars
    int max_len = sizeof(buffer);
    //char *buf_ptr = buffer;
    //end of messages
    char *end_signal = "00000";  //close when rec'd this
    //command line args
    if(argc!=2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);
    
    //create local (server) socket address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;  //default family
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //default IP address, little endian big endian
    servaddr.sin_port = htons(portno);  //default port converted to network byte order 

    //create listen socket
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if(parentfd<0){
        perror("Error: opening failed");
        exit(1);
    }
    // 
    
    int bind_ok = bind(parentfd,(const struct sockaddr *) &servaddr, sizeof(servaddr));
    if(bind_ok<0){
        perror("Error: binding failed!");
        exit(1);
    }
    //Listen to connection requests
    int listen_ok = listen(parentfd, wait_size);
    if(listen_ok<0){
        perror("Error: listening failed!");
        exit(1);
    }
    //each clntaddr could be of differing lengths
    len_clntaddr = sizeof(clientaddr);
    //accept:  wait for a connection request
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr,(socklen_t *) &len_clntaddr);
    if(childfd<0){
        perror("Error:  accepting failed!");
        exit(1);
    }

    //Handle connection
    while(1){
        //data transfer
        bytes=recv(childfd, buffer, max_len, 0);
        if(bytes<0){
            printf("server recv error");
            break;
        }
        printf("rec'd %d bytes\n", bytes);  //== num chars
        buffer[bytes] = '\0';
        if (strncmp(buffer, end_signal, 5)==0){
        	break;
        }
        puts(buffer);
        //send back all bytes rec'd
        int send_ok = send(childfd, buffer, bytes, 0);
        if(send_ok < 0)
            perror("Error sending echo");
            
        bzero(buffer, sizeof(buffer));
                
    }
    printf("Server exiting...\n");
    close(childfd);
    close(parentfd);
}   