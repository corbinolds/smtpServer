#include "includes.h"

#define MAXLINE 1024
#define PORT 9873
#define DEBUG 1

// ***************************************************************************
// * Read the command from the socket.
// *  Simply read a line from the socket and return it as a string.
// ***************************************************************************
string readCommand(int sockfd) {
	char* buffer = new char[400];

	//resets buffer to all 0s
	bzero(buffer, 400);
	
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
	
	if (theString == "HELO") {
		return 1;
	}
	else if (theString == "MAIL FROM") {
		return 2;
	}
	else if (theString == "RCPT TO") {
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
		return 7;
	}
	
}

//command to write to a socket
void writeCommand(int sockfd, string message) {
	int size = message.length();
	write(sockfd, message.c_str(), size);
	
	return;
}

//part of getting the MX record. Credit goes to http://www.sourcexr.com/articles/2013/10/12/dns-records-query-with-res_query
string parse_record (unsigned char *buffer, size_t r, const char *section, ns_sect s, int idx, ns_msg *m) {

	ns_rr rr;
    	int k = ns_parserr (m, s, idx, &rr);
    	if (k == -1) {
        	return "ERROR";
    	}

    	const size_t size = NS_MAXDNAME;
    	unsigned char name[size];
    	int t = ns_rr_type (rr);

    	const u_char *data = ns_rr_rdata (rr);
    	if (t == T_MX) {
        	int pref = ns_get16 (data);
        	ns_name_unpack (buffer, buffer + r, data + sizeof (u_int16_t),
                        name, size);
        	char name2[size];
        	ns_name_ntop (name, name2, size);
        	return name2;
    	}

	else {
		return "ERROR";
	}
}

//part of getting the MX record. Credit goes to http://www.sourcexr.com/articles/2013/10/12/dns-records-query-with-res_query
string getTheHost(string domain) {
	const size_t size = 1024;
    	unsigned char buffer[size];

    	const char *host = domain.c_str();

    	int r = res_query (host, C_IN, T_MX, buffer, size);
	if (r == -1) {
     		return "ERROR";
	}
    	else {
		if (r == static_cast<int> (size)) {
            		return "ERROR";
        	}
    	}
    	
    	
    	HEADER *hdr = reinterpret_cast<HEADER*> (buffer);
	
	if (hdr->rcode != NOERROR) {

	        switch (hdr->rcode) {
	        case FORMERR:
	        	cerr << "Format error";
            		break;
        	case SERVFAIL:
        		cerr << "Server failure";
            		break;
        	case NXDOMAIN:
            		cerr << "Name error";
            		break;
        	case NOTIMP:
            		cerr << "Not implemented";
            		break;
        	case REFUSED:
            		cerr << "Refused";
            		break;
        	default:
            		cerr << "Unknown error";
        	}
        	return "ERROR";
    	}
    	
    	int question = ntohs (hdr->qdcount);
    	int answers = ntohs (hdr->ancount);
    	int nameservers = ntohs (hdr->nscount);
    	int addrrecords = ntohs (hdr->arcount);

	ns_msg m;
    	int k = ns_initparse (buffer, r, &m);
    	if (k == -1) {
        	return "ERROR";
    	}


	for (int i = 0; i < question; ++i) {
        	ns_rr rr;
        	int k = ns_parserr (&m, ns_s_qd, i, &rr);
        	if (k == -1) {
            		return "ERROR";
        	}
    	}
    	for (int i = 0; i < answers; ++i) {
        	return parse_record (buffer, r, "answers", ns_s_an, i, &m);
    	}

    	for (int i = 0; i < nameservers; ++i) {
        	return parse_record (buffer, r, "nameservers", ns_s_ns, i, &m);
    	}

    	for (int i = 0; i < addrrecords; ++i) {
        	return parse_record (buffer, r, "addrrecords", ns_s_ar, i, &m);
    	}
    	
    	
}

//Executes all the commands on the foreign host needed to send an email with all of the data the user already inputted in my smtp server
string sendMail(int sockfd, string recipient, string sender, string message, string domain) {
	string read = "";
	//after connection, read connect message
    	read = readCommand(sockfd);
    	cout << read << endl;
    	//if doesn't equal connection successful code, break
    	if(read.substr(0, 3) != "220"){
    		string error = "CONNECTION ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}

	//Say HELO to the host
	writeCommand(sockfd, "HELO " + domain + "\r\n");
	read = readCommand(sockfd);
    	cout << read << endl;
	//if doesn't equal ok code or does not equal command parameter not implemented code (needed for janus and cardea), break
	if(read.substr(0, 3) != "250" && read.substr(0,3) != "504"){
		string error = "HELO ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}

	//Specify MAIL FROM on the host
	writeCommand(sockfd, "MAIL FROM:<" + sender + ">\r\n");
	read = readCommand(sockfd);
	cout << read << endl;
	//if doesn't equal ok code, break
	if(read.substr(0, 3) != "250"){
		string error = "MAIL FROM ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}
    	
    	//Specify RCPT TO on the host
	writeCommand(sockfd, "RCPT TO:<" + recipient + ">\r\n");
	read = readCommand(sockfd);
	cout << read << endl;
	//if doesn't equal ok code, break
	if(read.substr(0, 3) != "250"){
		string error = "RCPT TO ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}
    	
    	//Specify start data input on the host
	writeCommand(sockfd, "DATA\r\n");
	read = readCommand(sockfd);
	cout << read << endl;
	//if doesn't equal start mail input code, break
	if(read.substr(0, 3) != "354"){
		string error = "START DATA INPUT ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}
    	
    	//Actually put in the message on the host for the DATA input
	writeCommand(sockfd, message + "\r\n.\r\n");
	read = readCommand(sockfd);
	cout << read << endl;
	//if doesn't equal ok code, break
	if(read.substr(0, 3) != "250"){
		string error = "DATA INPUT ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}
    	
    	//Disconnect from the host
	writeCommand(sockfd, "QUIT\r\n");
	read = readCommand(sockfd);
    	cout << read << endl;
	//If doesn't equal disconnect code, break
	if(read.substr(0, 3) != "221"){
		string error = "QUIT ERROR\n";
    		cout << error;
    		close(sockfd);
    		return error;
    	}
    	close(sockfd);
    	return "";
}

//command called when RCPT domain is not localhost, opens a socket to the remote host and gives it all the email information. credit for socket handling to http://www.linuxhowtos.org/C_C++/socket.htm
string connectToSecondarySMTP(string recipient, string sender, string message, string domain) {	
		//handles opening the socket
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];

	portno = 25;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
    	cout << "ERROR opening socket" << endl;
    }
    string hostname = getTheHost(domain);
    
    if(hostname.c_str() == "ERROR"){
    	return "Error with recipient's host name";
    }
    else{
    	server = gethostbyname(hostname.c_str());
    }
	if (server == NULL) {
	    fprintf(stderr,"ERROR, no such host\n");
	    exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
	     (char *)&serv_addr.sin_addr.s_addr,
	     server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
	    cout << "ERROR connecting" << endl;
	bzero(buffer,256);

    return sendMail(sockfd,recipient, sender, message, domain);
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
	
	//strings used to store MAIL FROM, RCPT TO, username, host, and input values throughout the commands
	string forwardPath = "";
	string reversePath = "";
	string cmdString = "";
	string getString = "";
	string parameter = "";
	string messageBuffer = "";
	string username = "";
	string hostname = "";
	int findAtSymbol = 0;
	
	//used for displaying time in localhost messages
	time_t currentTime;
	
	string dataRead = "";
	ofstream ofs;
	
	writeCommand(sockfd, "220 Connection established\n");
	while (connectionActive) {


		// *******************************************************
		// * Read the command from the socket.
		// *******************************************************
		string getString = readCommand(sockfd);
		int findColon = getString.find(":");
		
		cmdString = getString.substr(0, findColon);
		parameter = getString.substr(findColon + 1, getString.length() - findColon);

		// *******************************************************
		// * Parse the command.
		// *******************************************************
		int command = parseCommand(cmdString);

		// *******************************************************
		// * Act on each of the commands we need to implement. 
		// *******************************************************
		switch (command) {
		case HELO :
			writeCommand(sockfd, "250 Corbin's smtp server Hello\n");
			break;
		case MAIL :
			seenMAIL = 1;
			seenRCPT = 0;
			seenDATA = 0;
		
			forwardPath = "";
			reversePath = parameter.substr(1, parameter.length() - 2);
			messageBuffer = "";
			
			writeCommand(sockfd, "250 OK\n");
			break;
		case RCPT :
			if(seenMAIL){
			
				seenRCPT = 1;
				forwardPath = parameter.substr(1, parameter.length() - 2);
			
				writeCommand(sockfd, "250 OK\n");
			}
			else{
				writeCommand(sockfd, "503 Need MAIL FROM command\n");
			}
			
			break;
		case DATA :
			if(seenMAIL && seenRCPT){
				messageBuffer = "";
			
				writeCommand(sockfd, "354 Send message content; end with <CRLF>.<CRLF>\n");
				dataRead = readCommand(sockfd);
				while(dataRead != "."){
					messageBuffer.append(dataRead + "\n");
					dataRead = readCommand(sockfd);
				}
				
				currentTime = time(NULL);
				
				//removes the last new line from the message
				messageBuffer = messageBuffer.substr(0, messageBuffer.length() - 1);
				
				findAtSymbol = forwardPath.find("@");
				username = forwardPath.substr(0, findAtSymbol);
				hostname = forwardPath.substr(findAtSymbol + 1, forwardPath.length() - findAtSymbol);

				//if domain is local, message gets stored in the server
				if (hostname == "localhost"){
					ofs.open(username.c_str(), ios_base::app);
					ofs << "From " + reversePath << endl;
					ofs << asctime(localtime(&currentTime));
					ofs << messageBuffer << "\n";
					ofs.close();
					writeCommand(sockfd, "250 OK\n");
				}
				//if domain is not local, connect to the remote server and send the email with smtp commands through it.
				else {
					writeCommand(sockfd, "251 User not local; will forward to <" + hostname + ">\n");
					for(int i = 0; i< 10; i++) {
						string status = connectToSecondarySMTP(forwardPath, reversePath, messageBuffer, hostname);
						messageBuffer += messageBuffer + "!"
						if (status == ""){
							writeCommand(sockfd, "250 OK\n");
						}
						else {
							writeCommand(sockfd, status);
						}
					}

				}

				seenDATA = 1;

				break;
			}
			else {
				writeCommand(sockfd, "Must specify RCPT TO and MAIL FROM first\n");
				break;
			}
		case RSET :
			seenMAIL = 0;
			seenRCPT = 0;
			seenDATA = 0;
			forwardPath = "";
			reversePath = "";
			messageBuffer = "";
			
			writeCommand(sockfd, "250 OK\n");
			break;
		case NOOP :
			break;
		case QUIT :
			writeCommand(sockfd, "221 Connection closing\n");
			exit(1);
			break;
		default :

			cout << "Unknown command (" << cmdString << ")" << endl;
			writeCommand(sockfd, "500 Unkown command(" + cmdString + ")\n");
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
