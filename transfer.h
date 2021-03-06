#include <map>
#include <string>
#define BUF_SIZE 1024
#define MAX_CLI 30
using namespace std;

class http_request{
public:
    string action;
    string method;
    string http_version;
    map<string, string> headers;
    string content;
    void display();
};

string stringtolower(string s);

bool exists_test (const string& name);
void recv_file(int sockfd, string filename);
void send_file(int sockfd,  string filename);
string recv_str(int sockfd);
void send_str(int sockfd, const string& str);
http_request get_http_request(int sockfd);
map<string, string> process_form_data(string form_data);
void send_http(int sockfd, string file, string type);
void send_redirect(int sockfd, string url);