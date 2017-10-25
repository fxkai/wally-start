#include "tcp.h"    // connect class

int main(int argc , char *argv[])
{
    tcp_client c;
    string data;
    int port = 0;

    if(argc < 2) {
        cout << "Usage : "<< argv[0] << " <port> [commands, ...]\n";
        return 1;
    }

    if(!c.conn(HOST , strtol(argv[1],NULL,10))){
        return 1;
    }

    // Read from stdin OR concat all arguments following the port argument
    if(argc < 3){
        while(cin) {
            string line;
            getline(cin, line);
            data+=line;
            if(line.length() > 0)
                data+="\n";
        }
    } else {
        for(int i = 2; i < argc; i ++){
            data += string(argv[i]);
            if(i < argc -1){
                data += " ";
            }
        }
        data += "\n";
    }

    c.send_data(data);
    data = c.receive();
    cout <<data;
     
    return 0;
}
