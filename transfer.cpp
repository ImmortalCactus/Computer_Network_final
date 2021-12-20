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


bool exists_test(const std::string& name){
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

void recv_file(int sockfd, const char* filename)
{
    FILE *fp;
    fp = fopen(filename, "wb");
    int size;
    read(sockfd, &size, sizeof(int));
    //std::cout<<"should receive "<<size<<std::endl;

    char buffer[BUF_SIZE];
    int bytes_received = 0;
    
    while(bytes_received < size){
        size_t cur_bytes_received;
        cur_bytes_received = read(sockfd, buffer, std::min(sizeof(buffer), (long unsigned int)(size-bytes_received)));
        //std::cout<<"received "<<cur_bytes_received<<std::endl;
        bytes_received += cur_bytes_received;
        fwrite(buffer, 1, cur_bytes_received, fp);
    }
    fclose(fp);
    //std::cout<<"FINISHED RECV_FILE!\n";
    
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
    //std::cout<<"should send "<<size<<std::endl;

    char buffer[BUF_SIZE];
    int bytes_sent = 0, bytes_read;
    while(bytes_sent < size){
        if((bytes_read=fread(buffer, 1, sizeof(buffer), fp))<=0){
            break;
        }
        size_t cur_bytes_sent = 0;
        while(cur_bytes_sent<bytes_read){
            int n = write(sockfd, buffer+cur_bytes_sent, bytes_read-cur_bytes_sent);
            //std::cout<<"sent "<<n<<std::endl;
            if(n==-1){
                fclose(fp);
                return;
            }
            cur_bytes_sent += n;
        }
        bytes_sent += cur_bytes_sent;
    }
    fclose(fp);
    //std::cout<<"FINISHED SEND_FILE!\n";
}

std::string recv_str(int sockfd){
    int size;
    read(sockfd, &size, sizeof(int));

    char buffer[BUF_SIZE];
    int bytes_received = 0;

    std::string str = "";
    while(bytes_received < size){
        int cur_bytes_received = read(sockfd, buffer, size-bytes_received);
        bytes_received += cur_bytes_received;
        buffer[cur_bytes_received] = '\0';
        str = str + std::string(buffer);
    }
    return str;

}
void send_str(int sockfd, const std::string& str){
    int size = str.length();
    write(sockfd, &size, sizeof(size));
    int bytes_sent = 0;
    while(bytes_sent < size){
        int n = write(sockfd, str.substr(bytes_sent).c_str(), size-bytes_sent);
        bytes_sent += n;
    }
}

