#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define CLIENT_PORT 35532
#define SERVER_IP "127.0.0.1" // MODIFY LATER

void *command_input_thread(void *arg)
{
	int sd;
	char prompt[] = "\nCmd>>";
	int len;
	char string[512];

	sd = *(int *)arg;
	free(arg);
	write(1, prompt, sizeof(prompt));

	do
	{
		fflush(stdin);
		len = read(0, string, sizeof(string));
		string[len] = '\0';

		sleep(1);
	} while (write(sd, string, strlen(string) + 1) != -1);

	close(sd);
	return 0;
}

void *server_output_thread(void *arg)
{
	int sd;
	char buffer[512];
	char output[512];
	sd = *(int *)arg;
	free(arg);

	while (read(sd, buffer, sizeof(buffer)) != 0)
	{
		sprintf(output, "\nBank>>%s\nCmd>>", buffer);
		write(1, output, strlen(output));
	}

	fflush(stdin);
	printf("Server disconected, ending session.\n");
	close(sd);
	exit(-1);
	return 0;
}

void set_iaddr_str(struct sockaddr_in *sockaddr, char *x, unsigned int port)
{
	struct hostent *hostptr;

	memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);

	if ((hostptr = gethostbyname(x)) == NULL)
	{
		printf("Error getting addr information\n");
	}
	else
	{
		bcopy(hostptr->h_addr, (char *)&sockaddr->sin_addr, hostptr->h_length);
	}
}

int main()
{
	int sd;
	int *output_sd;
	int *input_sd;
	char *func = "main";
	struct sockaddr_in addr;
	pthread_t tid;

	set_iaddr_str(&addr, SERVER_IP, CLIENT_PORT);
	do
	{
		errno = 0;
		printf("Connecting to server %s ...\n", SERVER_IP);
		if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("socket() failed in %s()\n", func);
			return -1;
		}
		else if (connect(sd, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			close(sd);
		}
		sleep(3);
	} while (errno == ECONNREFUSED);

	if (errno != 0)
	{
		printf("Could not connect to server %s errno %d\n", SERVER_IP, errno);
	}
	else
	{

		input_sd = (int *)malloc(sizeof(int));
		output_sd = (int *)malloc(sizeof(int));
		*input_sd = sd;
		*output_sd = sd;

		if (pthread_create(&tid, NULL, command_input_thread, input_sd) != 0)
		{
			printf("pthread_create() for command_input_thread failed in %s()\n", func);
			return 0;
		}
		else if (pthread_create(&tid, NULL, server_output_thread, output_sd) != 0)
		{
			printf("pthread_create() failed in %s()\n", func);
			return 0;
		}
	}
	printf("Connected to server %s\n", SERVER_IP);
	pthread_exit(0);
}
