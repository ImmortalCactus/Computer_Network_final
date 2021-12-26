#include <string>
#include <vector>
#include <sqlite3.h>

#define TEXT 0
#define IMAGE 1
#define FILE 2

using namespace std;
class chat_log{
public:
    time_t timestamp; // Absolutely needs fixing, just not now.
    string sender;
    string recver;
    int message_type;
    string message_content;
    void formatted_display();
};
class chat_db {
public:
    sqlite3 *db;
    void init_db();
    int has_user(string);
    void add_user(string);
    void add_friends(string, string);
    void delete_friends(string, string);
    int is_friends(string, string);
    vector<string> ls_friends(string);
    void add_chat_log(string, string, int, string);
    vector<chat_log> get_chat_log(string, string);
};