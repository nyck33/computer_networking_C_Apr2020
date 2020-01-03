//tcp client based on textbook, geeks for geeks and
//https://www.cs.dartmouth.edu/~campbell/cs50/socketprogramming.html
//and the CMU tutorial 
//main difference between UDP and TCP is connect()

#include "headerFiles.h"

int main(int argc, char* argv[]){  //Three args to be checked later
    printf("Nobutaka Kim, T00057442\n");
    int sockfd;  //socket descriptor
    int num_bytes;  //num bytes to rec in each call
    char * serv_name;  //server name
    int portno;  //server port num
    char *string;  //string to be echoed
    char buffer[256+1];  //buffer
    struct sockaddr_in servaddr;  //server socket address

    char *end_signal = "00000";

    if(argc<3){
        printf("Usage ./program {server addr} {portno} {message}... \n");
        exit(1);
    }
    serv_name = argv[1];
    portno = atoi(argv[2]);
    //string = arg[3];

    //create remote server socket address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serv_name);
    servaddr.sin_port = htons(portno);
    //Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0){
        perror("Error: socket creation failed!");
        exit(1);
    }
    //connect to the server, once
    // no need to specify server address in sendto 
	// connect stores the peers IP and port 
    //this is not done for UDP
    int connect_ok = connect(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));
    if(connect_ok<0){
        perror("Error: connection failed!");
        exit(1);
    }
    //first three argv's are program name, ip address, port
    int num_messages = argc - 3;
    printf("num_messages %d\n", num_messages);
    int i = 0;
    int keep_going = 1;
    //main loop
    while(keep_going){
        string = argv[i+3];
        printf("next string: %s\n", string);
        int send_ok = send(sockfd, string, strlen(string), 0);
        /*if(send_ok < 1){
            perror("Sending error\n");
            exit(1);
        }*/
        if(strncmp(string, end_signal, 5)==0)
            break;
        num_bytes=recv(sockfd, buffer,sizeof(buffer), 0);
        if(num_bytes==0){
            perror("Server terminated\n");
            exit(1);
        }
        buffer[num_bytes] = '\0';
        printf("Echoed string received: ");
        puts(buffer);
        printf("\n");
        i++;
        if(i==num_messages)
            break;
        
        bzero(buffer, sizeof(buffer));
    }
    printf("Client closing...\n");
    close(sockfd);
    exit(0);
}