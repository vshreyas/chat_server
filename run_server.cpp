#include "ChatServer.h"

int main(int argc, char* argv[])
{
	if(argc != 4 || atoi(argv[3]) == 0)
    {
        fprintf(stderr, "Start server with ./server <ip> <port number> <max_clients> \n");
        return -1;
    }
    int max_clients = atoi(argv[3]);
	ChatServer chat_server(max_clients);
	chat_server.start(argv[1], argv[2]);
	return 0;
}