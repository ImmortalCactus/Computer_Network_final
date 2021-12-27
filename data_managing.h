#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;
class chat_log{
public:
    string timestamp; // Absolutely needs fixing, just not now.
    string sender;
    string recver;
    string message_type;
    string message_content;
    void formatted_display();
};
class chat_db {
public:
    sqlite3 *db;
    void init_db();
    int has_user(string);
    int login_verify(string, string);
    void add_user(string, string);
    void add_friends(string, string);
    void delete_friends(string, string);
    int is_friends(string, string);
    vector<string> ls_friends(string);
    void add_chat_log(string, string, string, string);
    vector<chat_log> get_chat_log(string, string);
};