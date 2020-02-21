/*
*	swap_client.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>  //
#include <arpa/inet.h>  //


#define	MAXLINE 128	// maximum characters to receive and send at once
// minus 3 so 125
#define	MAXFRAME 256

extern int swap_connect(unsigned int addr, unsigned short port);
extern int swap_disconnect(int sd);
extern int sdp_send(int sd, char *buf, int length);
//writes data of length stored in buf to session sd, max size to send is 256B, returns num_bytes written to sd or -1
extern int sdp_receive(int sd, char *buf);
extern int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);

unsigned char checksum(unsigned char message [], int num_bytes);
unsigned char check_checksum(unsigned char message[], int num_bytes, unsigned char checksum);


int session_id = 0;

int S = 0;	//int + '0';

int swap_open(unsigned int addr, unsigned short port)
{
	int	sockfd;		// sockect descriptor
	struct	sockaddr_in	servaddr;	// server address
	char	buf[MAXLINE];
	int	len, n;

	/*
	*	if the session is already open, then return error
	*/

	if (session_id != 0)
		return -1;

	/*
	*	connect to a server
	*/

	session_id = swap_connect(addr, port);	// in sdp.o

	/*
	*	return the seesion id
	*/

	return session_id;
}

//csum, seqnum, numbits, message, csum calculated with seqnum included

int swap_write(int sd, char *buf, int length)
//session sd, char buffer, num chars in buf
{
	//loop until successful send and ACK receive
	int success=0;
	int nb_written = 0;

	while(success==0)
	{
		//int n;
		char frame[MAXLINE]; //128 -3 for message

		if (session_id == 0 || sd != session_id)
			return -1;

		//calc checksum of buf
		unsigned char csum = checksum((unsigned char*) buf, length);
		printf("sc, csum: %u\n", (unsigned int)csum);  //240
		
		//copy into frame
		frame[0] = (char)csum;
		//test conversion 
		printf("sc, cast frame[0] csum to unsigned int %u\n", (unsigned int)(unsigned char)frame[0]); //240
		//copy seq number
		frame[1] = (char)S;  
		printf("sc, seqnum frame[1]: %d\n", (int)frame[1]); //null
		//copy message length
		frame[2] = (char)length;
		//check
		printf("frame[2]: message length: %d\n", (int)frame[2]);//10

		//copy buffer into frame
		int count = 3;
		for(int i = 3; i<length+3; i++)
		{
			frame[i] = buf[i-3];
			printf("sc, frame[%d]: %c\n", i, frame[i]);  //good
			count++;
		}
		//printf("count: %d\n", count);  //13, 10 chars, seqnum, csum, message_len so works
		//null terminate so we don't infect the frame
		frame[13] = '\0';
		
		//loop until success in writing only
		int succ_write = 0;

	
		while(succ_write==0)
		{
			nb_written = sdp_send(sd, frame, length+3);
			//error in writing -1 so try again
			if(nb_written==-1)
			{
				succ_write=0;
				printf("Write error -1, trying again\n");
			}
			else  //nb_written == new_len len of frame
			{
				printf("sc, success, nb_written: %d\n", nb_written);  //13
				succ_write=1; //successful so exit loop
			}
			//receiver disconnects first time
		}
		// success so move on
		// Receive ACK for sent frame
		char ACKbuffer[2];  //1 char checksum, 1 char seq num
		unsigned int expiration = 2500; 
		//receive ack
		int numB_ack = sdp_receive_with_timer(sd, ACKbuffer, expiration);
		//printf("sc, ack_recv %d\n", ack_recv); //2
		
		
		if(numB_ack < 0)//errors
		{
			if(numB_ack ==-3)
			{
				printf("error -3 ACK not received in time, resend frame\n"); //ACK timer expired so resend frame
				//repeat from top
				S = (S+1) %2;
				//frame[1] = (char)S;
				// try clearing frame
				frame[0] = '\0';
			}
			else if(numB_ack ==-2)
			{
				printf("receiver disconnected, reconnect with swap_open\n");
				//printf("calling swap_open\n");
				//char addr[20] = "127.0.0.1";
				//char port[20] = "9988";
				//int temp = swap_open(inet_addr(addr), htons(atoi(port)));
				//can't call swap_open without argv's in driver
				S = (S+1) %2;
				//frame[1] = (char)S;
				// try clearing frame
				frame[0] = '\0';
			}
			else//-1
			{
				printf("general error code -1, resend frame\n");
				S = (S+1) %2;
				//frame[1] = (char)S;
				// try clearing frame
				frame[0] = '\0';
				
			}
		}
		else //ack_recv == 2 so no error
		{
			
			//server will send error code 9 if checksum did not match
			//check if seqnum position is 9
			if(ACKbuffer[1] =='9' || ACKbuffer[1]=='8')
			{
				printf("ACK rec'd not 0 or 1 but is char %c\n", ACKbuffer[1]);
				//resend frame
			}
			else
			{
				
				unsigned char ACK_csum = (unsigned char)ACKbuffer[0];
				//cast ACK for checksum calc
				unsigned char ACK_actual = (unsigned char)(int)(ACKbuffer[1]);
				printf("sc, ACK received: %d\n", (int)ACK_actual);
				unsigned char ACK [1] = {ACK_actual};
				
				unsigned char check = check_checksum(ACK, 1, ACK_csum);
				printf("check_csum of ACK: %u ", (unsigned int)check);

				//the checksum matches the checksum from ACK num
				//unsigned char check = checksum(ACK, 1);
				//printf("ack_csum: %u vs. check: %u\n", (unsigned int)ACK_csum, (unsigned int)check);
				
				//if checksum is same
				if((unsigned int)check == 255)
				{
					//seq num returned should be S+1 for STop Wait
					//cast ACK to unsigned int to compare with S
					int next_S = (S+1) % 2;
					int ACK_asint = (int)ACK_actual;
					printf("next_S: %d vs ACK %d\n", next_S, ACK_asint);  
					
					//if ACK is seqnum +1 mod 2
					if(next_S == ACK_asint)
					{
						S = next_S;
						success = 1;
						printf("sc, S is now: %d\n", S); 
					} 
				}
			}
		}
		//otherwise resend again from top
		printf("\nResending frame\n");
		//zero out frame
		frame[0] = '\0';
		succ_write=0;
	}
	return nb_written;
}


void swap_close(int sd)
{
	if (session_id == 0 || sd != session_id)
		return;

	else
		session_id = 0;

	swap_disconnect(sd);	// in sdp.o
}

//https://stackoverflow.com/questions/26478827/8-bit-checksum-with-wraparound-in-c
unsigned char checksum(unsigned char message [], int num_bytes)
{
    
    unsigned char sum = 0;

    //loop through (decrement and compare original with 0) and sum
    while (num_bytes-- > 0)
    {   
        //if expr 1 true, return expr 2 else return expr 3
        int carry = (sum + *message > 255) ? 1:0;
        sum += *(message++) + carry;
    }
    return (~sum);
}

unsigned char check_checksum(unsigned char message[], int num_bytes, unsigned char checksum)
{
    unsigned char sum = 0;

    //loop through (decrement and compare original with 0) and sum
    while (num_bytes-- > 0)
    {   
        //if expr 1 true, return expr 2 else return expr 3
        int carry = (sum + *message > 255) ? 1:0;
        sum += *(message++) + carry;
    }
    
	return sum + checksum; //should be 255 if cast to (uint)

}