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

int init_server(string port_s){
    int port;
    stringstream port_ss(port_s);
    port_ss >> port;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
       
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
       
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    cout<<"\033[1;32mWAITING FOR CONNECTION ON PORT "<<port<<"...\033[0m"<<endl;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    cout<<"\033[1;32mCONNECTED\033[0m"<<endl;
    return new_socket;
}

int main(int argc, char *argv[])
{
    int logged_in = 0;
    int username;
    int serverfd = init_client(string(argv[1]));
    int browserfd = init_server(string(argv[2]));
    cout<<"BOTH CONNECTION INITIALIZED"<<endl;
    fd_set readfds;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);
        FD_SET(browserfd, &readfds);
        int max_sd = max(serverfd, browserfd);
        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
        if ((activity < 0) && (errno!=EINTR))printf("select error"); 

        if(FD_ISSET(browserfd, &readfds)){
            http_request r = get_http_request(browserfd);
            cout<<"\033[1;36mREQUEST RECEIVED\033[0m"<<endl;
            r.display();
            if(logged_in == 0){
                if(r.method == "POST"){
                    if(r.action == "/login"){
                        map<string, string> m = process_form_data(r.content);
                        send_str(serverfd, "login "+m["username"]+" "+m["passwd"]);
                        string res = recv_str(serverfd);
                        if(res == "0"){
                            logged_in = 1;
                            cout<<"\033[1;32mLOGGED IN!!\033[0m"<<endl;
                            send_http(browserfd, "./static/main.html", "text/html");
                        }else{
                            cout<<"\033[1;31mFAILED LOGIN!!\033[0m"<<endl;
                            send_http(browserfd, "./static/login.html", "text/html");
                        }
                        /*string s = "HTTP/1.1 303 See Other\r\nLocation: wwsdfgsdfgsdgf\r\n\r\n";
                        write(browserfd, s.c_str(), s.length());*/
                        
                    }else if(r.action == "/signup"){
                        map<string, string> m = process_form_data(r.content);
                        send_str(serverfd, "signup "+m["username"]+" "+m["passwd"]);
                        string res = recv_str(serverfd);
                        if(res == "0"){
                            logged_in = 1;
                            cout<<"\033[1;32mACCOUNT CREATED!!\033[0m"<<endl;
                            send_http(browserfd, "./static/main.html", "text/html");
                        }else{
                            cout<<"\033[1;31mFAILED SIGNUP!!\033[0m"<<endl;
                            send_http(browserfd, "./static/login.html", "text/html");
                        }
                    }
                }else if(r.method == "GET"){
                    if(r.action.length() >= 7 && r.action.substr(0,7) == "/image/"){
                        send_http(browserfd, "."+r.action, "image/png");
                    }else if(r.action == "/signup"){
                        send_http(browserfd, "./static/signup.html", "text/html");
                    }else if(r.action.length()>=8 && r.action.substr(0,8)=="/static/"){
                        send_http(browserfd, "."+r.action, "text/html");
                    }else{
                        send_http(browserfd, "./static/login.html", "text/html");
                    }
                }
            }else{
                if(r.method == "GET"){
                    if(r.action.length() >= 7 && r.action.substr(0,7) == "/image/"){
                        send_http(browserfd, "."+r.action, "image/png");
                    }else if(r.action.length()>=8 && r.action.substr(0,8)=="/static/"){
                        send_http(browserfd, "."+r.action, "text/html");
                    }else{
                        send_http(browserfd, "./static/main.html", "text/html");
                    }
                }
            }
        }
    }

    return 0;
}