/*
*	swap_server.c
What happens when buf > 254 bytes/chars?  Use a more fragment bit, save current message to buffer or write to file, keep
file open, append and refresh buffer or open new file
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>  //
#include <arpa/inet.h>  //


#define	MAXLINE	128	// maximum characters to receive and send at once
#define MAXFRAME 256

extern int swap_accept(unsigned short port);
extern int swap_disconnect(int sd);
extern int sdp_send(int sd, char *buf, int length);
extern int sdp_receive(int sd, char *buf);
extern int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);
unsigned char checksum(unsigned char message [], int num_bytes);
unsigned char check_checksum(unsigned char message[], int num_bytes, unsigned char checksum);


int session_id = 0;

int R = 0;	// int to char is int + '0';


int swap_wait(unsigned short port)
{
	/*
	*	if the session is already open, then return error
	*/

	if (session_id != 0)
		return -1;

	/*
	*	accept a connection
	*/

	session_id = swap_accept(port);	// in sdp.o

	/*
	*	return a ssession id
	*/

	return session_id;
}


int swap_read(int sd, char *buf)
{
	int	n;
	char frame[MAXFRAME];

	if (session_id == 0 || sd != session_id)
		return -1;

	int success=0;
	int numB_recv = 0;
	
	//success means csum is correct, seqnum is correct
	while(success==0)
	{
		
		//keep going until client disconnects
		
		//receive a frame without a timer  
		numB_recv = sdp_receive(sd, frame);
		printf("\nss, numBrecv: %d\n check frame\n", numB_recv);  //13
		//test
		for(int m =0; m< numB_recv; m++)
		{
			printf("ss, frame[%d]: %c\n", m, frame[m]);
		}
		//unpack the frame:  order is checksum, seq num, message
		unsigned char csum = (unsigned char)frame[0];
		int seqnum = (int)(frame[1]);
		int message_len = (int)frame[2];
		printf("ss, csum: %u\n seqnum: %d\n, message_len: %d\n",(unsigned int)csum, seqnum, message_len);
		//240, 0, 10
		
		//calc checksum of message, if wrong send same ACK
		char temp [message_len];
		//iterate for message_len from frame[3]
		for(int i = 3; i< message_len+3; i++)
		{
			temp[i-3] = frame[i];
			printf("ss, temp[%d]: %c\n", i-3, temp[i-3]);  //okay
		}

		//unsigned char calc_csum = checksum((unsigned char*)temp, message_len);

		unsigned char csum_checker = check_checksum((unsigned char*)temp, message_len, csum);

		printf("csum_checker is 255?: %u\n", (unsigned int)csum_checker);  //should be 255

		//printf("ss, calc_csum: %u vs. actual csum: %u\n", (unsigned int) calc_csum, (unsigned int)csum);  //240 vs 240

		// if csum wrong ask for same frame by sending back error code 9
		if ((unsigned int)csum_checker !=255)
		{
			char csum_wrong [2];
			csum_wrong[0] = '9';
			csum_wrong[1] = '9';

			printf("ss, csum_wrong check: %c %c\n", csum_wrong[0], csum_wrong[1]);
			
			int error_numB = sdp_send(sd, csum_wrong, 2);
			printf("ss, error code 99 sent bytes = %d csum wrong\n", error_numB);
			//loop back so empty temp
			temp[0]='\0';
			frame[0]='\0';
			
		}
		else //checksum is correct
		{
			printf("seqnum check: %d vs. R: %d\n", seqnum, R);//0 vs 0, 
			//check seqnum vs. R
			if(seqnum == R)
			{
				//copy message len into buf
				int count = 0;
				for(int i = 3; i<message_len+3; i++)
				{
					buf[i-3] = frame[i];
					printf("ss, buf[%d]: %c\n", i-3, buf[i-3]);  //good
					count++;
				}
				//printf("count: %d\n", count);
				buf[message_len] = '\0';
				//slide window
				R = (R+1) % 2;
				printf("R is now: %d\n", R);  //1

				//send ACK with checksum
				//put R in array for checksum calc
				unsigned char seqnum_arr[1];
				seqnum_arr[0] = (unsigned char)R;
				unsigned char ack_csum = checksum(seqnum_arr, 1);

				char ACK [2];
				ACK[0] = (char)ack_csum;
				ACK[1] = (char)(R);
				printf("ACK [csum, ACK] %u %d\n", (unsigned int)(unsigned char)ACK[0], (int)ACK[1]);  //254, 1
				
				int ack_sendB = sdp_send(sd, ACK,2);
				printf("ss, ACK sent %d bytes\n", ack_sendB);  //2
				//get out and return to driver
				temp[0] ='\0';
				frame[0] ='\0';
				success = 1;

			}
			else  //seqnum != R so send error code
			{
				char seqnum_wrong [2];
				seqnum_wrong[0] = '8';
				seqnum_wrong[1] = '8';

				printf("ss, seqnum_wrong, sending: %c %c\n", seqnum_wrong[0], seqnum_wrong[1]);
				
				int error_numB = sdp_send(sd, seqnum_wrong, 2);
				("seqnum ACK send error code %d", error_numB);
				//clear temp
				temp[0]='\0';
				frame[0]='\0';
			}
			
		}
	}		
		
		return numB_recv-3;
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
    
	return sum + checksum;

}