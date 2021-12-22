#include <sys/stat.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>
#define BUF_SIZE 1024
using namespace std;

bool exists_test(const string& name){
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

void recv_file(int sockfd, const char* filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    int size;
    read(sockfd, &size, sizeof(int));
    //cout<<"should receive "<<size<<endl;

    char buffer[BUF_SIZE];
    int bytes_received = 0;
    
    while(bytes_received < size){
        size_t cur_bytes_received;
        cur_bytes_received = read(sockfd, buffer, min(sizeof(buffer), (long unsigned int)(size-bytes_received)));
        //cout<<"received "<<cur_bytes_received<<endl;
        bytes_received += cur_bytes_received;
        fwrite(buffer, 1, cur_bytes_received, fp);
    }
    fclose(fp);
    //cout<<"FINISHED RECV_FILE!\n";
    
}

void send_file(int sockfd,  const char* filename)
{
    FILE *fp;
    fp = fopen(filename, "rb");
    int size;
    struct stat st;
    stat(filename, &st);
    size = (int)st.st_size;

    write(sockfd, &size, sizeof(size));
    //cout<<"should send "<<size<<endl;

    char buffer[BUF_SIZE];
    int bytes_sent = 0, bytes_read;
    while(bytes_sent < size){
        if((bytes_read=fread(buffer, 1, sizeof(buffer), fp))<=0){
            break;
        }
        size_t cur_bytes_sent = 0;
        while(cur_bytes_sent<bytes_read){
            int n = write(sockfd, buffer+cur_bytes_sent, bytes_read-cur_bytes_sent);
            //cout<<"sent "<<n<<endl;
            if(n==-1){
                fclose(fp);
                return;
            }
            cur_bytes_sent += n;
        }
        bytes_sent += cur_bytes_sent;
    }
    fclose(fp);
    //cout<<"FINISHED SEND_FILE!\n";
}

string recv_str(int sockfd){
    int size;
    read(sockfd, &size, sizeof(int));

    char buffer[BUF_SIZE];
    int bytes_received = 0;

    string str = "";
    while(bytes_received < size){
        int cur_bytes_received = read(sockfd, buffer, size-bytes_received);
        bytes_received += cur_bytes_received;
        buffer[cur_bytes_received] = '\0';
        str = str + string(buffer);
    }
    return str;

}
void send_str(int sockfd, const string& str){
    int size = str.length();
    write(sockfd, &size, sizeof(size));
    int bytes_sent = 0;
    while(bytes_sent < size){
        int n = write(sockfd, str.substr(bytes_sent).c_str(), size-bytes_sent);
        bytes_sent += n;
    }
}

void get_http_request(int sockfd){
    char buffer[BUF_SIZE+1];
    int state = 0;
    int content_length = 0;
    vector<string> request;
    string line;
    while(1){
        read(sockfd, buffer, 1);
        if(state == 0 && buffer[0]=='\r')state = 1;
        else if(state == 1 && buffer[0]=='\n')state = 2;
        else if(state == 2){
            if(buffer[0]!='\r'){
                line += buffer[0];
            }else{
                if(line.substr(0, 16) == "content-length: "){
                    stringstream i_ss(line.substr(16));
                    i_ss >> content_length;
                }
                if(line.length() == 0){
                    state = 3;
                }else{
                    state = 1;
                    request.push_back(line);
                }
            }
        }
        else if(state == 3 && buffer[0]=='\n'){
            break;
        }
    }
    
    string content = "";
    int bytes_received = 0;
    while(bytes_received < content_length){
        int cur_bytes_received = read(sockfd, buffer, size-bytes_received);
        bytes_received += cur_bytes_received;
        buffer[cur_bytes_received] = '\0';
        str = str + string(buffer);
    }
}