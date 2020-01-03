/*
*	swap_client.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


#define	MAXLINE 128	// maximum characters to receive and send at once
#define	MAXFRAME 256


extern int swap_connect(unsigned int addr, unsigned short port);
extern int swap_disconnect(int sd);
extern int sdp_send(int sd, char *buf, int length);
extern int sdp_receive(int sd, char *buf);
extern int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);


int session_id = 0;

int S = 0;	// frame number sent


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


int swap_write(int sd, char *buf, int length)
{
	int n;
	char frame[MAXFRAME];

	if (session_id == 0 || sd != session_id)
		return -1;

	/*
	*	send a DATA frame
	*/

	// ...

	/*
	*	read a frame with a timer
	*/

	// ...

	/*
	*	several different cases including disconnection
	*/

	// ...

	/*
	*	return the length sent
	*/

	// ...
}


void swap_close(int sd)
{
	if (session_id == 0 || sd != session_id)
		return;

	else
		session_id = 0;

	swap_disconnect(sd);	// in sdp.o
}
