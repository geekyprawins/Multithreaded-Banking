#include        <sys/time.h>
#include        <sys/types.h>
#include        <sys/socket.h>
#include        <netinet/in.h>
#include        <errno.h>
#include        <netdb.h>
#include        <pthread.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <strings.h>
#include        <unistd.h>

#define CLIENT_PORT	35532


void *command_input_thread(void *arg)
{
	int			sd;		
	char			prompt[] = "\nCmd>>";
	int			len;
	char			string[512];

	/*get our socket descriptor and free the dynamically allocated space we passed it in with */
	sd = *(int *)arg;
	free(arg);
	write( 1, prompt, sizeof(prompt));
	/*Don't want any threads joining on this thread.. let it run and do its thing */
	/*pthread_detach( pthread_self() );*/
	do
	{
		fflush(stdin);	
		len = read( 0, string, sizeof(string));
		string[len]= '\0'; /* *Shwarzeneger voice * (null) TERMINATE DA STRING */	

		sleep( 2 ); /*throttle the input by 2 seconds to simulate having many users connecting to the server */
	}while (write( sd, string, strlen( string ) + 1 ) != -1);
	

	close( sd ); /*just in case something goes haywire and we get here, close the socked descriptor */
	return 0; /*the thread should never get here, as  the loop is inifinite */	
}

void *server_output_thread(void *arg)
{
	int			sd;
	char			buffer[512];
	char			output[512];

	
	/*get our socket descriptor and free the dynamically allocated space we passed it in with */
	sd = *(int *)arg;
	free(arg);

	/*Don't want any threads joining on this thread.. let it run and do its thing */
	/*pthread_detach( pthread_self( ) );*/

	while(read( sd, buffer, sizeof(buffer) ) != 0) /* read what the server is sending while its not th exit command ("q!") */	
	{
		sprintf( output, "\nBank>>%s\nCmd>>", buffer ); 
		write( 1, output, strlen(output) );

	}
	
	fflush(stdin);
	printf("Server disconected, ending session.\n");
	close(sd);
	exit(-1); /*since the threads are user level , exit() closes all threads and terminates the process */
	return 0;
}

void
set_iaddr_str( struct sockaddr_in * sockaddr, char * x, unsigned int port )
{
	struct hostent * hostptr;

	memset( sockaddr, 0, sizeof(*sockaddr) );
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons( port );

	if ( (hostptr = gethostbyname( x )) == NULL )
	{
		printf( "Error getting addr information\n" );
	}
	else
	{
		bcopy( hostptr->h_addr, (char *)&sockaddr->sin_addr, hostptr->h_length );
	}
}

int
main( int argc, char ** argv )
{
	int			sd;
	int			*output_sd;
	int			*input_sd;
	char *			func = "main";
	struct sockaddr_in	addr;
	pthread_t		tid;


	if ( argc < 2 )
	{
		printf( "\x1b[2;31mMust specify server name on command line\x1b[0m\n" );
		return 1;
	}
	
	set_iaddr_str( &addr, argv[1], CLIENT_PORT );
	do {
		errno = 0;
		printf( "Connecting to server %s ...\n", argv[1] );
		if ( (sd = socket( AF_INET, SOCK_STREAM, 0 )) == -1 )
		{
			printf( "socket() failed in %s()\n", func );
			return -1;
		}
		else if ( connect( sd, (const struct sockaddr *)&addr, sizeof(addr)) == -1 )
		{
			close( sd );
		}
		sleep(3); /*wait 3 seconds and attempt to connect again*/
	} while ( errno == ECONNREFUSED );

	if ( errno != 0 )
	{
		printf( "Could not connect to server %s errno %d\n", argv[1], errno );
	}
	else
	{
		
		/*Use dynamically allocated memory to pass the socket descriptor to the server communication threads */		
		input_sd = (int *)malloc( sizeof( int ) );
		output_sd = (int *)malloc( sizeof( int ) );
		*input_sd = sd;
		*output_sd = sd;

		/*create a command_input_thread so that we can send commands to the server */	
		if ( pthread_create( &tid, NULL, command_input_thread, input_sd ) != 0 )
		{
			printf( "pthread_create() for command_input_thread failed in %s()\n", func );
			return 0;
		}
		/*create a server output thread for reading what the server has to say */
		else if ( pthread_create( &tid, NULL, server_output_thread, output_sd ) != 0 )
		{
			printf( "pthread_create() failed in %s()\n", func );
			return 0;
		}
	}
	printf( "Connected to server %s\n", argv[1] );
	/* main is no longer needed so we exit the thread (just the thread not the process) */
	pthread_exit( 0 );
	
}


