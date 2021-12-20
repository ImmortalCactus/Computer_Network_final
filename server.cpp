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
#include <map>
#include "transfer.h"


#define MAX_CLI 30
#define BUF_SIZE 1024

using namespace std;


enum state {NOT_USED, NO_ONE, LOGGING_IN, LOGGED_IN, SIGNING_UP};
/*
bool request_signup(int socketfd){
    send_str(socketfd, "1. login\n 2. signup");
    string choice = recv_str(socketfd)
    string username = recv_str(socketfd);
    
}

bool user_is_logged_in(string username){
    //checks if user is logged in
}
*/
int main(int argc , char *argv[])  
{  
    int opt = 1;  
    int master_socket , addrlen , new_socket , client_socket[MAX_CLI], activity, i , valread;
    state client_state[MAX_CLI];
    string names[MAX_CLI];
    map<string, int> user2socket;
    

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
         
    //bind the socket to localhost port 8888 
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

    user2socket.insert(pair<string, int>("admin", 0));
    user2socket.insert(pair<string, int>("guest", 0));
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
                    send_str(new_socket, "1. login\n2. signup\n3. quit\n");
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
                    user2socket[names[i]] = 0;
                    names[i] = "";
                }  
                     
                //Echo back the message that came in 
                else 
                {
                    string c = recv_str(sockfd);
                    
                    switch(client_state[i]){
                        case NO_ONE:
                            if(c == "1"){
                                send_str(sockfd, "You want to login\nInput your username below or type \"cancel\" to cancel: \n");
                                client_state[i] = LOGGING_IN;
                            }else if(c == "2"){
                                send_str(sockfd, "You want to signup\nPlease choose your username below or type \"cancel\" to cancel: \n");
                                client_state[i] = SIGNING_UP;
                            }else if(c == "3"){
                                send_str(sockfd, "You want to quit\n");
                            }else{
                                send_str(sockfd, "RTFM kekw\n");
                            }
                            break;
                        case LOGGING_IN:
                            if(c == "cancel"){
                                send_str(sockfd, "1. login\n2. signup\n3. quit\n");
                                client_state[i] = NO_ONE;
                            }else if(user2socket.count(c) != 0){
                                if(user2socket[c] == 0){
                                    send_str(sockfd, "Logged in\nType in commands or \"logout\" to logout.\n");
                                    client_state[i] = LOGGED_IN;
                                    user2socket[c] = sockfd;
                                    names[i] = c;
                                }else{
                                    send_str(sockfd, "Failed. You are logged in somewhere else.\n1. login\n2. signup\n3. quit\n");
                                    client_state[i] = NO_ONE;
                                }
                            }else{
                                send_str(sockfd, "Failed. Username not found.\n1. login\n2. signup\n3. quit\n");
                                client_state[i] = NO_ONE;
                            }
                            //request name
                            //if option is cancel, state -> NO_ONE
                            //else if name is valid and not already logged_in, state -> LOGGED_IN
                            //else request again
                            break;
                        case LOGGED_IN:
                            if(c == "logout"){
                                send_str(sockfd, "Logged out.\n1. login\n2. signup\n3. quit\n");
                                client_state[i] = NO_ONE;
                                user2socket[names[i]] = 0;
                            }else if(c == "whoami"){
                                send_str(sockfd, "You are "+names[i]+".\n");
                            }
                            break;
                        case SIGNING_UP:
                            //request name
                            //if option is cancel, state -> NO_ONE
                            //else if name is valid, state -> LOGGED_IN
                            //else request again
                        default:
                            break;
                    }
                }  
            }  
        }  
    }  
         
    return 0;  
}  