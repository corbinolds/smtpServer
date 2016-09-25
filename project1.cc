#include "includes.h"

#define MAXLINE 1024
#define PORT 9874
#define DEBUG 1

// ***************************************************************************
// * Read the command from the socket.
// *  Simply read a line from the socket and return it as a string.
// ***************************************************************************
string readCommand(int sockfd) {
	char* buffer = new char[10];

	read(sockfd, buffer, 400);
	
	// Removes last 2 characters from the buffer
	if('\n' == buffer[strlen(buffer) - 1])
    		buffer[strlen(buffer) - 2] = '\0';
    		
	return buffer;
}

// ***************************************************************************
// * Parse the command.
// *  Read the string and find the command, returning the number we assoicated
// *  with that command.
// ***************************************************************************
int parseCommand(string commandString) {
	string theString(commandString);
	
	if (theString == "HELO"){
		return 1;
	}
	else if (theString == "MAIL") {
		return 2;
	}
	else if (theString == "RCPT") {
		return 3;
	}
	else if (theString == "DATA") {
		return 4;
	}
	else if (theString == "RSET") {
		return 5;
	}
	else if (theString == "NOOP") {
		return 6;
	}
	else if (theString == "QUIT") {
		cout << "why" << endl;
		return 7;
	}
	else {
		//cout << theString << endl;
	}
	
	
}

// ***************************************************************************
// * processConnection()
// *  Master function for processing thread.
// *  !!! NOTE - the IOSTREAM library and the cout varibables may or may
// *      not be thread safe depending on your system.  I use the cout
// *      statments for debugging when I know there will be just one thread
// *      but once you are processing multiple rquests it might cause problems.
// ***************************************************************************
void* processConnection(void *arg) {


	// *******************************************************
	// * This is a little bit of a cheat, but if you end up
	// * with a FD of more than 64 bits you are in trouble
	// *******************************************************
	int sockfd = *(int *)arg;
	if (DEBUG)
		cout << "We are in the thread with fd = " << sockfd << endl;

	int connectionActive = 1;
	int seenMAIL = 0;
	int seenRCPT = 0;
	int seenDATA = 0;
	string forwardPath = "";
	string reversePath = "";
	char *messageBuffer = NULL;
	while (connectionActive) {


		// *******************************************************
		// * Read the command from the socket.
		// *******************************************************
		string cmdString = readCommand(sockfd);

		// *******************************************************
		// * Parse the command.
		// *******************************************************
		int command = parseCommand(cmdString);

		// *******************************************************
		// * Act on each of the commands we need to implement. 
		// *******************************************************
		switch (command) {
		case HELO :
			break;
		case MAIL :

			break;
		case RCPT :
			break;
		case DATA :
			break;
		case RSET :
			break;
		case NOOP :
			break;
		case QUIT :

			exit(1);
			break;
		default :

			cout << "Unknown command (" << command << ")" << endl;

		}
	}

	if (DEBUG)
		cout << "Thread terminating" << endl;

}


// ***************************************************************************
// * Main
// ***************************************************************************
int main(int argc, char **argv) {

       if (argc != 1) {
                cout << "useage " << argv[0] << endl;
                exit(-1);
        }

	// *******************************************************************
	// * Creating the inital socket is the same as in a client.
	// ********************************************************************
	int     listenfd = -1;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "Failed to create listening socket " << strerror(errno) <<  endl;
		exit(-1);
	}


	// ********************************************************************
	// * The same address structure is used, however we use a wildcard
	// * for the IP address since we don't know who will be connecting.
	// ********************************************************************
	struct sockaddr_in  servaddr;
	
	bzero(&servaddr, sizeof(servaddr));
	
	servaddr.sin_family = PF_INET;
	
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	servaddr.sin_port = htons(PORT);
	
	if (bind(listenfd, (sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		cout << "bind() failed: " << strerror(errno) <<  endl;
		exit(-1);
	}
	
	int listenq = 3;
	if (listen(listenfd, listenq) < 0) {
		cout << "listen() failed: " << strerror(errno) <<  endl;
		exit(-1);
	}

	// ********************************************************************
	// * Binding configures the socket with the parameters we have
	// * specified in the servaddr structure.  This step is implicit in
	// * the connect() call, but must be explicitly listed for servers.
	// ********************************************************************
	if (DEBUG)
		cout << "Process has bound fd " << listenfd << " to port " << PORT << endl;


	// ********************************************************************
        // * Setting the socket to the listening state is the second step
	// * needed to being accepting connections.  This creates a que for
	// * connections and starts the kernel listening for connections.
        // ********************************************************************
	if (DEBUG)
		cout << "We are now listening for new connections" << endl;


	// ********************************************************************
        // * The accept call will sleep, waiting for a connection.  When 
	// * a connection request comes in the accept() call creates a NEW
	// * socket with a new fd that will be used for the communication.
        // ********************************************************************
	set<pthread_t*> threads;
	while (1) {
		if (DEBUG)
			cout << "Calling accept() in master thread." << endl;
			
		int* connfd = new int;
		*connfd = -1;

		if ((*connfd = accept(listenfd, (sockaddr *) NULL, NULL)) < 0) {
			cout << "accept() failed: " << strerror(errno) <<  endl;
			exit(-1);
		}
	
		if (DEBUG)
			cout << "Spawing new thread to handled connect on fd=" << connfd << endl;

		pthread_t* threadID = new pthread_t;
		pthread_create(threadID, NULL, processConnection, (void *)connfd);
		threads.insert(threadID);
	}
}
