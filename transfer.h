#define BUF_SIZE 1024
#define MAX_CLI 30

bool exists_test (const std::string& name);
void recv_file(int sockfd, const char* filename);
void send_file(int sockfd,  const char* filename);
std::string recv_str(int sockfd);
void send_str(int sockfd, const std::string& str);