#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define CLIENT_PORT 35532
#define SERVER_IP "127.0.0.1" // MODIFY LATER

void* command_input_thread(void *arg);
void* server_output_thread(void *arg);

void set_iaddr_str(struct sockaddr_in *sockaddr, const char *ip, unsigned int port);

int main()
{
	int clientSocket;
	char *func = "main";
	struct sockaddr_in addr;
	pthread_t tid;

	set_iaddr_str(&addr, SERVER_IP, CLIENT_PORT);
	do
	{
		errno = 0;
		printf("Connecting to server %s ...\n", SERVER_IP);
		if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("socket() failed in %s()\n", func);
			return -1;
		}
		else if (connect(clientSocket, (const struct sockaddr *)&addr, sizeof(addr)) == -1)
		{
			close(clientSocket);
		}
		sleep(3);
	} while (errno == ECONNREFUSED);

	if (errno != 0)
	{
		printf("Could not connect to server %s errno %d\n", SERVER_IP, errno);
	}
	else
	{

		if (pthread_create(&tid, NULL, command_input_thread, &clientSocket) != 0)
		{
			printf("pthread_create() for command_input_thread failed in %s()\n", func);
			return 0;
		}
		else if (pthread_create(&tid, NULL, server_output_thread, &clientSocket) != 0)
		{
			printf("pthread_create() failed in %s()\n", func);
			return 0;
		}
	}
	printf("Connected to server %s\n", SERVER_IP);
	pthread_exit(0);
}

void *command_input_thread(void *arg)
{
	int inputSocket		= *(int *)arg;
	char prompt[]		= "\nCmd>>";
	int len				= 0;
	char string[512]	= "";

	write(1, prompt, sizeof(prompt));

	do
	{
		fflush(stdin);
		len = read(0, string, sizeof(string));
		string[len] = '\0';

		sleep(1);
	} while (write(inputSocket, string, strlen(string) + 1) != -1);

	close(inputSocket);
	return 0;
}

void *server_output_thread(void *arg)
{
	int outputSocket;
	char buffer[512];
	char output[526];
	outputSocket = *(int *)arg;

	while (read(outputSocket, buffer, sizeof(buffer)) != 0)
	{
		sprintf(output, "\nBank>>%s\nCmd>>", buffer);
		write(1, output, strlen(output));
	}

	fflush(stdin);
	printf("Server disconected, ending session.\n");
	close(outputSocket);
	exit(-1);
	return 0;
}

void set_iaddr_str(struct sockaddr_in *sockaddr, const char *ip, unsigned int port)
{
	memset(sockaddr, 0, sizeof*sockaddr);
	sockaddr->sin_family = AF_INET;
	inet_pton(AF_INET, ip, &sockaddr->sin_addr.s_addr);
	sockaddr->sin_port = htons(port);
}