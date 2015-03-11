#include "common.h"

const char* connect_command = "connect";
const char* login_command = "login";
const char* history_command = "history\n";
const char* list_command = "list";
const char* search_command = "search";

typedef struct chat_command{
	char keyword[32];
	char arguments[MAXDATASIZE];
} chat_command;

chat_command* create(const char* keyword, const char* args) {
	chat_command* cc = (chat_command*)malloc(sizeof(chat_command));
	if(strlen(args) < 32)strcpy(cc->keyword, keyword);
	strncpy(cc->arguments, args,MAXDATASIZE);
	return cc;
}

chat_command* process_command(char* buffer) {
	char copy[MAXDATASIZE];
	strcpy(copy, buffer);
	char* command = strtok(copy, " ");
	if(command == NULL) {
		return create("", buffer);
	} else if(strcmp(command, connect_command) == 0) {
		char* remaining = strtok(NULL, "\n");
		return create(connect_command, remaining);
	} else if(strcmp(command, login_command) == 0) {
		char* remaining = strtok(NULL, "\n");
		return create(login_command, remaining);
	} else if(strcmp(command, history_command) == 0) {
		char* remaining = strtok(NULL, "\n");
		if(remaining != NULL && strlen(remaining) > 0) {// it's a chat message
			return create("", buffer);
		}
		else {
			return create(history_command, "");
		}
	} else if(strcmp(command, list_command) == 0) {
		char* remaining = strtok(NULL, "\n");
		if(remaining != NULL && strcmp(remaining, "users") == 0) {
			return create(list_command, remaining);
		}
	} else if(strcmp(command, search_command) == 0) {
		char* remaining = strtok(NULL, " ");
		if(remaining != NULL && strcmp(remaining, "history") == 0) {
			remaining = strtok(NULL, "\n");
			if(remaining != NULL) {
				return create(search_command, remaining);
			}
		}
		else return create("", buffer);
	}
	else return create("", buffer);
}

int process_connect(char* buffer, char* ip, char* port) {
	char* command = strtok(buffer, " ");
	if(command == NULL || strcmp(command, connect_command) != 0) {
		return -1;
	}
	else {
		char* dest = strtok(NULL, " ");
		if(dest == NULL) {
			return -1;
		} else {
			char* dest_ip = strtok(dest, ":");
			if(dest_ip == NULL) {
				return -1;
			} else {
				strcpy(ip, dest_ip);
				char* dest_port = strtok(NULL, "\n");
				if(dest_ip == NULL) {
					return -1;
				} else {
					strcpy(port, dest_port);
					return strlen(ip);
				}
			}
		}
	}
}

int process_login(char* buffer, char* username) {
	char* command = strtok(buffer, " ");
	if(command == NULL || strcmp(command, login_command) != 0) {
		return -1;
	}
	else {
		char* name = strtok(NULL, "\n");
		if(name == NULL) {
			return -1;
		} else {
			strcpy(username, name);
			return strlen(name);
		}
	}
}

int main(int argc, char *argv[])
{

	int sockfd, numbytes;  

	char recvbuf[MAXDATASIZE]= {0};
	char inputbuf[MAXDATASIZE/2]= {0};

	struct addrinfo myhints, *servinfo, *pp;
	int rev;
	char str[INET6_ADDRSTRLEN];

	int size_username;
	char username[30] = {0};
	char server_ip[30]= {0} ;
	char server_port[30]= {0} ;

	fd_set reads, cpy_reads;

	int fd_max ;
	printf("chat>");
	if(fgets(inputbuf, MAXDATASIZE/2, stdin) != NULL) {
		if(process_connect(inputbuf, server_ip, server_port) < 0) {
			fprintf(stderr, "Please connect to a valid server address\
			using the command: connect <ip>:<port>");
			return -1;
		}
	}
	
	memset(&myhints, 0, sizeof(myhints));
	myhints.ai_family = AF_UNSPEC;
	myhints.ai_socktype = SOCK_STREAM;

	if ((rev = getaddrinfo(server_ip, server_port, &myhints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rev));
		return -1;
	}

	// start of connecting //
	for(pp = servinfo; pp != NULL; pp = pp->ai_next) {
		if ((sockfd = socket(pp->ai_family, pp->ai_socktype,
				pp->ai_protocol)) == -1) {
			perror("chat> socket");
			continue;
		}
		if (connect(sockfd, pp->ai_addr, pp->ai_addrlen) == -1) {
			close(sockfd);
			perror("chat> connect");
			continue;
		}

		break;
	}
	if (pp == NULL) {
		fprintf(stderr, "chat> failed to connect\n");
		return -2;
	}
	inet_ntop(pp->ai_family,getInAddr((struct sockaddr *)pp->ai_addr),
			str, sizeof(str));

	printf(" chat> connecting to %s   \n\n", str);// curline++;
	printf(" chat> Please login using your preferred name.\
	When you want to quit chatting, type 'Ctrl+d'. \n\n");
	printf("chat>");
	fflush(stdout) ;
	freeaddrinfo(servinfo);
	// end of connecting  //
	if(fgets(inputbuf, MAXDATASIZE/2, stdin) != NULL) {
		if(process_login(inputbuf, username) < 0) {
			fprintf(stderr, "Please log in first with your chosen handle \
			via the command: login <username>");
		}
	}
	
	size_username= checkSizeOfCString(username, 30);
	// start of joining  //
	_UPACKET sendpacket ;  // the packet to be sent
	memset(&sendpacket, 0, sizeof(_UPACKET));

	short size_packet ;
	/// packaging the sendpacket
	size_packet =  packaging(JOIN, ATTR_USERNAME ,size_username, username,  &sendpacket); 
	// send the packet
	if ((send(sockfd, sendpacket.cPacket,size_packet , 0)) == -1) 
			perror("send");
	//end of joining //

	FD_ZERO(&reads);
	FD_SET(sockfd, &reads);
	FD_SET(STDIN_FILENO, &reads);
	fd_max = sockfd ;

	printf("chat>");
	fflush(stdout) ;

	while(1){
		cpy_reads = reads ;
		int numfd = 0 ;
		numfd = select(fd_max+1, &cpy_reads, 0, 0, NULL);

		// multiplexing
		if(numfd == 0 ) {
			
		} else if(numfd > 0){
			int l;
			for(l= 0; l<fd_max+1 ; l++){
				if(FD_ISSET(l, &cpy_reads)){
					if(l==sockfd){
						///start of receive //
						int i ;
						for ( i = 0 ; i < MAXDATASIZE ; i++) {
							recvbuf[i]= 0 ;
						}
						if ((numbytes = read(sockfd, recvbuf, MAXDATASIZE-1)) == -1) {
							perror("chat> recv");
							return -2;
						}
						else if (numbytes==0) {
							fprintf(stderr, "chat> server is disconnected.\n") ; 
							return -1;
						}				
						else if (numbytes >=4){
							numbytes = 0 ;
							_UPACKET2 recvpacket2 ;
							memset(&recvpacket2, 0, sizeof(_UPACKET2));
							short attr_count = parsing(numbytes, recvbuf, &recvpacket2) ;
							// got send
							if((recvpacket2.strPacket.header.sheader >> 9) == FWD) {
								// got message
								char* message = NULL;
								char* user = NULL;
								short s;
								for(s = 0;s < attr_count;s++) {
									if(recvpacket2.strPacket.payload[s].attrtype.stype == ATTR_MESSAGE)
									{
										message = recvpacket2.strPacket.payload[s].data;
										
									}
									else if(recvpacket2.strPacket.payload[s].attrtype.stype == ATTR_USERNAME)
									{
										user = recvpacket2.strPacket.payload[s].data;
									}
								}
								if(message != NULL){ 
									for(i = 0; i < size_username + 5 ;i++) {
										printf("\b") ;
									}
									if(user != NULL) {
										printf("chat>%s : %s\n", user, message);
									}
									else printf("chat>%s\n", message);
									
									fflush(stdout) ;
								}
							}
							numbytes = 0;
							/// end of receive //
						} // else if (numbytes
					} // if(l==sockfd
					else if(l == STDIN_FILENO){
						// Start of sending//
						int i; // for printf
						short size_data = 0 ;
						memset(inputbuf, 0, MAXDATASIZE/2) ;
						printf("chat>");
						fflush(stdout) ;

						fgets(inputbuf, MAXDATASIZE/2, stdin);
						size_data= checkSizeOfCString(inputbuf, MAXDATASIZE/2);
						if(inputbuf[0] =='\0'){  // When a user wants to quit
							for (i = 0 ; i < size_username+3 ; i++) {
								printf("\b") ;
							}
							fflush(stdout) ;
							printf("chat> ending session \n");
							close(sockfd);
							return 0;
						}
						
						chat_command* cmd = process_command(inputbuf);
						if(strcmp(cmd->keyword, "") == 0) {
							_UPACKET sendpacket ;  // the packet to be sent
							memset(&sendpacket, 0, sizeof(_UPACKET));
							short size_packet ;
							/// packaging the sendpacket
							size_packet =  packaging(SEND, ATTR_MESSAGE ,size_data, inputbuf, &sendpacket);
							// send the packet
							if (send(sockfd, sendpacket.cPacket,size_packet , 0) == -1) {
								perror("send");
							}
						}
						else if(strcmp(cmd->keyword, login_command) == 0) {
							printf("chat> Already logged in as: %s\n",  username);
							
						}
						else if(strcmp(cmd->keyword, connect_command) == 0) {
							printf("chat> Already connected to %s:%s\n", server_ip, server_port);
						}
						else if(strcmp(cmd->keyword, list_command) == 0) {
							_UPACKET sendpacket ;  // the packet to be sent
							memset(&sendpacket, 0, sizeof(_UPACKET));
							/// packaging the sendpacket
							short size_packet = packaging(LIST, 0, 0, inputbuf, &sendpacket);
							// send the packet
							if (send(sockfd, sendpacket.cPacket,size_packet , 0) == -1) {
								perror("send");
							}
						}
						else if(strcmp(cmd->keyword, history_command) == 0) {
							_UPACKET sendpacket ;  // the packet to be sent
							memset(&sendpacket, 0, sizeof(_UPACKET));
							/// packaging the sendpacket
							short size_packet = packaging(HISTORY, 0, 0, inputbuf, &sendpacket);
							// send the packet
							if (send(sockfd, sendpacket.cPacket,size_packet , 0) == -1) {
								perror("send");
							}
						}
						else if(strcmp(cmd->keyword, search_command) == 0) {
							_UPACKET sendpacket ;  // the packet to be sent
							memset(&sendpacket, 0, sizeof(_UPACKET));
							short size_packet ;
							/// packaging the sendpacket
							size_packet =  packaging(SEARCH, ATTR_REGEX, size_data,
												cmd->arguments, &sendpacket);
							// send the packet
							if (send(sockfd, sendpacket.cPacket,size_packet , 0) == -1) {
								perror("send");
							}
						}
						free(cmd);
						// End of sending//
					}
				}// close if(FD_ISSET
			}// close for(l
		} // if(select)
	} // while

	close(sockfd);

	return 0;
}
