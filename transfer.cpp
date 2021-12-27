#include <sys/stat.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <map>
#include "transfer.h"
#define BUF_SIZE 1024
using namespace std;

bool exists_test(const string& name){
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

void recv_file(int sockfd, string filename)
{
    FILE *fp;
    fp = fopen(filename.c_str(), "wb");
    int size;
    read(sockfd, &size, sizeof(int));
    cout<<"should receive "<<size<<endl;

    char buffer[BUF_SIZE];
    int bytes_received = 0;
    
    while(bytes_received < size){
        size_t cur_bytes_received;
        cur_bytes_received = read(sockfd, buffer, min(sizeof(buffer), (long unsigned int)(size-bytes_received)));
        cout<<"received "<<cur_bytes_received<<endl;
        bytes_received += cur_bytes_received;
        fwrite(buffer, 1, cur_bytes_received, fp);
    }
    fclose(fp);
    cout<<"FINISHED RECV_FILE!\n";
    
}

void send_file(int sockfd,  string filename)
{
    FILE *fp;
    fp = fopen(filename.c_str(), "rb");
    int size;
    struct stat st;
    stat(filename.c_str(), &st);
    size = (int)st.st_size;

    write(sockfd, &size, sizeof(size));
    cout<<"should send "<<size<<endl;

    char buffer[BUF_SIZE];
    int bytes_sent = 0, bytes_read;
    while(bytes_sent < size){
        if((bytes_read=fread(buffer, 1, sizeof(buffer), fp))<=0){
            break;
        }
        size_t cur_bytes_sent = 0;
        while(cur_bytes_sent<bytes_read){
            int n = write(sockfd, buffer+cur_bytes_sent, bytes_read-cur_bytes_sent);
            cout<<"sent "<<n<<endl;
            if(n==-1){
                fclose(fp);
                return;
            }
            cur_bytes_sent += n;
        }
        bytes_sent += cur_bytes_sent;
    }
    fclose(fp);
    cout<<"FINISHED SEND_FILE!\n";
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

string stringtolower(string s){
    string temp="";
    for(auto i : s){
        if(i>='A' && i<='Z')temp+=(i-'A'+'a');
        else temp+=i;
    }
    return temp;
}


http_request get_http_request(int sockfd){
    http_request ret;
    char buffer[BUF_SIZE+1];
    int state = 0;
    int got_first_line = 0;
    string line = "";
    while(1){
        int n = read(sockfd, buffer, 1);
        if(state == 1 && buffer[0]=='\n'){
            line = "";
            state = 0;
        }else if(state == 0){
            if(buffer[0]!='\r'){
                line += buffer[0];
            }else{
                if(line.length() == 0){
                    state = 2;
                }else{
                    if(got_first_line == 0){
                        stringstream i_ss(line);
                        i_ss >> ret.method >> ret.action >> ret.http_version;
                        got_first_line = 1;
                    }else{
                        string field, value;
                        int i = line.find(":");
                        field = stringtolower(line.substr(0,i));
                        value = line.substr(i+2);
                        ret.headers.insert(pair<string, string>(field, value));
                    }
                    state = 1;
                }
            }
        }else if(state == 2 && buffer[0]=='\n'){
            break;
        }
    }
    if(ret.headers.count("content-length")!=0){
        int content_length;
        stringstream i_ss(ret.headers["content-length"]);
        i_ss >> content_length;
        string content = "";
        while((int)content.length() < content_length){
            int cur_bytes_received = read(sockfd, buffer, min(BUF_SIZE, (int)(content_length-content.length())));
            buffer[cur_bytes_received] = '\0';
            content = content + string(buffer);
        }
        ret.content = content;
    }else{
        ret.content = "";
    }
    return ret;
}

void http_request::display(){
    cout<<"========START OF REQUEST========\n";
    cout<<"METHOD: "<<method<<"\nACTION: "<<action<<"\nVERSION: "<<http_version<<"\nHEADERS: \n";
    for(auto i : headers)cout<<i.first<<": "<<i.second<<endl;
    cout<<"CONTENT:\n"<<content<<endl;
    cout<<"========END OF REQUEST========\n";
}

map<string, string> process_form_data(string form_data){
    map<string, string> ret;
    int begin=0, end=0;
    for(;end<=form_data.length();end++){
        if(end == form_data.length() || form_data[end]=='&'){
            string entry = form_data.substr(begin, end-begin);
            int eq = entry.find("=");
            ret.insert(pair<string,string>(entry.substr(0, eq), entry.substr(eq+1)));
            begin = end+1;
        }
    }
    return ret;
}

void send_http(int sockfd, string file, string type){
    FILE *fp;
    fp = fopen(file.c_str(), "rb");
    int size;
    struct stat st;
    stat(file.c_str(), &st);
    size = (int)st.st_size;
    string http_str = "HTTP/1.1 200 OK\r\nContent-Length: "+to_string(size)+"\r\n\r\n";
    int bytes_sent = 0;

    while(bytes_sent < http_str.length()){
        int n = write(sockfd, http_str.substr(bytes_sent).c_str(), http_str.length()-bytes_sent);
        bytes_sent += n;
    }


    char buffer[BUF_SIZE];
    bytes_sent = 0;
    int bytes_read;
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
    return;
}

void send_redirect(int sockfd, string url){
    string http_str = "HTTP/1.1 303 See Other\r\nLocation: " + url + "\r\n\r\n";
    int bytes_sent = 0;

    while(bytes_sent < http_str.length()){
        int n = write(sockfd, http_str.substr(bytes_sent).c_str(), http_str.length()-bytes_sent);
        bytes_sent += n;
    }
}