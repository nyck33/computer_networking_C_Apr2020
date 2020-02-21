/*fta_client.c 

The whole packet sent by swap_client.c is 

"csum / seqnum / len to be written to buffer / 'message'" of which message contains 3 bytes for:
"more bit / len message / actual message"

So actual message is 127 - 5 = 122 or 123 bytes max.

We know transmission is complete when more bit is 0.

Client will read into buffer from file in a loop and write small chunks.

argv[1] source file name, argv[2] destination file name

when sending the destination file name, set more bit to '9'.  


*/

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
extern int swap_write(int sd, char *buf, int length);
extern void swap_close(int sd);
extern int swap_open(unsigned int addr, unsigned short port);

#define MAX_FTA	128

int main (int argc, char *argv[])
{
	int	sd;
	char buf[MAX_FTA];
	int	n;
	int	i;

    if (argc < 3) {
		fprintf(stderr, "Usage: %s IP_address port_number\n", argv[0]);
		exit(1);
	}

    //vars for fopen
    int num;
    FILE *fp;
    long source_size;
    char *buff;

    char source [32];
    strcpy(source, argv[3]);
    printf("fc, Source file is: %s\n", source);

    char dest [32];
    strcpy(dest, argv[4]);
    printf("fc, Destination file is: %s\n", dest);

    //https://stackoverflow.com/questions/3747086/reading-the-whole-text-file-into-a-char-array-in-c/22515917
    if((fp = fopen(source, "rb")) == NULL){
        printf("Error!  Opening file\n");

        exit(1);
    }
    //find end of file for size
    fseek(fp, 0L, SEEK_END);
    //size in bytes
    source_size = ftell(fp);
    //divide into chunks of 125 and remainder
    int num_fragments = source_size / 125;
    printf("fc,num_fragments %d\n", num_fragments);

    int last_block = source_size % 125; 
    printf("fc, last_block %d\n", last_block);

    //convert num_fragments and last_block to strings
    /*
    int length = snprintf(NULL, 0, "%d", num_fragments);
    char* numfrag = malloc(length+1);
    snprintf(numfrag, length+1, %d, num_fragments);
    
    int length2 = snprintf(NULL, 0, "%d", last_block);
    char* lablock = malloc(length2+1);
    snprintf(lablock, length2+1, %d, last_block);
    */
    
    rewind(fp);

    //allocate memory for entire content
    buff = calloc(1, source_size+1);
    if(!buff)
        fclose(fp), fputs("memory allocation error", stderr), exit(1);

    //copy file into buffer
    if(1!= fread(buff, source_size, 1, fp))
        fclose(fp), free(buff), fputs("lump sum read failed", stderr), exit(1);
    
    fclose(fp);

    int connected = 1;

    char curr_buff[125];

    int count = 0;
    int pos = 0;
    while(connected)
    {
        connected = 0;
        // connect to the swap server
        sd = swap_open(inet_addr(argv[1]), htons(atoi(argv[2])));
        if (sd < 0)
        {
            fprintf(stderr, "%s cannot swap_open\n", argv[0]);
            //exit(1);
            continue;
        }
        connected = 1;
        //send buff in chunks of 2+123
        //set more bit and message len as first 2
        int writing = 1;
        while(writing)
        {
            int numB_written = 0;
            //send the destination file name
            if(count==0)
            {
                //more bit > 1 to signal that it's source size and dest file name
                curr_buff[0] = '9';
                //////////////////////////////////////////////////////////////////////////////
                /*
                int dcount = 1;
                //copy num fragments in 
                for(int k=0; k<strlen(numfrag); k++)
                {
                    curr_buff[dcount] = numfrag[k];
                    dcount++;
                }
                */
                ///////////////////////////////////////////////////////////////////////////////
                curr_buff[1] = '9';//(char)num_fragments;
                printf("fc, num_fragments: %d\n", num_fragments);
                //len
                curr_buff[2] = '9';//(char)last_block;
                printf("fc, lastblock: %d\n", last_block);
                //len of dest name
                curr_buff[3] = '9';//(char)strlen(dest);
                printf("fc, strlen dest %d\n", strlen(dest));
                
                //strlen plus signal + num fragments + last block + len destname
                int i=0;
                for(i = 4; i<strlen(dest)+4; i++)
                {
                    curr_buff[i] = dest[i-4];
                }
                curr_buff[i+1] = '\0';
                numB_written = swap_write(sd, curr_buff, strlen(dest)+5);
                count++;
                curr_buff[0]='\0';
                printf("sent dest filename: %s\n", dest);
                continue;
                
            }
            else if(count < num_fragments +1 )// read from buff for num_fragments + dest times
            {
                //more bit
                curr_buff[0] = '1';
                //len
                curr_buff[1] = (char)125;
                
                int i =0;
                for(i = 2; i< 125; i++)
                {
                    curr_buff[i] = buff[pos];
                    pos++;
                }
                //TODO:  need to adjust curr_buff size to 124??????????????????????????????????
                curr_buff[i+1] = '\0';
                //swap write
                numB_written = swap_write(sd, curr_buff, 125);
                count++;
                curr_buff[0] = '\0';
                printf("sent fragment %d", count);
            }
            else // the last block remainder
            {
                curr_buff[0] = '0';  //no more fragments

                curr_buff[1] = last_block;
                
                int i =0;
                for(i = 2; i < last_block +2; i++)
                {
                    curr_buff[i] = buff[pos];
                    pos++;
                }
                curr_buff[i+1] = '\0';
                //swap write
                numB_written = swap_write(sd, curr_buff, last_block+2);
                count++;
                curr_buff[0] = '\0';
                printf("sent last block of %d, frag num %d", last_block, count);
                writing = 0;
            }

        }
        // close the connection to the swap server
        printf("done writing, client closing\n");
        swap_close(sd);
    }

    free(buff);
}
