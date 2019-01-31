#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <netdb.h>

using namespace std;


int main(int argc, char **argv){

	if(argc<4){
	printf("Please Check Arguments\n");
	exit(0);
	}

	// variables declaration
	int clientSocket, ret;
	struct sockaddr_in serverAddr;
	struct hostent *server;
	char buffer[4096];

	// creation of a socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSocket < 0){
		printf("Error in connection\n");
		exit(1);
	}
	printf("Client Socket is created.\n");
	server = gethostbyname(argv[2]);
	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[3]));
	bcopy((char *)server->h_addr,(char *)&serverAddr.sin_addr.s_addr, server->h_length);

	// connect to a server
	ret = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(ret < 0){
		printf("Error in connection\n");
		exit(1);
	}
	printf("Connected to Server\n");

	while(1){
		bzero(buffer, sizeof(buffer));
		printf("Input file name: ");
		scanf("%s", buffer);
		strcat(buffer, ";");
		strcat(buffer, argv[1]);
		send(clientSocket, buffer, strlen(buffer), 0);
		if(string(buffer).rfind("nullfile",0)== 0){
			printf("Disconnected from server\n");
			close(clientSocket);
			exit(1);
		}
		bzero(buffer, sizeof(buffer));
		if(recv(clientSocket, buffer, 1024, 0) < 0){
			printf("Error in receiving data\n");
		}else{
			printf("%s\n", buffer);
		}
	}

	return 0;
}
