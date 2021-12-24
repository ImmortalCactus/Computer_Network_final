sqlite: sqlite3/sqlite3.c sqlite3/sqlite3.h
	gcc -c sqlite3/sqlite3.c -o sqlite3.o
client: client.cpp transfer.cpp transfer.h
	g++ --std=c++2a client.cpp transfer.cpp -o client
server: server.cpp transfer.cpp transfer.h data_managing.cpp data_managing.h
	g++ --std=c++2a server.cpp transfer.cpp data_managing.cpp sqlite3.o -lpthread -ldl -o server 