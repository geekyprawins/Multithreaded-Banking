#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1" // MODIFY LATER
#define SERVER_PORT 35532

int clientSocket = -1;
char buffer[256], buffer2[100];
char name[100];

const char* input_string(char* string, int capacity);
void press_enter();
void clear_screen();
void send_and_recv();
void load_from_file(const char* fileName);

int sock_to_server();
void bank_menu();

void create_account();

void login();
void access_account();
void withdraw();
void deposit();
void transfer();

void credit_screen();
void disconnection();

int main()
{
	clientSocket = sock_to_server();
	bank_menu(clientSocket);
	credit_screen();
	return 0;
}

// Function for inputting strings that may contain spaces
const char* input_string(char* string, int capacity)
{
	fgets(string, capacity, stdin);
	size_t pos = strcspn(string, "\n");
	if(string[pos] == '\n')
	{
		string[pos] = '\0';
	}
	else
	{
		int ch;
		while((ch=getchar())!='\n' && ch!=EOF)
		{}
	}
	return string;
}

// Function that keeps taking input until user presses enter
void press_enter()
{
	printf("Press enter to continue...");
	input_string(buffer, 1);
}

// Function that clears terminal screen similiar to clear command
void clear_screen()
{
	// \x1B[2J		-> Escape sequence to clear screen
	// \x1B[0;0H	-> Escape sequence to move cursor to position (0, 0)
	printf("\x1B[2J\x1B[0;0H");

	// Flush standard output so that is is immediately visible
	fflush(stdout);
}

int sock_to_server()
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock < 3)
	{
		printf("Error: Unable to create a socket. Terminating program...\n");
		exit(EXIT_FAILURE);
	}

	const char* ip = SERVER_IP;
	in_port_t port = SERVER_PORT;

	struct sockaddr_in sockaddr_in;
	memset(&sockaddr_in, 0, sizeof sockaddr_in);
	sockaddr_in.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &sockaddr_in.sin_addr.s_addr);
	sockaddr_in.sin_port = htons(port);
	
	do
	{
		printf("\nAttempting to connect to banking server...\n");
		sleep(1);
		if (connect(sock, (struct sockaddr*)&sockaddr_in, sizeof sockaddr_in) == 0)
		{
			break;
		}

		close(sock);
		printf("Error: Could not connect to server. Trying again in 3 seconds...\n");
		sleep(2);

	} while(1);
	
	printf("Successfully connected to banking server.\n\n");
	press_enter();
	return sock;
}

void send_and_recv(){
	int length = strlen(buffer)+1;
	if(write(clientSocket, buffer, length) != length 
		|| read(clientSocket, buffer, sizeof(buffer)) < 0)
	{
		disconnection();
	}
}

void bank_menu()
{
	char option;
	do
	{
		clear_screen();
		printf("\nSure Ya Bank Menu\n");
		printf("\nOptions\n");
		printf("\na. Create an account\n");
		printf("b. Login\n");
		printf("c. Exit\n");
		printf("\nEnter your choice: ");
		input_string(buffer, 2);
		option = tolower(buffer[0]);
		switch(option)
		{
		case 'a':
			create_account();
			break;
		case 'b':
			login();
			break;
		case 'c':
			strcpy(buffer, "quit");
			send_and_recv();
			break;
		default:
			printf("Error: Invalid option.\n\n");
			press_enter();
		}
	}while(option != 'c');
}

void login()
{
	clear_screen();
	printf("\nSure Ya Bank Account Login\n");
	printf("\nEnter name: ");
	input_string(name, 100);
	sprintf(buffer, "serve %s", name);
	send_and_recv();

	if(strncmp(buffer, "Success: ", 9) == 0)
	{
		access_account();
	} 
	else
	{
		printf("%s\n\n", buffer);
		press_enter();
	}
}

void access_account()
{
	char option;
	int length;
	do{
		sprintf(buffer, "query %s", name);
		send_and_recv();

		clear_screen();
		printf("\nAccessing Sure Ya Bank Account\n");
		printf("%s\n\n", buffer);
		printf("Options\n");
		printf("\na. Withdraw Money\n");
		printf("b. Deposit Money\n");
		printf("c. Transfer Money\n");
		printf("d. Log Out\n");
		printf("\nEnter your choice: ");
		input_string(buffer, 2);
		option = tolower(buffer[0]);
		switch(option){
		case 'a':
			printf("Enter amount to withdraw: ");
			input_string(buffer2, 20);
			sprintf(buffer, "withdraw %s", buffer2);
			break;
		case 'b':
			printf("Enter amount to deposit: ");
			input_string(buffer, 20);
			sprintf(buffer, "deposit %s", buffer2);
			break;
		case 'c':
			/*
			printf("Enter account name to transfer to: ");
			input_string(buffer2, 100);
			printf("\nEnter amount to transfer: ");
			input_string(buffer, 20);
			*/
			break;
		case 'd':
			strcpy(buffer, "end");
			break;
		default:
			printf("Error: Invalid choice.\n\n");
			press_enter();
		}

		if('a' <= option && option <= 'd'){
			send_and_recv();
			printf("\n%s\n\n", buffer);
			press_enter();
		}
	}while(option != 'd');
	
	strcpy(buffer, "end");
	length = strlen(buffer)+1;
	if(write(clientSocket, buffer, length) != length)
	{
		disconnection();
	}
}

void create_account(){
	clear_screen();
	printf("\nCreating a Sure Ya Bank Account\n");
	printf("\nEnter name: ");
	input_string(name, 100);
	sprintf(buffer, "create %s", name);
	int length = strlen(buffer)+1;
	if(write(clientSocket, buffer, length) != length 
		|| read(clientSocket, buffer, sizeof(buffer)) < 0)
	{
		disconnection();
	}
	printf("%s\n\n", buffer);
	press_enter();
}

void load_from_file(const char* fileName){

}

void credit_screen(){
	clear_screen();
	load_from_file("credit.txt");
}

void disconnection(){
	clear_screen();
	load_from_file("disconnect.txt");
	close(clientSocket);
	exit(EXIT_FAILURE);
}