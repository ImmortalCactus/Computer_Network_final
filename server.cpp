#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include "transfer.h"
#include "data_managing.h"


#define MAX_CLI 30
#define BUF_SIZE 1024

using namespace std;


enum state {NOT_USED, SIGNING_UP, LOGGING_IN, NO_ONE,LOGGED_IN};

bool user_online(string user, string names[], state client_state[]){
    for(int i=0; i<MAX_CLI; i++){
        if(names[i]==user && client_state[i] == LOGGED_IN)return 1;
    }
    return 0;
}

int main(int argc , char *argv[])  
{  
    int opt = 1;  
    int master_socket , addrlen , new_socket , client_socket[MAX_CLI], activity, i , valread;
    state client_state[MAX_CLI];
    string names[MAX_CLI];

    int max_sd;  
    struct sockaddr_in address;  
         
    char buffer[BUF_SIZE];  //data buffer of 1K 
    string ports(argv[1]); 
    stringstream portss(ports); 
    int port;  
    portss >> port;  
         
    //set of socket descriptors 
    fd_set readfds;  
     
    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < MAX_CLI; i++)  
    {  
        client_socket[i] = 0;  
        client_state[i] = NOT_USED;
    }  
         
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        //perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        //perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
     
    //type of socket created 
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( port );  

    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        //perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", port);  
         
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0)  
    {  
        //perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");

    chat_db database;  
    database.init_db();
    /*
    database.add_user("bbb", "bpass");
    database.add_user("aaa", "apass");
    database.add_user("ccc", "cpass");
    database.add_user("ddd", "dpass");
    database.add_friends("aaa", "bbb");
    database.add_friends("aaa", "ddd");
    database.add_chat_log("aaa", "bbb", "text", "Hello bbb, I am aaa.");
    database.add_chat_log("bbb", "aaa", "text", "Hello aaa, Nice to meet you.");
    database.add_chat_log("aaa", "bbb", "text", "This is nice.");
    database.add_friends("aaa", "bbb");
    database.delete_friends("aaa", "bbb");*/
    while(1)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
     
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        //add child sockets to set 
        for ( i = 0 ; i < MAX_CLI ; i++)  
        {  
            //socket descriptor 
            int sd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
     
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
             
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                //perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            //inform user of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                 
            //add new socket to array of sockets 
            for (i = 0; i < MAX_CLI; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  

                    client_socket[i] = new_socket;
                    client_state[i] = NO_ONE;
                    printf("Adding to list of sockets as %d\n" , i);    
                    break;  
                }  
            }
        }  
             
        //else its some IO operation on some other socket
        for (i = 0; i < MAX_CLI; i++)  
        {  
            int sockfd = client_socket[i];  
                 
            if (FD_ISSET( sockfd , &readfds))  
            {  
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = recv( sockfd , buffer, 1, MSG_PEEK | MSG_DONTWAIT)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sockfd , (struct sockaddr*)&address , (socklen_t*)&addrlen);  
                    printf("Host disconnected, name %s, ip %s , port %d \n" , names[i].c_str(), inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    //Close the socket and mark as 0 in list for reuse 
                    close(sockfd);  
                    client_socket[i] = 0; 
                    client_state[i] = NOT_USED;
                    names[i] = "";
                }  
                     
                else 
                {
                    string c = recv_str(sockfd);
                    if(c == "login"){
                        string username = recv_str(sockfd);
                        string passwd = recv_str(sockfd);
                        cout<<"\033[1;36mTrying login with "<<username<<"/"<<passwd<<"\033[0m\n";
                        if(database.login_verify(username, passwd) != 0){
                            if(!user_online(username, names, client_state)){
                                send_str(sockfd, "0");
                                client_state[i] = LOGGED_IN;
                                names[i] = username;
                            }else{
                                send_str(sockfd, "1");
                            }
                        }else{
                            send_str(sockfd, "2");
                        }
                    }else if(c == "signup"){
                        string username = recv_str(sockfd);
                        string passwd = recv_str(sockfd);
                        if(database.has_user(username) == 0){
                            database.add_user(username, passwd);
                            client_state[i] = LOGGED_IN;
                            names[i] = username;
                            send_str(sockfd, "0");
                        }else{
                            send_str(sockfd, "1");
                        }
                    }else if(c == "friends"){
                        vector<string> friends_list = database.ls_friends(names[i]);
                        ofstream temp_fstream;
                        temp_fstream.open("./server_dir/"+names[i]+"_friends.json");
                        temp_fstream << "{\"friends\":[";
                        for(int i=0;i<friends_list.size();i++){
                            temp_fstream << "\"" << friends_list[i] << "\"";
                            if(i!=friends_list.size()-1)temp_fstream << ",";
                        }
                        temp_fstream << "]}";
                        temp_fstream.close();
                        send_file(sockfd, "./server_dir/"+names[i]+"_friends.json");
                    }else if(c == "add"){
                        string name_friend = recv_str(sockfd);
                        if(database.has_user(name_friend) && !database.is_friends(names[i],name_friend)){
                            database.add_friends(names[i], name_friend);
                            send_str(sockfd, "0");
                        }else{
                            send_str(sockfd, "1");
                        }
                    }else if(c == "del"){
                        string name_friend = recv_str(sockfd);
                        if(database.is_friends(names[i],name_friend)){
                            database.delete_friends(names[i], name_friend);
                            send_str(sockfd, "0");
                        }else{
                            send_str(sockfd, "1");
                        }
                    }else if(c == "logout"){
                        client_state[i] = NO_ONE;
                        names[i] = "";
                    }else if(c == "log"){
                        string recver = recv_str(sockfd);
                        cout<<"\033[1;36mFETCHING CHAT LOG WITH"<<names[i]<<" and "<<recver<<"\033[0m\n";
                        vector<chat_log> log = database.get_chat_log(names[i], recver);
                        ofstream temp_fstream;
                        temp_fstream.open("./server_dir/"+names[i]+"_"+recver+".json");
                        temp_fstream << "{\"log\":[";
                        for(int i=0;i<log.size();i++){
                            temp_fstream << "{\"sender\":\""+log[i].sender+"\",";
                            temp_fstream << "\"recver\":\""+log[i].recver+"\",";
                            temp_fstream << "\"type\":\""+log[i].message_type+"\",";
                            temp_fstream << "\"content\":\""+log[i].message_content+"\",";
                            temp_fstream << "\"timestamp\":\""+log[i].timestamp+"\"}";
                            if(i!=log.size()-1)temp_fstream <<  ",";
                        }
                        temp_fstream <<  "]}";
                        temp_fstream.close();
                        send_file(sockfd, "./server_dir/"+names[i]+"_"+recver+".json");
                    }else if(c == "sendtext"){
                        string recver = recv_str(sockfd);
                        string text = recv_str(sockfd);
                        database.add_chat_log(names[i], recver, "text", text);
                        send_str(sockfd, "0");
                    }
                }  
            }  
        }  
    }  
         
    return 0;  
}  