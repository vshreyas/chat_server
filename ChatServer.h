#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <map>
#include <unistd.h>
#include <cctype>
#include "sbcp.h"

class ChatServer {
	fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    
    char buf[MAXDATASIZE];    // buffer for client data
    int nbytes;
    char* username;
    int num_clients;
	int max_clients;
	
	std::map<int, char*>  clients;
	std::map<int, FILE*> history;
	
	public:
	ChatServer(int max): max_clients(max), num_clients(0)
	{
		
	}
	int start(char* ip, char* port);
	
	static void *get_in_addr(struct sockaddr *sa)
	{
		if (sa->sa_family == AF_INET)
		{
			return &(((struct sockaddr_in*)sa)->sin_addr);
		}

		return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}
	
	static char* generate_posix_filename(const char* username) {
		char* filename = (char*)malloc(FILENAME_MAX);
		strcpy(filename, "hist_");
		size_t next = strlen(filename);
		int i;
		for(i = 0;i < strlen(username);i++)
		{
			if(isalnum(username[i]) || username[i] == '_' || username[i] == '.')
			{
				filename[next] = username[i];
				next++;
				if(next == FILENAME_MAX - 1) break;
			}
		}
		strcpy(filename + next, ".log");
		return filename;
	}
	
	protected:
	int accept_connection();
	void terminate_connection(int fd);
	void handle_message(int fd, sbcp_pkt pkt1);
	void handle_join_request(int fd, sbcp_pkt pkt1);
	void handle_list_request(int fd);
	void handle_history_request(int fd);
	void handle_search_request(int fd, sbcp_pkt pkt1);
};

#endif