#include "tcp.h"

tcp_client::tcp_client()
{
    sock = -1;
    address = "";
}
 
bool tcp_client::conn(string address , int port)
{
    if(port == 0 ) {
        cout << "Error : Port " << port << " invalid";
        return false;
    }

    if(sock == -1)
    {
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
            perror("Could not create socket");
            return false;
        }
        debug("Socket created\n");
    }

    // Set Timeout
    struct timeval tv = {TIMEOUT,0};
    setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv));

    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *he;
        struct in_addr **addr_list;
         
        if ( (he = gethostbyname( address.c_str() ) ) == NULL)
        {
            herror("gethostbyname");
            debug("Failed to resolve hostname\n");
            return false;
        }
         
        addr_list = (struct in_addr **) he->h_addr_list;
        for(int i = 0; addr_list[i] != NULL; i++)
        {
            server.sin_addr = *addr_list[i];
            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;
            break;
        }
    } else {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    }
     
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
     
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return false;
    }
     
    debug("Connected\n");
    return true;
}
 
bool tcp_client::send_data(string data)
{
    if( send(sock , data.c_str() , data.length() , 0) < 0)
    {
        perror("Send failed : ");
        return false;
    }
    return true;
}
 
string tcp_client::receive(int size)
{
    char buffer[size];
    string reply;
    int len=0;
     
    len = recv(sock , buffer , sizeof(buffer) , 0);
    if( len < 0 )
    {
        puts("recv failed");
        return "";
    } else {
        buffer[len+1]='\0'; 
        reply = buffer;
        return reply;
    }
}
 
