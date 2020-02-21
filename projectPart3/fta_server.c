//fta_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>  //
#include <sys/socket.h> //
#include <netinet/in.h> //
#include <arpa/inet.h>  //

extern int swap_wait(unsigned short port);
extern int swap_read(int sd, char *buf);
extern void swap_close(int sd);

#define MAX_FTA	128  //actual len of buf is max 125 since 3 used for csum, seqnum and mlen
					//here we have more bit == '9' for file name and message len < 123 when last fragment

int main(int argc, char *argv[])
{
    int sd, bytes_read;
    char buf[MAX_FTA];

	//vars for fopen
    int num;
    FILE *fp;
    long dest_size;
    char *big_buff;
	char dest[32];

	
    if (argc < 2) {
		fprintf(stderr, "Usage: %s port_number\n", argv[0]);
		exit(1);
	}
	
	sd = swap_wait(htons(atoi(argv[1])));
	if (sd < 0) {
		fprintf(stderr, "%s cannot wait, %d\n", argv[0], sd);
		exit(1);
	}

	int reading = 1;
    
	//int dest_size = 0;
	int num_fragments = 0;
	int curr_fragment = 0;
	int last_block = 0;
	int destname_len = 0;
	int pos = 0;
	while(reading)
	{
		bytes_read = swap_read(sd, buf); 
		
		//check more bit if 9 filename
		if(buf[0] == '9')
		{
			num_fragments = (int)buf[1];
			last_block = (int)buf[2];
			destname_len = (int)buf[3];
			//copy destination name to char dest []
			int i = 0;
			for(i=0; i<destname_len; i++)
			{
				dest[i] = buf[i+4];
			}
			dest[i+1] = '\0';

			//allocate memory for entire content
			dest_size = (125 * num_fragments) + last_block; 
			big_buff = calloc(1, dest_size+1);
    		if(!big_buff)
        		fclose(fp), fputs("memory allocation error\n", stderr), exit(1); 
			printf("dest rec'd: %s, big_buff %ld allocated\n", dest, sizeof(big_buff));
			continue;
		}
		else  //
		{
			if(num_fragments==0)//message is less than 125B 
			{
				for(pos=0; pos<last_block; pos++)
				{
					big_buff[pos] = buf[pos+2];
				}
				reading = 0;
				printf("read %ldB short msg\n", sizeof(big_buff));
			}
			else //more than 1 fragment
			{
				if(curr_fragment < num_fragments) //read the full 123
				{
					for(int j=2; j<125; j++)
					{
						big_buff[pos] = buf[j];
						pos++;
					}
					curr_fragment++;
					printf("read fragment %d\n", curr_fragment-1);
				}
				else //curr_fragment == num_fragments so read last block
				{
					if(last_block==0) //nothing in last block
					{
						reading=0;
					}
					else
					{
						for(int j = 2; j< last_block+2; j++)
						{
							big_buff[pos] = buf[j];
							pos++;
						}
						reading = 0;
						printf("read last block %dB\n", last_block);
					}
				}
			}
		}
	}
	// write big_buff to file, call fopen with dest and fwrite to write
	if((fp = fopen(dest, "wb")) == NULL){
        printf("Error!  Opening file\n");

        exit(1);
    }
	int bb_size = sizeof(big_buff);
	printf("writing %d bytes bigbuff to file\n", bb_size);
	fwrite(big_buff, sizeof(char), sizeof(big_buff), fp);
	fclose(fp);
	printf("all received from test_swap_server, closing connection\n");
	// close the file and the connection
	swap_close(sd);
}