#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>

#define BUFSIZE 65535
#define HOST "127.0.0.1"
#define TIMEOUT 3

#ifdef DEBUG
#define debug(x) printf(x)
#else
#define debug(x) //
#endif

using namespace std;
 
class tcp_client
{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;
     
public:
    tcp_client();
    bool conn(string, int);
    bool send_data(string data);
    string receive(int size=BUFSIZE);
};
 
