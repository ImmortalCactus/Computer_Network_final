#include "data_managing.h"
#include <iostream>
#include <iomanip>
#include <sstream>
using namespace std;

void chat_db::init_db(){
    int rc = sqlite3_open("chat.db", &db);
    char *zErrMsg = 0;
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }else{
        fprintf(stderr, "Opened database successfully\n");
    }
    string sql = "create table if not exists USERS("
                "ID INTEGER PRIMARY KEY  AUTOINCREMENT,"
                "NAME TEXT NOT NULL,"
                "PASSWD TEXT NOT NULL);";
    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table USERS created successfully\n");
    }
    sql = "create table if not exists FRIENDS("
                "USERA TEXT NOT NULL,"
                "USERB TEXT NOT NULL);";
    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table FRIENDS created successfully\n");
    }
    sql = "create table if not exists CHAT_HISTORY("
                "SENDER TEXT NOT NULL,"
                "RECVER TEXT NOT NULL,"
                "TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "MSGTYPE TEXT NOT NULL,"
                "CONTENT TEXT);";
    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table CHAT_HISTORY created successfully\n");
    }
}

int chat_db::has_user(string user){
    int found = 0;
    char *zErrMsg = 0;
    auto callback = [](void *data, int argc, char **argv, char **azColName) { 
        (*(int *)data)++;
        return 0;
    };
    string sql = "select * from USERS where NAME = '" + user + "'";
    int rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&found, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    return found;
}

int chat_db::login_verify(string user, string passwd){
    int found = 0;
    char *zErrMsg = 0;
    auto callback = [](void *data, int argc, char **argv, char **azColName) { 
        (*(int *)data)++;
        return 0;
    };
    string sql = "select * from USERS where NAME = '" + user + "' and PASSWD = '" + passwd + "'";
    int rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&found, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    return found;
}

void chat_db::add_user(string user, string passwd){
    char *zErrMsg = 0;
    string sql = "insert into USERS (NAME,PASSWD) values ('" + user + "','" + passwd +"');";
    int rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
}

void chat_db::add_friends(string user1, string user2){
    char *zErrMsg = 0;
    string sql = "insert into FRIENDS (USERA, USERB) values ('" + user1 + "','" + user2 + "');";
    int rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
}

void chat_db::delete_friends(string user1, string user2){
    char *zErrMsg = 0;
    string sql = "delete from FRIENDS where USERA = '" + user1 +"' and USERB = '" + user2 + "' or USERA = '" + user2 +"' and USERB = '" + user1 + "';";
    int rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
};

int chat_db::is_friends(string user1, string user2){
    int found = 0;
    char *zErrMsg = 0;
    auto callback = [](void *data, int argc, char **argv, char **azColName) { 
        (*(int *)data)++;
        return 0;
    };
    string sql = "select * from FRIENDS where USERA = '" + user1 +"' and USERB = '" + user2 + "' or USERA = '" + user2 +"' and USERB = '" + user1 + "';";
    int rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&found, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    return found;
}

vector<string> chat_db::ls_friends(string user){
    vector<string> v;
    char *zErrMsg = 0;
    auto callback1 = [](void *data, int argc, char **argv, char **azColName) { 
        string str(argv[1]);
        (*(vector<string> *)data).push_back(str);
        return 0;
    };
    string sql = "select * from FRIENDS where USERA = '" + user + "'";
    int rc = sqlite3_exec(db, sql.c_str(), callback1, (void*)&v, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    auto callback2 = [](void *data, int argc, char **argv, char **azColName) { 
        string str(argv[0]);
        (*(vector<string> *)data).push_back(str);
        return 0;
    };
    sql = "select * from FRIENDS where USERB = '" + user + "'";
    rc = sqlite3_exec(db, sql.c_str(), callback2, (void*)&v, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    return v;
}

void chat_db::add_chat_log(string sender, string recver, string message_type, string message_content){
    char *zErrMsg = 0;
    string sql = "insert into CHAT_HISTORY (SENDER, RECVER, MSGTYPE, CONTENT) values ('" + sender + "','" + recver + "','" + message_type + "','" + message_content + "');";
    int rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
}

vector<chat_log> chat_db::get_chat_log(string user1, string user2){
    char *zErrMsg = 0;
    vector<chat_log> ret;
    auto callback = [](void *data, int argc, char **argv, char **azColName) {
        chat_log temp;
        temp.sender = string(argv[0]);
        temp.recver = string(argv[1]);
        temp.timestamp = string(argv[2]);
        temp.message_type = string(argv[3]);
        temp.message_content = string(argv[4]);
        (*(vector<chat_log> *)data).push_back(temp);
        return 0;
    };
    string sql = "select * from CHAT_HISTORY where SENDER = '" + user1 + "' and RECVER = '" + user2 + "' or SENDER = '" + user2 + "' and RECVER = '" + user1 + "';";
    int rc = sqlite3_exec(db, sql.c_str(), callback, (void *)&ret, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Operation done successfully\n");
    }
    return ret;
}

void chat_log::formatted_display(){
    cout<<sender<<"->"<<recver<<"("<<message_type<<"): '"<<message_content<<"' --- at "<<timestamp<<endl;
}

