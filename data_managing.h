#include <string>
#include <vector>
#include <sqlite3.h>
using namespace std;
class chat_db {
public:
    sqlite3 *db;
    void init_db();
    int has_user(string);
    void add_user(string);
    void add_friends(string, string);
    vector<string> ls_friends(string);
};