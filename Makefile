client: client.cpp transfer.cpp transfer.h
	g++ --std=c++17 -o client client.cpp transfer.cpp
server: server.cpp transfer.cpp transfer.h
	g++ --std=c++17 -o server server.cpp transfer.cpp