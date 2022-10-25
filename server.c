#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENT_PORT 35532
#define MAX_CUSTOMERS 20

typedef struct
{
	char	accountName[100];
	int		isInUse;
	float	balance;
}Account;

enum BankError{
	NO_ACCOUNT=-10,
	ACCOUNT_EXISTS,
	ACCOUNT_IN_USE,
	INSUFFICIENT_BALANCE,
	SERVER_FULL,
};

/* Global variables used */
static pthread_attr_t	kernelAttribute;
static sem_t			actionLock;
static Account			customers[MAX_CUSTOMERS];
static pthread_mutex_t	customerMutex[MAX_CUSTOMERS];
static pthread_mutex_t	bankMutex;
static int				connectionCount = 0;

/* Function prototypes */
float	parse_float(const char* string);
char*	ps(unsigned int x, char *s, char *p);
void	abnormal_exit(const char* msg);
void	set_iaddr(struct sockaddr_in *sockaddr, long x, unsigned int port);
void	server_response(int sock, char* response);
void	close_connection(int sock);

int		search_account(char* accountName); 
int		serve_account(char* accountName);
int		create_account(char* accountName);

void	periodic_action_handler(int signo, siginfo_t *ignore, void *ignore2);
void*	periodic_action_cycle_thread(void *ignore);
void*	session_acceptor_thread(void *ignore);

int main(int argc, char **argv)
{
	pthread_t tid;
	
	// Initialize customer associated mutexes with default values
	for (int i = 0; i < MAX_CUSTOMERS; i++)
	{
		if (pthread_mutex_init(&customerMutex[i], NULL) != 0)
		{
			abnormal_exit("pthread_mutex_init()");
		}
	}

	// pthread_attr and mutex initializations as well as pthread_create for 
	// session acceptor and periodic action handler
	if (pthread_attr_init(&kernelAttribute) != 0)
	{
		abnormal_exit("pthread_attr_init()");
	}
	else if (pthread_attr_setscope(&kernelAttribute, PTHREAD_SCOPE_SYSTEM) != 0)
	{
		abnormal_exit("pthread_attr_setscope()");
	}
	else if (sem_init(&actionLock, 0, 0) != 0)
	{
		abnormal_exit("sem_init()");
	}
	else if (pthread_mutex_init(&bankMutex, NULL) != 0)
	{
		abnormal_exit("pthread_mutex_init()");
	}
	else if (pthread_create(&tid, &kernelAttribute, session_acceptor_thread, 0) != 0)
	{
		abnormal_exit("pthread_create()");
	}
	else if (pthread_create(&tid, &kernelAttribute, periodic_action_cycle_thread, 0) != 0)
	{
		abnormal_exit("pthread_create()");
	}
	else
	{
		printf("server is ready to receive client connections ...\n");
		pthread_exit(0);
	}
}

void abnormal_exit(const char* msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

void set_iaddr(struct sockaddr_in *sockaddr, long x, unsigned int port)
{
	memset(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(port);
	sockaddr->sin_addr.s_addr = htonl(x);
}

char* ps(unsigned int x, char *s, char *p)
{
	return x == 1 ? s : p;
}

void periodic_action_handler(int signo, siginfo_t *ignore, void *ignore2)
{
	if (signo == SIGALRM)
	{
		sem_post(&actionLock);
	}
}

void *periodic_action_cycle_thread(void *ignore)
{
	/* Variables declaration, index, sigaction struct and timer */
	int i;
	struct sigaction action;
	struct itimerval interval;

	/* no need for any thread to join on this */
	pthread_detach(pthread_self());

	/*initialization of signal handler and timer for the 20 second account print cycle */
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	action.sa_sigaction = periodic_action_handler;
	sigemptyset(&action.sa_mask);
	sigaction(SIGALRM, &action, 0);
	/* it_value, time until next expiration
	it_intervation, interval for periodic timer */
	/* tv_sec seconds, tv_usec microseconds*/
	interval.it_interval.tv_sec = 20;
	interval.it_interval.tv_usec = 0;
	interval.it_value.tv_sec = 20;
	interval.it_value.tv_usec = 0;
	// ITIMER_REAL counts down in real time
	// which newvalue oldvalue
	setitimer(ITIMER_REAL, &interval, 0);
	while(1) /* infinite loop */
	{
		/* decrement sem count and lock a mutex to go through the bank accounts!*/
		sem_wait(&actionLock);
		pthread_mutex_lock(&bankMutex);
		printf("There %s %d active %s.\nCustomers:\n", ps(connectionCount, "is", "are"),
			connectionCount, ps(connectionCount, "connection", "connections"));
		for (i = 0; i < MAX_CUSTOMERS; i++)
		{
			if (customers[i].isInUse == 1)
			{
				printf("IN SERVICE: ");
			}
			if (strcmp(customers[i].accountName, "") != 0)
			{
				printf("%s\n", customers[i].accountName);
			}
		}
		pthread_mutex_unlock(&bankMutex);
		/* Calling thread to relinquish CPU */
		sched_yield();
	}
	return 0;
}

float parse_float(const char* string){
	// Buffer used during float conversions
	static char floatNumBuffer[32];
	float num = atof(string);
	sprintf(floatNumBuffer, "%0.2f", num);
	num = atof(floatNumBuffer);
	return num;
}

void server_response(int sock, char* response)
{
	write(sock, response, strlen(response)+1);
	printf("%s\n", response);
}

void close_connection(int sock){
	// Closing connection and decrementing connectionCount
	close(sock);
	pthread_mutex_lock(&bankMutex);
	--connectionCount;
	pthread_mutex_unlock(&bankMutex);
}

void* client_session_thread(void *arg)
{
	// main thread will not be waiting for this thread, so let us detach it
	pthread_detach(pthread_self());

	// Get mutex lock for connectionCount and increment it
	pthread_mutex_lock(&bankMutex);
	++connectionCount;
	pthread_mutex_unlock(&bankMutex);
	printf("New Client has connected!\n");

	// Server socket for handling client
	int sock = *(int *)arg;

	// Holds index of current Account being handled.
	// Initialized to NO_ACCOUNT
	int currentIndex = NO_ACCOUNT;
	
	// Buffers for input and output
	char request[256], response[256];

	// Server keeps accepting input from client.
	// When client disconnects, read() returns 0 and the server will close connection.
	while(read(sock, request, sizeof(request)) > 0)
	{
		printf("\nServer received input:  %s\n", request);
		if(currentIndex == NO_ACCOUNT)
		{
			if(strncmp(request, "create ", 7) == 0)
			{
				pthread_mutex_lock(&bankMutex);
				int status = create_account(request+7);
				pthread_mutex_unlock(&bankMutex);
				switch(status){
				case ACCOUNT_EXISTS:
					sprintf(response, "Error: Account for %s already exists", request+7);
					break;
				case SERVER_FULL:
					sprintf(response, "Error: No memory left for more accounts");
					break;
				
				default:
					sprintf(response, "Success: Account for %s was created", request+7);
				}
				server_response(sock, response);
			}
			else if(strncmp(request, "serve ", 6) == 0)
			{
				pthread_mutex_lock(&bankMutex);
				int status = serve_account(request+6);
				switch(status){
				case NO_ACCOUNT:
					sprintf(response, "Error: Account %s does not exist", request+6);
					break;
				case ACCOUNT_IN_USE:
					sprintf(response, "Error: Account %s is already in use", request+6);
					break;
				
				default:
					currentIndex = status;
					sprintf(response, "Success: Successfully logged into %s's account", request+6);
					break;
				}
				pthread_mutex_unlock(&bankMutex);
				server_response(sock, response);
			}
			else if(strncmp(request, "quit", 4) == 0)
			{
				break;
			}
			else
			{
				server_response(sock, "Error: Unknown command sent to base server");
			}
		}
		else
		{
			if(strncmp(request, "query", 5) == 0)
			{
				pthread_mutex_lock(&customerMutex[currentIndex]);	
				float balance = customers[currentIndex].balance;
				pthread_mutex_unlock(&customerMutex[currentIndex]);
				sprintf(response, "%-20s %s\n%-20s %0.2f", 
					"Full Name: ", customers[currentIndex].accountName, 
					"Balance in Rupees: ", balance);
				server_response(sock, response);
			}
			else if(strncmp(request, "end", 3) == 0)
			{
				pthread_mutex_lock(&customerMutex[currentIndex]);
				customers[currentIndex].isInUse = 0;
				pthread_mutex_unlock(&customerMutex[currentIndex]);
				sprintf(response, "Success: Logged out from %s's account", customers[currentIndex].accountName);
				server_response(sock, response);
				currentIndex = NO_ACCOUNT;
			}
			else if(strncmp(request, "deposit ", 8) == 0)
			{
				float amount = parse_float(request+8);
				if (amount > 0)
				{
					pthread_mutex_lock(&customerMutex[currentIndex]);
					customers[currentIndex].balance += amount;
					pthread_mutex_unlock(&customerMutex[currentIndex]);
					sprintf(response, "Success: Deposited Rs%0.2f into %s's account", 
						amount, customers[currentIndex].accountName);
					server_response(sock, response);
				}
				else
				{
					sprintf(response, "Error: %s sent an invalid number", 
						customers[currentIndex].accountName);
					server_response(sock, response);
				}
			}
			else if (strncmp(request, "withdraw ", 9) == 0)
			{
				float amount = parse_float(request+9);
				if (amount > 0)
				{
					pthread_mutex_lock(&customerMutex[currentIndex]);
					if (customers[currentIndex].balance >= amount)
					{
						customers[currentIndex].balance -= amount;
						sprintf(response, "Success: Withdrew of Rs%0.2f from %s's account", 
							amount, customers[currentIndex].accountName);
						server_response(sock, response);
					}
					else
					{
						sprintf(response, "Error: Cannot withdraw Rs%.2f from %s's account due to insufficient balance", 
							amount, customers[currentIndex].accountName);
						server_response(sock, response);
					}
					pthread_mutex_unlock(&customerMutex[currentIndex]);
				}
				else
				{
					sprintf(response, "Error: %s sent an invalid number", 
						customers[currentIndex].accountName);
					server_response(sock, response);
				}
			}
			else if (strncmp(request, "transfer ", 9) == 0)
			{
				size_t amountPtr = strcspn(request, "\n");
				request[amountPtr] = '\0';
				int index = search_account(request+9);
				if(index == NO_ACCOUNT){
					sprintf(response, "Error: Account %s does not exist", request+9);
					server_response(sock, response);
				}
				else
				{
					float amount = parse_float(request+amountPtr+1);
					if (amount > 0)
					{
						pthread_mutex_lock(&customerMutex[currentIndex]);
						if (customers[currentIndex].balance >= amount)
						{
							customers[currentIndex].balance -= amount;
							pthread_mutex_unlock(&customerMutex[currentIndex]);
							pthread_mutex_lock(&customerMutex[index]);
							customers[index].balance += amount;
							pthread_mutex_unlock(&customerMutex[index]);
							sprintf(response, "Success: Transferred %.2f from %s to %s",
								amount, customers[currentIndex].accountName, customers[index].accountName);
							server_response(sock, response);
						}
						else
						{
							pthread_mutex_unlock(&customerMutex[currentIndex]);
							sprintf(response, "Error: %s has insufficient balance for transfer of Rs%.2f", 
								customers[currentIndex].accountName, amount);
							server_response(sock, response);
						}
					}
					else
					{
						sprintf(response, "Error: %s sent an invalid number", 
							customers[currentIndex].accountName);
						server_response(sock, response);
					}
				}
			}
			else
			{
				server_response(sock, "Error: Unknown command sent to login server");
			}
		}
	}
	close_connection(sock);
	pthread_exit(0);
}

void *session_acceptor_thread(void *ignore)
{
	int sd;
	int fd;
	struct sockaddr_in addr;
	struct sockaddr_in senderAddr;
	socklen_t ic;
	pthread_t tid;
	int on = 1;
	char *func = "session_acceptor_thread";

	pthread_detach(pthread_self());					  /* no waiting on this thread ! */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) /* get socket store in sd */
	{
		printf("socket() failed in %s()\n", func);
		return 0;
	}
	else if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) /*reserve a port to bind on */
	{
		printf("setsockopt() failed in %s()\n", func);
		return 0;
	}
	else if (set_iaddr(&addr, INADDR_ANY, CLIENT_PORT), errno = 0,
			 bind(sd, (const struct sockaddr *)&addr, sizeof(addr)) == -1) /*bind to a port */
	{
		printf("bind() failed in %s() line %d errno %d\n", func, __LINE__, errno);
		close(sd);
		return 0;
	}
	else if (listen(sd, MAX_CUSTOMERS) == -1)
	{
		printf("listen() failed in %s() line %d errno %d\n", func, __LINE__, errno); /*listen to at most 100 connections */
		close(sd);
		return 0;
	}
	else
	{
		ic = sizeof(senderAddr);
		/* accept loop, spawns a new client thread if a succesfull connection occurs , passes in the sd */
		while ((fd = accept(sd, (struct sockaddr *)&senderAddr, &ic)) != -1)
		{
			if (pthread_create(&tid, &kernelAttribute, client_session_thread, &fd) != 0)
			{
				printf("pthread_create() failed in %s()\n", func);
				return 0;
			}
			else
			{
				continue;
			}
		}
		close(sd);
		return 0;
	}
}

/*
 * Serve checks for account existence. If account is found, returns its index
 * in the bank struct array to be used for serve !
 */
int search_account(char* acc_name){
	for(int i = 0; i < MAX_CUSTOMERS; ++i)
	{
		if(strcmp(customers[i].accountName, acc_name) == 0)
		{
			return i;
		}
	}
	return NO_ACCOUNT;
}

int serve_account(char *acc_name)
{
	int index = search_account(acc_name);
	if(index > 0)
	{
		if(customers[index].isInUse == 0)
		{
			return index;
		}
		else
		{
			return ACCOUNT_IN_USE;
		}
	}
	return index;
}

int create_account(char *acc_name)
{
	if(search_account(acc_name) != NO_ACCOUNT)
	{
		return ACCOUNT_EXISTS;
	}
	/* go through bank struct */
	for (int i = 0; i < MAX_CUSTOMERS; i++)
	{
		if (strcmp(customers[i].accountName, "") == 0)
		{
			strcpy(customers[i].accountName, acc_name);
			return i;
		}
	}
	return SERVER_FULL;
}