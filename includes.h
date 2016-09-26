#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <arpa/inet.h>

#include <errno.h>

#include <iostream>
#include <fstream>

#include <ctime>

#include <netinet/in.h>
#include <netdb.h>

#include <resolv.h>
#include <arpa/nameser.h>

#include <stdio.h>

#include <string>
#include <set>
#include <algorithm>

using namespace std;

// ************************************************************************
// * Assigning a tag to each token just makes the code easier to read.
// ************************************************************************
#define HELO 1
#define MAIL 2
#define RCPT 3
#define DATA 4
#define RSET 5
#define NOOP 6
#define QUIT 7

// ************************************************************************
// * Local functions we are going to use.
// ************************************************************************
string readCommand(int sockfd);
int parseCommand(string commandString);
void* processConnection(void *arg);
