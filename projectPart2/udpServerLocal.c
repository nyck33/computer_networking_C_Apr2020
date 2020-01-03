//UDP echo server
#include "headerFiles.h"
int main(int argc, char *argv[]){
	printf("Nobutaka Kim, T00057442\n");
	//declare an define variables
	int sockfd; //socket descriptor
	int len; //length of string to be echoed
	char buffer[1024];  //data buffer
	struct sockaddr_in servAddr, clntAddr;  //server (local) socket address
	bzero(&servAddr, sizeof(servAddr));
	socklen_t clntAddrLen;  //length of client socket address
	char *end_signal = "00000";  //last message to close socket
	//address and port number from argv
	//char *localhost = argv[1];
	int port = atoi(argv[1]);	
	//Build local (server) socket address
	//memset(&servAddr, 0, sizeof(servAddr));  //allocate memory
	servAddr.sin_family = AF_INET;  //family field
	servAddr.sin_port = htons(port);  //default port number
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);  //default IP address
	
    //Create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd<0)
	{
		perror("Error, socket failed");
		exit(1);
	}
	//bind socket to local address and port
	bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));

	clntAddrLen = sizeof(clntAddr);
    for(;;)
	{
		//allocate buffer memory
        //buffer = malloc(sizeof(char)*1024); 
        //receive string
		len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clntAddr,&clntAddrLen);
		buffer[len] = '\0';
		printf("received: %d\n%s\n", len, buffer);
        puts(buffer);
		printf("\n");
        if (strcmp(buffer, end_signal)==0){
			break;
		}
		//send string
		sendto(sockfd, buffer, len, 0, (struct sockaddr*)&clntAddr, sizeof(clntAddr));
        printf("echo sent\n");
	}//End of for loop
	printf("end signal received, closing\n");
	close(sockfd);
	exit(0);

}//End of echo server program
