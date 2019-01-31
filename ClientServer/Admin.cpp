#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <list>
#include <iterator>
#include <csignal>
#include <signal.h>
#include <sys/wait.h>

using namespace std;


struct commArgs{
	char inbuf;
	int incomingPipe[2], outgoingPipe[2], sleepTime;
};


struct ReadyQueueValues{
	int sockfd, clientId, portNumber;
	string fileName;
};

typedef struct commArgs commStruct;


list <int> socketDescriptors;
list <int> tempDescriptors;
list <int> portNumbersList;
list <ReadyQueueValues> incomingReadyQueue;
list <ReadyQueueValues> queueForDumping;
int pipeDescriptor;


int startServer(int portNumber);
void *adminCommunication(void *commArgs);
void *serverConnection(void *connArgs);
void *serverCommunication(void *readArgs);
void clientCommunication(int sleepTime);
void dumpQueue(int signal);

int main(int argc, char **argv) {
	if(argc<3){
	printf("Please Check Arguments\n");
	exit(0);
	}

	int rpid, pid, incomingPipe[2], outgoingPipe[2], sleepTime = atoi(argv[2]), portNumber = atoi(argv[1]);
	string id;
	char inbuf;
	char outbuf[4096];

	pipe(incomingPipe);
	pipe(outgoingPipe);

	rpid = fork();

		if( rpid < 0 ){
				cout << "Fork Failed." + id << endl;
		}else if( rpid == 0 ){
			signal(SIGRTMIN, dumpQueue);
			id = "Computer";
			pid = getpid();
			cout << id << ", " << rpid << ", " << pid << endl;
			cout << "Computer process server socket ready" << endl;
			int sockfd = startServer(portNumber);

			pthread_t adminThread, connectThread, readThread;
			commStruct *commArgs;
			commArgs = (commStruct *)malloc(sizeof(commStruct));

			commArgs->inbuf = inbuf;
			commArgs->sleepTime = sleepTime;
			commArgs->incomingPipe[0] = incomingPipe[0];
			commArgs->incomingPipe[1] = incomingPipe[1];
			commArgs->outgoingPipe[0] = outgoingPipe[0];
			commArgs->outgoingPipe[1] = outgoingPipe[1];

			pthread_create(&adminThread, NULL, adminCommunication, (void *)commArgs);
			pthread_create(&connectThread, NULL, serverConnection, &sockfd);
			pthread_create(&readThread, NULL, serverCommunication, NULL);

			pthread_join(adminThread, NULL);
			pthread_join(connectThread, NULL);
			pthread_join(readThread, NULL);

		}else if( rpid >0 ){
			sleep(1);
			id = "Admin";
			pid = getpid();
			cout << id << ", " << rpid << ", " << pid << endl;
			close(incomingPipe[0]);
			close(outgoingPipe[1]);
			do {
				cout << "Admin command: ";
				cin >> inbuf;
				write(incomingPipe[1], &inbuf, 1);
				if(inbuf == 'Q' ){
					kill(rpid,SIGRTMIN);
					bzero(outbuf, sizeof(outbuf));
					read(outgoingPipe[0],&outbuf,sizeof(outbuf));
					cout << outbuf << endl;
				}else if(inbuf == 'x'){

				}
			} while (inbuf != 'T');


			if(inbuf == 'T'){
				cout << "\nExiting program...\n\n";
				close(incomingPipe[1]);
				close(outgoingPipe[0]);
				exit(EXIT_SUCCESS);
			}

		}
		return 0;
}


int startServer (int PortNumber){

	int sockfd, binding;
	struct sockaddr_in serverAddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("Error in connection\n");
		exit(1);
	}
	printf("Server Socket is created\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PortNumber);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	// binding to a port
	binding = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if(binding < 0){
		printf("Error in binding\n");
		exit(1);
	}

	// listening
	if(listen(sockfd, SOMAXCONN) == 0){
		printf("Listening....\n");
	}else{
		printf("Error in Listening\n");
	}
	return sockfd;
}

void *adminCommunication(void *commArgs){

	commStruct *args;
	args = (commStruct *)commArgs;
	pipeDescriptor = args->outgoingPipe[1];

	close(args->incomingPipe[1]);
	close(args->outgoingPipe[0]);

	do {
		read(args->incomingPipe[0], &args->inbuf, 1);
		if(args->inbuf == 'Q' ){

		}else if(args->inbuf == 'x'){
			clientCommunication(args->sleepTime);
		}
	} while (args->inbuf !='T');

	if(args->inbuf == 'T'){
		close(args->incomingPipe[0]);
		close(args->outgoingPipe[1]);
		exit(EXIT_SUCCESS);
	}
	return NULL;
}

void *serverConnection(void *connArgs){
	int newSockfd;
	int *sockfd = (int *)connArgs;
	socklen_t addr_size;
	struct sockaddr_in newAddr;

	while(1){
		newSockfd = accept(*sockfd, (struct sockaddr*)&newAddr, &addr_size);
		if(newSockfd < 0){
			printf("Connection failed\n");
			exit(1);
		}
		//printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
		portNumbersList.push_back(ntohs(newAddr.sin_port));
		socketDescriptors.push_back(newSockfd);
		tempDescriptors.push_back(newSockfd);
	}
	return NULL;
}

void *serverCommunication(void *readArgs){
	list <int> :: iterator it;
	char buffer[4096];
	while(1){
		int i=0;
	for(it = socketDescriptors.begin(); it != socketDescriptors.end(); ++it){
		bzero(buffer,sizeof(buffer));
		recv(*it, buffer, 4096, 0);
		string data = buffer;
		int pos = data.find_first_of(';');
		string fileName = data.substr(0,pos);
		string clientId = data.substr(pos+1);
		if(strcmp(fileName.c_str(), "nullfile") == 0){
			socketDescriptors.remove(*it);
			portNumbersList.pop_front();
			tempDescriptors.empty();
			tempDescriptors.assign(socketDescriptors.begin(),socketDescriptors.end());
			break;
		}else{
			ReadyQueueValues readyQueueValues = ReadyQueueValues();
			readyQueueValues.clientId = atoi(clientId.c_str());
			readyQueueValues.fileName = fileName;
			readyQueueValues.sockfd = *it;
			list<int> :: iterator it2 = portNumbersList.begin();
			advance(it2, i);
			readyQueueValues.portNumber = *it2;

			incomingReadyQueue.push_back(readyQueueValues);
			queueForDumping.push_back(readyQueueValues);
		}
		i++;
		}

	}
	return NULL;
}


void clientCommunication(int sleepTime){
	list <ReadyQueueValues> :: iterator it;
	char buffer[4096];
	for(it = incomingReadyQueue.begin(); it != incomingReadyQueue.end(); ++it){
		bzero(buffer,sizeof(buffer));
		int number = 0;
		int sum = 0;
		string firstLine;
		int lines = 0;

		// reading file contents
		fstream file(it->fileName+".txt");

		// sum calculation
		getline( file, firstLine );
		while(file >> number && lines < atoi(firstLine.c_str())){
			sum += number;
			lines++;
			usleep(sleepTime*1000);
		}
		string clientOutput = to_string(it->clientId)+", "+it->fileName+", "+to_string(sum);
		strcpy(buffer, clientOutput.c_str());
		send(it->sockfd, buffer, strlen(buffer), 0);
		queueForDumping.pop_front();
	}
	queueForDumping.clear();
	incomingReadyQueue.clear();
}

void dumpQueue(int signal){
	char outbuf[4096];
	bzero(outbuf,sizeof(outbuf));
	list <ReadyQueueValues> :: iterator it;
	if(queueForDumping.empty()){
		write(pipeDescriptor,"Empty Ready Queue!",sizeof("Empty Ready Queue!"));
	}else{
		string output = "";
		for(it = queueForDumping.begin(); it != queueForDumping.end(); it++){
			output = output + to_string(it->clientId) + ", " + it->fileName + ", " + to_string(it->sockfd)+", "+to_string(it->portNumber)+"\n";

		}
		output = output.substr(0, output.size()-1);
		strcpy(outbuf, output.c_str());
		write(pipeDescriptor,&outbuf,sizeof(outbuf));
	}

}
