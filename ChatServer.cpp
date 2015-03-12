#include "ChatServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>

#define NAME_MAX 255    /* # chars in a file name */

int ChatServer::start(char* ip, char* port)
{
	struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
	char remoteIP[INET6_ADDRSTRLEN];
    int yes=1;        // argument to set setsockopt() to SO_REUSEADDR, below
    int i, j, rv;
    int num_clients =0;
    struct addrinfo hints, *ai, *p;
	
	FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
	
	// get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(ip, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "server: %s\n", gai_strerror(rv));
        return -1;
    }

    for(p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // Check for bind error
    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        return -1;
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor so far
    fdmax = listener;

    // loop forever
    for(;;)
    {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            return -1;
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))   // we got one!!
            {
                if (i == listener)
                {
					int newfd = accept_connection();
					if(newfd != -1)
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax)      // keep track of the max
						{
							fdmax = newfd;
						}
					}
				}
				else {
					 // an existing client wants to pass a message or issue a command
                    sbcp_pkt packet1;
                    if ((nbytes = recv(i, &packet1, sizeof(sbcp_pkt), 0)) <= 0)
                    {
                        // error or connection closed by client
                        if (nbytes == 0)
                        {
                            // connection closed
                            printf("server: socket %d hung up\n", i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        terminate_connection(i);
                    }
                    else
                    {
                        // check packet
                        if(packet1.vrsn != 3 || packet1.length < 0)
                        {
                            fprintf(stderr, "Malformed header or version mismatch\n");
                        }
                        else
                        {
							switch(packet1.type) {
								case SEND: handle_message(i, packet1);
									break;
								case LIST: handle_list_request(i);
									break;
								case HISTORY: handle_history_request(i);
									break;
								case SEARCH: handle_search_request(i, packet1);
									break;
								case JOIN: handle_join_request(i, packet1);
									break;
								default: fprintf(stderr, "Unknown message type");
									break;
							}
						}
					}
				}
			}
		}
	}
	
}

int ChatServer::accept_connection()
{
	// handle new connections
	struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen = sizeof(remoteaddr);
	char remoteIP[INET6_ADDRSTRLEN];
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

	printf("server: new connection from %s on socket %d\n",
		   inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr),
					 remoteIP, INET6_ADDRSTRLEN),
		   newfd);
	if (newfd == -1)
	{
		perror("accept");
		return -1;
	}
	else return newfd;
}

void ChatServer::handle_message(int i, sbcp_pkt packet1)
{
	
	memset(buf, '\0', MAXDATASIZE);
	if ((nbytes = recv(i, buf + nbytes, packet1.length - sizeof(sbcp_pkt), 0)) <= 0)
	{
		perror("recv");
		terminate_connection(i);
	}
	else
	{
		short attr_type;
		memcpy(&attr_type, &buf[4], 2);
		short attr_len;
		memcpy(&attr_len, &buf[6], 2);
		if(attr_type == ATTR_MESSAGE)
		{
			username = clients[i];
			sbcp_pkt header1;
			header1.vrsn = 3;
			header1.type = 3;
			header1.length = packet1.length + strlen(username) + 4;
			
			memcpy(buf, &header1, 4);
			attr_type = 2;
			memcpy(&buf[packet1.length], &attr_type, sizeof(short));
			int username_attr_len = strlen(username) + 4;
			memcpy(&buf[packet1.length + 2], &username_attr_len, sizeof(short));
			memcpy(&buf[packet1.length + 4], username, strlen(username));
			memset(&buf[packet1.length + 4 + strlen(username)], '\0', MAXDATASIZE - packet1.length);
			for(int j = 0; j <= fdmax; j++)
			{
				// send to everyone!
				if (FD_ISSET(j, &master))
				{
					// except the listener and the sender
					if (j != listener && j != i)
					{
						sbcp_pkt h;
						memcpy(&h, buf, 4);

						if (send(j, buf, MAXDATASIZE, 0) == -1)
						{
							perror("send");
							terminate_connection(j);
						}
					}
				}
			}
		}
	}
}

void ChatServer::handle_join_request(int newfd, sbcp_pkt packet1)
{
	memset(buf, 0, MAXDATASIZE);
	nbytes = sizeof(sbcp_pkt);
	while(nbytes < packet1.length)
	{
		int len = (packet1.length - nbytes) > MAXDATASIZE ? MAXDATASIZE : (packet1.length - nbytes);
		int x = recv(newfd, buf + nbytes, len, 0);
		if(x < 0)
		{
			perror("recv");
		}
		else nbytes += x;
	}
	buf[packet1.length] = '\0';
	username = (char*)malloc((2 + strlen(buf + 8))* sizeof(char));
	memset(username, 0, sizeof(username));
	strcpy( username, buf + 8);
	//check if username is already taken
	int username_exists = 0;
	for(auto const &it : clients)
	{
		if(strcmp(it.second, username) ==0) username_exists = 1;
	}
	if(username_exists == 1 || num_clients == max_clients)
	{
		printf("Sorry, connection already exists with this name or too many connections\n");
		free(username);
		close(newfd);
		FD_CLR(newfd, &master);
	}
	else {
		num_clients++;
		clients[newfd] = username;
		char* fname = generate_posix_filename(username);
		history[newfd] = fopen(fname, "a+");
		free(fname);
		printf("Username: %s on socket %d\n", clients[newfd], newfd);
	}
}

void ChatServer::handle_list_request(int i) {
	fprintf(history[i], "list users\n");
	fflush(history[i]);
	int k;
	char usernames[MAXDATASIZE] = {0};
	sprintf(usernames, "--");
	for(k = 0;k < max_clients;k++)
	{
		if(clients[k] != NULL && k != i) {
			if(strlen(usernames) + strlen(clients[k]) + 1 > MAXDATASIZE)
			{
				break;
			}
			strcat(usernames, clients[k]);
			strcat(usernames, ",");
		}
	}
	
	_UPACKET sendpacket ;  // the packet to be sent
	memset(&sendpacket, 0, sizeof(_UPACKET));
	/// packaging the sendpacket
	short size_packet = packaging(FWD, ATTR_MESSAGE, strlen(usernames), usernames, &sendpacket);

	if (send(i, sendpacket.cPacket, size_packet, 0) == -1)
	{
		perror("send");
		terminate_connection(i);
	}
}


void ChatServer::handle_history_request(int i)
{
	fprintf(history[i], "history\n");
	fclose(history[i]);
	username = clients[i];
	char* fname = generate_posix_filename(username);
	history[i]  = fopen(fname, "r");
	if(history[i] == NULL) {
		perror("opening history file");
		close(i);
		FD_CLR(i, &master);
		free(clients[i]);
		clients[i] = NULL;
		return;
	}
	else {
		char * line = NULL;
		size_t len = 0;
		ssize_t read;
		while((read = getline(&line, &len, history[i])) != -1) {
			_UPACKET sendpacket ;  // the packet to be sent
			memset(&sendpacket, 0, sizeof(_UPACKET));
			/// packaging the sendpacket
			short size_packet = packaging(FWD, ATTR_MESSAGE, strlen(line), line, &sendpacket);

			if (send(i, sendpacket.cPacket, size_packet, 0) == -1)
			{
				perror("send");
				if (line){
					free(line);
				}
				terminate_connection(i);
				return;
			}
		}
		fclose(history[i]);
		if (line){
			free(line);
		}
	}
	fname = generate_posix_filename(username);
	history[i]  = fopen(fname, "a+");
	if(history[i] == NULL) {
		perror("opening history file");
	}
}

void ChatServer::handle_search_request(int i, sbcp_pkt packet1)
{
	memset(buf, '\0', MAXDATASIZE);
	if ((nbytes = recv(i, buf + nbytes, packet1.length - sizeof(sbcp_pkt), 0)) <= 0)
	{
		perror("recv");
		terminate_connection(i);
		return;
	}
	else
	{
		int attr_type;
		memcpy(&attr_type, &buf[4], 2);
		int attr_len;
		memcpy(&attr_len, &buf[6], 2);
		// printf("%s: %s\n", clients[i], buf + 8);
		if(attr_type == ATTR_REGEX)
		{
			char* pattern = (char*) malloc((1 + attr_len) * sizeof(char));
			memcpy(pattern, buf + 8, attr_len);
			pattern[attr_len - 1] = '\0';
			fprintf(history[i], "search history %s\n", pattern);
			fflush(history[i]);
			regex_t regex;
			int reti = regcomp(&regex, pattern, 0);
			if(reti == 0) {
				fclose(history[i]);								
				char* fname = generate_posix_filename(username);
				history[i]  = fopen(fname, "r");
				if(history[i] == NULL) {
					perror("opening history file");
					regfree(&regex);
					terminate_connection(i);
					return;
				}
				else {
					char * line = NULL;
					size_t len = 0;
					ssize_t read;
					while((read = getline(&line, &len, history[i])) != -1) {
						reti = regexec(&regex, line, 0, NULL, 0);
						if(reti == 0)
						{
							_UPACKET sendpacket ;  // the packet to be sent
							memset(&sendpacket, 0, sizeof(_UPACKET));
							/// packaging the sendpacket
							short size_packet = packaging(FWD, ATTR_MESSAGE, strlen(line), line, &sendpacket);

							if (send(i, sendpacket.cPacket, size_packet, 0) == -1)
							{
								perror("send");
								if (line){
									free(line);
								}
								regfree(&regex);
								terminate_connection(i);
								return;
							}
						}
					}
					fclose(history[i]);
					if (line){
						free(line);
					}
					regfree(&regex);
				}
				fname = generate_posix_filename(username);
				history[i]  = fopen(fname, "a+");
				if(history[i] == NULL) {
					perror("opening history file");
				}
			}
		}
	}
}

void ChatServer::terminate_connection(int i)
{
	free(clients[i]);
	clients.erase(i);
    close(i); // bye!
	if(history.find(i) != history.end() && history[i] != NULL) {
		fclose(history[i]);
	}
	history.erase(i);
	FD_CLR(i, &master); // remove from master set
}
