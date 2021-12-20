#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include "transfer.h"
#include <algorithm>

#define BUF_SIZE 1024

using namespace std;

int init_client(string IP_port){
    int sock;
    struct sockaddr_in serv_addr;
    string IPstr;
    int port_num;
    for(int i=0;i<IP_port.length();i++){
        if(IP_port[i]==':'){
            IPstr = IP_port.substr(0,i);
            stringstream port_ss(IP_port.substr(i+1)); 
            port_ss >> port_num;
            break;
        }
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);

    inet_pton(AF_INET, IPstr.c_str(), &serv_addr.sin_addr);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    return sock;
}

int main(int argc, char *argv[])
{
    int sockfd = init_client(string(argv[1]));
    while(1){
        string c = recv_str(sockfd);
        if(c == "You want to quit\n")return 0;
        cout<<c;
        cin>>c;
        send_str(sockfd, c);
    }
    return 0;
}