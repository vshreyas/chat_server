/*
** server.c - SBCP server for multi-user chat
*/

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
#include "sbcp.h"
#include "common.h"

#define NAME_MAX         255    /* # chars in a file name */
#define MTU 1500    // maximum transmission unit

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* generate_posix_filename(const char* username) {
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

int main(int argc, char* argv[])
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[MTU];    // buffer for client data
    int nbytes;
    char* username;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes=1;        // argument to set setsockopt() to SO_REUSEADDR, below
    int i, j, rv;
    int num_clients =0;
    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);


    if(argc != 4 || atoi(argv[3]) < 0)
    {
        fprintf(stderr, "Start server with ./server <ip> <port number> <max_clients> \n");
        return -1;
    }
    int max_clients = atoi(argv[3]);
    char* clients[max_clients];
    for(i = 0; i < max_clients; i++)
    {
        clients[i] = NULL;
    }
	FILE* history[max_clients];
	for(i = 0; i < max_clients; i++)
    {
        history[i] = NULL;
    }
	
    // get us a socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &ai)) != 0)
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
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);

                    if (newfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)      // keep track of the max
                        {
                            fdmax = newfd;
                        }

                        printf("server: new connection from %s on "
                               "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr*)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);
                        memset(buf, 0, sizeof(buf));
                        sbcp_pkt packet1;
                        //Wait for join
                        if ((nbytes = recv(newfd, &packet1, sizeof(sbcp_pkt), 0)) <= 0)
                        {
                            // got error or connection closed by client
                            if (nbytes == 0)
                            {
                                // connection closed
                                printf("server: socket %d hung up\n", i);
                            }
                            else
                            {
                                perror("recv");
                            }
                            close(newfd); // bye!
                            FD_CLR(newfd, &master); // remove from master set
                        }
                        else
                        {
                            //Received a JOIN request, check if header is OK
                            if(packet1.vrsn != 3 || packet1.type != 2 || packet1.length < 0)
                                fprintf(stderr, "Ill-formed or corrupted JOIN request\n");
                            else
                            {

                                nbytes = sizeof(sbcp_pkt);
                                while(nbytes < packet1.length)
                                {
                                    int len = (packet1.length - nbytes) > MTU ? MTU : (packet1.length - nbytes);
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
								int j;
                                for(j = 0; j < max_clients; j++)
                                {
                                    if(clients[j] != NULL)
                                    {
                                        if(strcmp(clients[j], username) ==0) username_exists = 1;
                                    }
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
                        }
                    }
                }
                else 
				{
                    // an existing client wants to pass a SEND message or issue a command
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
                        free(clients[i]);
                        clients[i] = NULL;
                        close(i); // bye!
						if(history[i] != NULL)fclose(history[i]);
						history[i] = NULL;
                        FD_CLR(i, &master); // remove from master set
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
							if(packet1.type == SEND)
							{
								memset(buf, '\0', MTU);
								if ((nbytes = recv(i, buf + nbytes, packet1.length - sizeof(sbcp_pkt), 0)) <= 0)
								{
									perror("recv");
									close(i);
									FD_CLR(i, &master);
									free(clients[i]);
									clients[i] = NULL;
									fclose(history[i]);
									history[i] = NULL;
								}
								else
								{
									int attr_type;
									memcpy(&attr_type, &buf[4], 2);
									int attr_len;
									memcpy(&attr_len, &buf[6], 2);
									// printf("%s: %s\n", clients[i], buf + 8);
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
										memset(&buf[packet1.length + 4 + strlen(username)], '\0', MTU - packet1.length);
										for(j = 0; j <= fdmax; j++)
										{
											// send to everyone!
											if (FD_ISSET(j, &master))
											{
												// except the listener and the sender
												if (j != listener && j != i)
												{
													sbcp_pkt h;
													memcpy(&h, buf, 4);

													if (send(j, buf, MTU, 0) == -1)
													{
														perror("send");
														close(j);
														FD_CLR(j, &master);
														free(clients[j]);
														clients[j] = NULL;
														fclose(history[j]);
														history[j] = NULL;
													}

												}
											}
										}
									}
								}
							} 
							else if(packet1.type == LIST)
							{
								fprintf(history[i], "list users\n");
								fflush(history[i]);
								int k;
								char usernames[MTU] = {0};
								sprintf(usernames, "--");
								for(k = 0;k < max_clients;k++)
								{
									if(clients[k] != NULL && k != i) {
										if(strlen(usernames) + strlen(clients[k]) + 1 > MTU)
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
									close(i);
									FD_CLR(i, &master);
								}
							}
							else if(packet1.type == HISTORY)
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
									break;
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
											close(i);
											FD_CLR(i, &master);
											free(clients[i]);
											clients[i] = NULL;
											break;
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
							else if(packet1.type == SEARCH) {
								
								memset(buf, '\0', MTU);
								if ((nbytes = recv(i, buf + nbytes, packet1.length - sizeof(sbcp_pkt), 0)) <= 0)
								{
									perror("recv");
									close(i);
									FD_CLR(i, &master);
									free(clients[i]);
									clients[i] = NULL;
									fclose(history[i]);
									history[i] = NULL;
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
												close(i);
												FD_CLR(i, &master);
												free(clients[i]);
												clients[i] = NULL;
												break;
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
															close(i);
															FD_CLR(i, &master);
															free(clients[i]);
															clients[i] = NULL;
															break;
														}
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
									}
								}
							}
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END of infinite loop

    return 0;
}

