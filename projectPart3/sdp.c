/*
*	misc.c
*
*		swap_connect() and swap_accept() should not used together
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/unistd.h>


int swap_connect(unsigned int addr, unsigned short port);
int swap_disconnect(int sd);
int swap_accept(unsigned short port);
int sdp_receive(int sd, char *buf);
int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);
int sdp_send(int sd, char *buf, int length);

#define	MAXLINE	256	// maximum characters to receive and send at once


int sockfd;
unsigned int opponent_addr;
unsigned short opponent_port;
int sssn_id = 0;


int swap_connect(unsigned int addr, unsigned short port)
{
	struct	sockaddr_in	servaddr;	// server address
	char	buf[MAXLINE];
	int	len, n;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("swap_connect: can not open a socket\n");
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = addr;
	servaddr.sin_port = port;

	buf[0] = 0xff;
	n = sendto(sockfd, buf, 10, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if (n < 0)
		return -1;
	else {
		opponent_addr = addr;
		opponent_port = port;
		sssn_id++;
		return sssn_id;
	}
}


int swap_disconnect(int sd)
{
	if (sd != sssn_id)
		return -1;

	char buf[MAXLINE];

	buf[0] = 0xfe;	// disconnect

	sdp_send(sd, buf, 10);
}


int swap_accept(unsigned short port)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	struct	sockaddr_in	servaddr;	// server address
	struct	sockaddr_in	cliaddr;	// server address
	char	buf[MAXLINE];
	int	len, n;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("swap_accept: can not open a socket\n");
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = port;
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		printf("swap_accept: can not bind\n");
		return -1;
	}

	while(1) {
		while(1) {
			FD_ZERO(&rfds);
			FD_SET(sockfd, &rfds);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
			if (retval)
				break;
		}

		len = sizeof(cliaddr);
		n = recvfrom(sockfd, buf, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
//printf("accept: received, n = %d, %x\n", n, (unsigned char)buf[0]);
		if ((unsigned char)buf[0] == 0xff) {
//printf("accept: received connection request\n");
			break;
		}
	}

	opponent_addr = cliaddr.sin_addr.s_addr;
	opponent_port = cliaddr.sin_port;
//printf("swap_accept: addr=%x, port=%d\n", ntohl(opponent_addr), ntohs(opponent_port));
//printf("swap_accept: addr=%x, port=%d\n", ntohl(cliaddr.sin_addr.s_addr), ntohs(cliaddr.sin_port));
	sssn_id++;

	return sssn_id;
}


int sdp_receive(int sd, char *buf)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	struct	sockaddr_in	cliaddr;
	int	len, n;

	if (sd != sssn_id)
		return -1;	// general error

	while(1) {
		while(1) {
			FD_ZERO(&rfds);
			FD_SET(sockfd, &rfds);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
			if (retval)
				break;
		}

		len = sizeof(cliaddr);
		n = recvfrom(sockfd, buf, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);

		if ((unsigned char)buf[0] == 0xfe)  // disconnection from the opponent SWAP
			return -2;  // disconnection
//	It is done in sdp_send().
//		if (random() % 100 < 20 && n > 0)
//			continue;  // let's drop it
		//if (random() % 100 < 50 && n > 10) {
	//		buf[n-1] = 0xff;
	//	}

		break;
	}

	if (n <= 0)
		return -1;	// general error
	else if ((unsigned char)buf[0] == 0xfe)  // disconnection from the opponent SWAP
		return -2;	// disconnected
	else
		return n;
}


// expiration in milliseconds
int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	struct	sockaddr_in	cliaddr;
	int	len, n;

	if (sd != sssn_id)
		return -1;	// general error

	while(1) {

		/*
		*	set timer
		*/

		while(1) {
			FD_ZERO(&rfds);
			FD_SET(sockfd, &rfds);
			tv.tv_sec = expiration / 1000;
			tv.tv_usec = (expiration % 1000) * 1000;	// usec -> microsecond, not millisecond
			retval = select(sockfd+1, &rfds, NULL, NULL, &tv);  //select() blocks until ready to execute
			if (retval)
				break;
			else
				return -3;	// timeout error
		}

		/*
		*	receive data
		*/

		len = sizeof(cliaddr);
		n = recvfrom(sockfd, buf, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);

		/*
		*	if disconnected
		*/

		if (n > 0 && (unsigned char)buf[0] == 0xfe)  // disconnection from the opponent SWAP
			return -2;

		/*
		*	include some errors
		*/
		/*
		if (random() % 100 < 20 && n > 0)
			return -3;  // timeout error
		if (random() % 100 < 20 && n > 10) {
			buf[n-1] = 0xff;
		}
		*/
		break;
	}

	if (n <= 0)
		return -1;	// general error
	else if ((unsigned char)buf[0] == 0xfe)  // disconnection from the opponent SWAP
		return -2;	// disconnected
	else
		return n;
}


int sdp_send(int sd, char *buf, int length)
{
	struct	sockaddr_in	cliaddr;
	int	len, n;

	if (sd != sssn_id)
		return -1;

	// 10% of packets are dropped
	/*
	if (random() % 100 < 10)
		return length;  // let's drop it
	*/
	// the last 80% of packets

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr = opponent_addr;
	cliaddr.sin_port = opponent_port;

//printf("sdp_send: before sending, addr=%x, port=%d\n", ntohl(opponent_addr), ntohs(opponent_port));
	n = sendto(sockfd, buf, length, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
//printf("sdp_send: after sending\n");

	return n;
}


