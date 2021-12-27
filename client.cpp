#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include "transfer.h"
#include <algorithm>
#include <filesystem>

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

int init_server(string port_s, struct sockaddr_in &address, int &addrlen){
    int port;
    stringstream port_ss(port_s);
    port_ss >> port;
    int server_fd, new_socket;
    int opt = 1;
       
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
       
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,  sizeof(address))<0)
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
    return server_fd;
}

int main(int argc, char *argv[])
{
    int logged_in = 0;
    int username;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int serverfd = init_client(string(argv[1]));
    int listenfd = init_server(string(argv[2]), address, addrlen);
    int browserfd;
    int browser_connected = 0;
    cout<<"\033[1;32mSERVER CONNECTED\033[0m"<<endl;
    cout<<"\033[1;36mLISTENING FOR BROWSER...\033[0m"<<endl;
    fd_set readfds;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);
        int max_sd;
        if(browser_connected){
            FD_SET(browserfd, &readfds);
            max_sd = max(serverfd, browserfd);
        }else{
            FD_SET(listenfd, &readfds);
            max_sd = max(serverfd, listenfd);
        }
        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
        if ((activity < 0) && (errno!=EINTR))printf("select error"); 
        
        if(browser_connected && FD_ISSET(browserfd, &readfds)){
            int valread;
            char buffer[BUF_SIZE];
            if ((valread = recv( browserfd , buffer, 1, MSG_PEEK | MSG_DONTWAIT)) == 0)
            {  
                close(browserfd);
                cout<<"\033[1;31mBROWSER DISCONNECTED\033[0m"<<endl;
                browser_connected = 0;  
            }
            else
            {
                http_request r = get_http_request(browserfd);
                cout<<"\033[1;36mREQUEST RECEIVED\033[0m"<<endl;
                r.display();
                if(r.action.length() >= 7 && r.action.substr(0,7) == "/image/"){
                    send_http(browserfd, "."+r.action, "image/png");
                }else if(r.action == "/favicon.ico"){
                    send_http(browserfd, "./image/favicon.ico", "image/*");
                }else if(logged_in == 0){
                    if(r.method == "POST"){
                        if(r.action == "/login"){
                            map<string, string> m = process_form_data(r.content);
                            send_str(serverfd, "login");
                            send_str(serverfd, m["username"]);
                            send_str(serverfd, m["passwd"]);
                            string res = recv_str(serverfd);
                            if(res == "0"){
                                logged_in = 1;
                                cout<<"\033[1;32mLOGGED IN\033[0m"<<endl;
                            }else{
                                cout<<"\033[1;31mFAILED LOGIN\033[0m"<<endl;
                            }
                            send_redirect(browserfd, "/");
                            
                        }else if(r.action == "/signup"){
                            map<string, string> m = process_form_data(r.content);
                            send_str(serverfd, "signup");
                            send_str(serverfd, m["username"]);
                            send_str(serverfd, m["passwd"]);
                            string res = recv_str(serverfd);
                            if(res == "0"){
                                logged_in = 1;
                                cout<<"\033[1;32mACCOUNT CREATED\033[0m"<<endl;
                            }else{
                                cout<<"\033[1;31mFAILED SIGNUP\033[0m"<<endl;
                            }
                            send_redirect(browserfd, "/");
                        }
                    }else if(r.method == "GET"){
                        if(r.action == "/signup"){
                            send_http(browserfd, "./static/signup.html", "text/html");
                        }else if(r.action.length()>=8 && r.action.substr(0,8)=="/static/"){
                            send_http(browserfd, "."+r.action, "text/html");
                        }else{
                            send_http(browserfd, "./static/login.html", "text/html");
                        }
                    }
                }else{
                    if(r.method == "GET"){
                        if(r.action == "/friends"){
                            send_str(serverfd, "friends");
                            string temp = recv_str(serverfd);
                            ofstream temp_fstream;
                            temp_fstream.open("./data/friends.json");
                            temp_fstream << temp;
                            temp_fstream.close();
                            send_http(browserfd, "./data/friends.json", "text/html");
                        }else if(r.action.length() >= 6 && r.action.substr(0,6) == "/chat/"){
                            string recver = r.action.substr(6);
                            send_http(browserfd, "./static/chatroom.html", "text/html");
                        }else if(r.action.length() >= 5 && r.action.substr(0,5) == "/log/"){
                            string recver = r.action.substr(5);
                            send_str(serverfd, "log");
                            send_str(serverfd, recver);
                            string temp = recv_str(serverfd);
                            ofstream temp_fstream;
                            temp_fstream.open("./data/"+recver+".json");
                            temp_fstream << temp;
                            temp_fstream.close();
                            send_http(browserfd, "./data/"+recver+".json", "text/html");
                        }else{
                            send_http(browserfd, "./static/main.html", "text/html");
                        }
                    }else if(r.method == "POST"){
                        if(r.action == "/add"){
                            map<string, string> m = process_form_data(r.content);
                            send_str(serverfd, "add");
                            send_str(serverfd, m["username"]);
                            string res = recv_str(serverfd);
                            if(res == "0"){
                                cout<<"\033[1;32mADDED FRIEND "<<m["username"]<<"\033[0m"<<endl;
                            }else{
                                cout<<"\033[1;31mFRIEND "<<m["username"]<<" NOT ADDED\033[0m"<<endl;
                            }
                            send_redirect(browserfd, "/");
                        }else if(r.action == "/del"){
                            map<string, string> m = process_form_data(r.content);
                            send_str(serverfd, "del");
                            send_str(serverfd, m["username"]);
                            string res = recv_str(serverfd);
                            if(res == "0"){
                                cout<<"\033[1;32mDELETED FRIEND "<<m["username"]<<"\033[0m"<<endl;
                            }else{
                                cout<<"\033[1;31mFAILED TO DELETE FRIEND "<<m["username"]<<"\033[0m"<<endl;
                            }
                            send_redirect(browserfd, "/");
                        }else if(r.action == "/logout"){
                            logged_in = 0;
                            for (const auto& entry : filesystem::directory_iterator("./data")){
                                filesystem::remove_all(entry.path());
                            }
                            send_str(serverfd, "logout");
                            send_redirect(browserfd, "/");
                        }else if(r.action == "/sendtext"){
                            map<string, string> m = process_form_data(r.content);
                            send_str(serverfd, "sendtext");
                            send_str(serverfd, m["recver"]);
                            send_str(serverfd, m["text"]);
                            string res = recv_str(serverfd);
                            if(res == "0"){
                                cout<<"\033[1;32mMESSAGE SENT\033[0m"<<endl;
                            }else{
                                cout<<"\033[1;31mMESSAGE NOT SENT\033[0m"<<endl;
                            }
                            send_redirect(browserfd, "/chat/"+m["recver"]);
                        }
                    }
                }
            }
        }
        if(!browser_connected && FD_ISSET(listenfd, &readfds)){
            int addrlen = sizeof(address);
            if ((browserfd = accept(listenfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){ 
                exit(EXIT_FAILURE);  
            }
            browser_connected = 1;
            cout<<"\033[1;32mBROWSER CONNECTED\033[0m"<<endl;
        }
    }

    return 0;
}