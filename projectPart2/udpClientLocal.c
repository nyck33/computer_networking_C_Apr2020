// udp client driver program 
//borrowed heavily from geeksforgeeks, textbook code and CMU tutorial
#include "headerFiles.h"

// Driver code 
int main(int argc, char *argv[]){   
    printf("Nobutaka Kim, T00057442\n");
	//64KB is the max size of UDP packet, https://stackoverflow.com/questions/2862071/how-large-should-my-recv-buffer-be-when-calling-recv-in-the-socket-library
	char buffer[1024]; 
	char * end_signal = "00000";
	//socket file descriptor, n= num bytes rec'd from server
    int sockfd, len; 
    //struct for server IP address, port and type 
	struct sockaddr_in servaddr; 
	//address and port number from argv
	char *localhost = argv[1];
	int port = atoi(argv[2]);	
	// clear servaddr 
	bzero(&servaddr, sizeof(servaddr)); 
	// allocate memory
	//memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(localhost);  //inet_addr("127.0.0.1"); 
	servaddr.sin_port = htons(port);  //htons(PORT); 
	
	
	// create datagram socket 
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if(sockfd < 0){
		perror("error, socket failed");
		exit(1);
	}

	// request to send datagram 
	// no need to specify server address in sendto 
	// connect stores the peers IP and port 
    //first three argv's are argc, ip address, port
    int num_messages = argc - 3;
	printf("num_messages %d\n", num_messages);
	int i = 0;
	char *string = "";
    //send all argv messages
	
	int keep_going = 1;
    while(keep_going){
		string = argv[i+3];  //argv[0] is program name
        len = sendto(sockfd, string, strlen(string), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)); 
		if(strcmp(string, end_signal)==0)
			break;
		// waiting for response
		recvfrom(sockfd, buffer, len, 0, NULL, NULL); 
		buffer[len]='\0';
		printf("echo received: ");
        puts(buffer);
		printf("\n"); 
		i++;
		if(i==num_messages)
			break;
    }
	// close the descriptor 
	close(sockfd); 
	exit(0);
} 
